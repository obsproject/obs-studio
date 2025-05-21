/* Copyright (c) 2025 pkv <pkv@obsproject.com>
 *
 * This file uses the Steinberg VST3 SDK, which is licensed under MIT license.
 * See https://github.com/steinbergmedia/vst3sdk for details.
 *
 * This file and all modifications by pkv <pkv@obsproject.com> are licensed under
 * the GNU General Public License, version 3 or later, to comply with the SDK license.
 */
#include "VST3EditorWindow.h"
#include <string>
#include <cstring>
#include <utility>
#include "pluginterfaces/base/funknown.h"
#include "util/c99defs.h"

using Coord = int32_t;

struct Size {
	Coord width;
	Coord height;
};

inline bool operator!=(const Size &lhs, const Size &rhs)
{
	return lhs.width != rhs.width || lhs.height != rhs.height;
}

inline bool operator==(const Size &lhs, const Size &rhs)
{
	return lhs.width == rhs.width && lhs.height == rhs.height;
}

inline bool operator==(const Steinberg::ViewRect &r1, const Steinberg::ViewRect &r2)
{
	return memcmp(&r1, &r2, sizeof(Steinberg::ViewRect)) == 0;
}

inline bool operator!=(const Steinberg::ViewRect &r1, const Steinberg::ViewRect &r2)
{
	return !(r1 == r2);
}

/* XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY 0
#define XEMBED_WINDOW_ACTIVATE 1
#define XEMBED_WINDOW_DEACTIVATE 2
#define XEMBED_REQUEST_FOCUS 3
#define XEMBED_FOCUS_IN 4
#define XEMBED_FOCUS_OUT 5
#define XEMBED_FOCUS_NEXT 6
#define XEMBED_FOCUS_PREV 7
/* 8-9 were used for XEMBED_GRAB_KEY/XEMBED_UNGRAB_KEY */
#define XEMBED_MODALITY_ON 10
#define XEMBED_MODALITY_OFF 11
#define XEMBED_REGISTER_ACCELERATOR 12
#define XEMBED_UNREGISTER_ACCELERATOR 13
#define XEMBED_ACTIVATE_ACCELERATOR 14

#define XEMBED_MAPPED (1 << 0)
#ifndef XEMBED_EMBEDDED_NOTIFY
#define XEMBED_EMBEDDED_NOTIFY 0
#endif
#ifndef XEMBED_FOCUS_IN
#define XEMBED_FOCUS_IN 4
#define XEMBED_FOCUS_CURRENT 0
#endif

struct XEmbedInfo {
	uint32_t version;
	uint32_t flags;
};

void send_xembed_message(Display *dpy,                   /* display */
			 Window w,                       /* receiver */
			 Atom messageType, long message, /* message opcode */
			 long detail,                    /* message detail */
			 long data1,                     /* message data 1 */
			 long data2                      /* message data 2 */
)
{
	XEvent ev;
	memset(&ev, 0, sizeof(ev));
	ev.xclient.type = ClientMessage;
	ev.xclient.window = w;
	ev.xclient.message_type = messageType;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = CurrentTime;
	ev.xclient.data.l[1] = message;
	ev.xclient.data.l[2] = detail;
	ev.xclient.data.l[3] = data1;
	ev.xclient.data.l[4] = data2;
	XSendEvent(dpy, w, False, NoEventMask, &ev);
	XSync(dpy, False);
}

class VST3EditorWindow::Impl {
public:
	Impl(Steinberg::IPlugView *view, std::string title, Display *display, RunLoopImpl *runloop)
		: _view(view),
		  _win(0),
		  _frame(nullptr),
		  _title(std::move(title)),
		  _display(display),
		  _runloop(runloop)
	{
		_resizeable = view->canResize() == Steinberg::kResultTrue;
		_resizeDebounceTimer = new QTimer();
		_resizeDebounceTimer->setSingleShot(true);
		_resizeDebounceTimer->setInterval(100); // adjust as needed (40–100ms usually good)
		QObject::connect(_resizeDebounceTimer, &QTimer::timeout, [this]() { this->handleDebouncedResize(); });
	}
	~Impl()
	{
		if (_win && _display) {
			if (_plugWindow) {
				_runloop->unregisterWindow(_plugWindow);
				XUnmapWindow(_display, _plugWindow);
				XReparentWindow(_display, _plugWindow, RootWindow(_display, DefaultScreen(_display)), 0,
						0);
				XSync(_display, False);
				_plugWindow = 0;
			}
			XDestroyWindow(_display, _win);
			_win = 0;
		}
		if (_attached) {
			_view->removed();
			_view->setFrame(nullptr);
		}
		if (_frame) {
			delete _frame;
			_frame = nullptr;
		}
		_view = nullptr;
	}

	auto getXEmbedInfo() -> XEmbedInfo *
	{
		int actualFormat;
		unsigned long itemsReturned;
		unsigned long bytesAfterReturn;
		Atom actualType;
		XEmbedInfo *xembedInfo = NULL;
		if (xEmbedInfoAtom == None)
			xEmbedInfoAtom = XInternAtom(_display, "_XEMBED_INFO", true);
		auto err = XGetWindowProperty(_display, _plugWindow, xEmbedInfoAtom, 0, sizeof(xembedInfo), false,
					      xEmbedInfoAtom, &actualType, &actualFormat, &itemsReturned,
					      &bytesAfterReturn, reinterpret_cast<unsigned char **>(&xembedInfo));
		if (err != Success)
			return nullptr;
		return xembedInfo;
	}

	void setClient(Window client)
	{
		if (client != 0) {
			_plugWindow = client;
			fprintf(stderr, "[vst3][xembed] handshake parent=0x%lx child=0x%lx\n", (unsigned long)_win,
				(unsigned long)client);

			XClearArea(_display, client, 0, 0, 1, 1, True); // force an Expose to “kick” painting
			_runloop->registerWindow(_plugWindow,
						 [this](const XEvent &e) { return handleMainWindowEvent(e); });
			XResizeWindow(_display, _plugWindow, mCurrentSize.width, mCurrentSize.height);
			auto eventMask = StructureNotifyMask | PropertyChangeMask | FocusChangeMask | ExposureMask;
			XWindowAttributes parentAttr, clientAttr;
			XGetWindowAttributes(_display, _win, &parentAttr);
			XGetWindowAttributes(_display, _plugWindow, &clientAttr);

			if (parentAttr.colormap != clientAttr.colormap) {
				fprintf(stderr, "WARNING: Colormap mismatch; fixing\n");
				XSetWindowColormap(_display, _win, clientAttr.colormap);
			}

			if ((eventMask & clientAttr.your_event_mask) != eventMask)
				XSelectInput(_display, client, clientAttr.your_event_mask | eventMask);

			if (xEmbedAtom == None)
				xEmbedAtom = XInternAtom(_display, "_XEMBED", False);
			send_xembed_message(_display, _plugWindow, xEmbedAtom, XEMBED_EMBEDDED_NOTIFY, 0, _win, 0);

			if (xEmbedInfoAtom == None)
				xEmbedInfoAtom = XInternAtom(_display, "_XEMBED_INFO", False);

			struct XEmbedInfo info{0, 0};
			XChangeProperty(_display, _plugWindow, xEmbedInfoAtom, xEmbedInfoAtom, 32, PropModeReplace,
					reinterpret_cast<const unsigned char *>(&info), 2);

			XMapWindow(_display, _plugWindow);
			XSync(_display, False);
			XFlush(_display);
		}
	}

	bool handleMainWindowEvent(const XEvent &event)
	{
		bool handled = false;
		if (event.xany.window == _plugWindow && _plugWindow != 0) {
			switch (event.type) {
			case Expose:
				/* DEBUG
				 if (_view) {
					fprintf(stderr, "[vst3] Expose event received by _plugWindow\n");
				}*/
				return true;
			case PropertyNotify:
				if (event.xproperty.atom == xEmbedInfoAtom) {
					// should Map or UnMap
				}
				return true;
			case ConfigureNotify:
				if (_resizeable) {
					XWindowAttributes hostAttr;
					XGetWindowAttributes(_display, _win, &hostAttr);
					XWindowAttributes attr;
					XGetWindowAttributes(_display, _plugWindow, &attr);
					if (attr.width != hostAttr.width || attr.height != hostAttr.height)
						XResizeWindow(_display, _win, attr.width, attr.height);
				}
				return true;
			case ClientMessage: {
				/* DEBUG:
				  auto name = XGetAtomName(_display, event.xclient.message_type);
				  printf("child x11 window received the ClientMessage atom %s", name);*/
				if (event.xclient.message_type == xEmbedAtom) {
					switch (event.xclient.data.l[1]) {
					case XEMBED_REQUEST_FOCUS: {
						XSetInputFocus(_display, _plugWindow, RevertToParent, CurrentTime);
						send_xembed_message(_display, _plugWindow, xEmbedAtom, XEMBED_FOCUS_IN,
								    0, _win, 0);
						return true;
					}
					default:
						break;
					}
				}
				break;
			}
			default:
				break;
			}
		} else if (event.xany.window == _win && _win != 0) {
			switch (event.type) {
			case MapNotify:
			case VisibilityNotify:
			case Expose:
				if (_plugWindow) {
					XClearArea(_display, _plugWindow, 0, 0, 0, 0, True);
					XFlush(_display);
				}
				return false;
			case ClientMessage: {
				/* DEBUG:
				 fprintf(stderr, "[handler] ClientMessage: win=0x%lx atom=%ld\n", event.xclient.window,
					event.xclient.message_type);*/
				if ((Atom)event.xclient.message_type == _wmProtocols &&
				    (Atom)event.xclient.data.l[0] == _wmDelete) {
					close();
					_wasClosed = true;
					return true;
				}
				break;
			}
			case DestroyNotify:
				if (event.xdestroywindow.window == _win) {
					close();
					_wasClosed = true;
					return true;
				}
				break;

			case UnmapNotify:
				if (event.xunmap.window == _win) {
					close();
					_wasClosed = true;
					return false;
				}
				break;
			case ReparentNotify:
				if (event.xreparent.parent == _win && event.xreparent.window != _plugWindow) {
					setClient(event.xreparent.window);
					return true;
				}
				break;
			case CreateNotify:
				if (event.xcreatewindow.parent != event.xcreatewindow.window &&
				    event.xcreatewindow.parent == _win && event.xcreatewindow.window != _plugWindow) {
					setClient(event.xcreatewindow.window);
					return true;
				}
				break;
			case ConfigureNotify: {
				int newWidth = event.xconfigure.width;
				int newHeight = event.xconfigure.height;
				Size newSize = {newWidth, newHeight};

				if (mCurrentSize != newSize) {
					_pendingResize = newSize;
					if (!_resizeScheduled) {
						_resizeScheduled = true;
						_resizeDebounceTimer->start();
					}
				}
				return true;
			}
			case ResizeRequest: {
				auto width = event.xresizerequest.width;
				auto height = event.xresizerequest.height;
				Size request{width, height};
				if (mCurrentSize != request) {
					auto constraintSize = constrainSize(request);
					if (constraintSize != request) {
					}
					resize(constraintSize, true);
				}
				return true;
			}
			default:
				break;
			}
		}

		return handled;
	}

	bool create(int width, int height)
	{
		if (!_display)
			return false;
		xEmbedInfoAtom = XInternAtom(_display, "_XEMBED_INFO", true);

		int screen = DefaultScreen(_display);

		XSetWindowAttributes winAttr{};
		winAttr.border_pixel = BlackPixel(_display, screen);
		winAttr.background_pixmap = None;
		winAttr.colormap = DefaultColormap(_display, screen);
		winAttr.override_redirect = False;
		unsigned long mask = CWEventMask | CWBorderPixel | CWBackPixmap | CWColormap;

		_win = XCreateWindow(_display, RootWindow(_display, screen), 0, 0, width, height, 0,
				     DefaultDepth(_display, screen), InputOutput, DefaultVisual(_display, screen), mask,
				     &winAttr);

		// Register for events
		_runloop->registerWindow(_win, [this](const XEvent &event) { return handleMainWindowEvent(event); });

		Size size = {width, height};
		resize(size, true);
		XFlush(_display);
		XSelectInput(_display, _win,
			     ExposureMask | StructureNotifyMask | SubstructureNotifyMask | FocusChangeMask |
				     VisibilityChangeMask);

		auto sizeHints = XAllocSizeHints();
		sizeHints->flags = PMinSize;
		if (!_resizeable) {
			sizeHints->flags |= PMaxSize;
			sizeHints->min_width = sizeHints->max_width = size.width;
			sizeHints->min_height = sizeHints->max_height = size.height;
		} else {
			sizeHints->min_width = sizeHints->min_height = 80;
		}
		XSetWMNormalHints(_display, _win, sizeHints);
		XFree(sizeHints);

		XStoreName(_display, _win, _title.data());

		_wmProtocols = XInternAtom(_display, "WM_PROTOCOLS", False);
		_wmDelete = XInternAtom(_display, "WM_DELETE_WINDOW", False);
		Atom protos[1] = {_wmDelete};
		XSetWMProtocols(_display, _win, protos, 1);

		Atom windowType = XInternAtom(_display, "_NET_WM_WINDOW_TYPE", False);
		Atom normalType = XInternAtom(_display, "_NET_WM_WINDOW_TYPE_NORMAL", False);
		XChangeProperty(_display, _win, windowType, XA_ATOM, 32, PropModeReplace, (unsigned char *)&normalType,
				1);

		Atom allowedActions = XInternAtom(_display, "_NET_WM_ALLOWED_ACTIONS", False);
		Atom actions[3] = {XInternAtom(_display, "_NET_WM_ACTION_MINIMIZE", False),
				   XInternAtom(_display, "_NET_WM_ACTION_CLOSE", False),
				   XInternAtom(_display, "_NET_WM_ACTION_MOVE", False)};
		XChangeProperty(_display, _win, allowedActions, XA_ATOM, 32, PropModeReplace, (unsigned char *)actions,
				3);

		XMapWindow(_display, _win);
		XFlush(_display);

		_frame = new PlugFrameImpl(_display, _win, this);
		if (_view) {
			_view->setFrame(_frame);
			if (_view->attached((void *)_win, Steinberg::kPlatformTypeX11EmbedWindowID) !=
			    Steinberg::kResultOk) {
				_view->setFrame(nullptr);
				return false;
			}
			_attached = true;
			// "Lazy size" plugin support
			int _width = mCurrentSize.width;
			int _height = mCurrentSize.height;
			Steinberg::ViewRect vr(0, 0, _width, _height);
			Steinberg::ViewRect rect;
			if (_view->getSize(&rect) == Steinberg::kResultOk) {
				if (rect.getWidth() != _width || rect.getHeight() != _height) {
					_view->onSize(&rect);
				}

			} else {
				_view->onSize(&vr);
			}
		}
		XSync(_display, False);
		return true;
	}

	void reattach()
	{
		_view->setFrame(_frame);
		Steinberg::tresult res = _view->attached((void *)_win, Steinberg::kPlatformTypeX11EmbedWindowID);
		if (res != Steinberg::kResultOk) {
			_view->setFrame(nullptr);
		} else {
			_attached = true;
		}
	}

	void show()
	{
		if (_win && _display) {
			if (!_attached) {
				reattach();
				setClient(_plugWindow);
				XReparentWindow(_display, _plugWindow, _win, 0, 0);
			}

			if (_attached) {
				Steinberg::ViewRect cur{};
				if (_view->getSize(&cur) != Steinberg::kResultTrue)
					_view->onSize(&cur);
				mCurrentSize.width = cur.getWidth();
				mCurrentSize.height = cur.getHeight();
				XMapWindow(_display, _win);
				XMapWindow(_display, _plugWindow);
				XMapRaised(_display, _win);
				XSync(_display, False);
				XFlush(_display);
			}
		}
		_wasClosed = false;
	}

	void detach()
	{
		if (!_attached)
			return;
		if (_plugWindow) {
			_runloop->unregisterWindow(_plugWindow);
			XUnmapWindow(_display, _plugWindow);
			XReparentWindow(_display, _plugWindow, RootWindow(_display, DefaultScreen(_display)), 0, 0);
			XSync(_display, False);
		}

		if (_view) {
			_view->removed();
			_view->setFrame(nullptr);
			_attached = false;
		}
	}

	void close()
	{
		detach();
		if (_win && _display)
			XUnmapWindow(_display, _win);

		XFlush(_display);
	}

	void resize(Size newSize, bool force)
	{
		if (!force && mCurrentSize == newSize)
			return;
		if (_win)
			XResizeWindow(_display, _win, newSize.width, newSize.height);
		mCurrentSize = newSize;
	}

	void onResize(Size newSize)
	{
		if (_view) {
			Steinberg::ViewRect r{};
			r.right = newSize.width;
			r.bottom = newSize.height;
			Steinberg::ViewRect r2{};
			if (_view->getSize(&r2) == Steinberg::kResultTrue && r != r2)
				_view->onSize(&r);
		}
	}

	Size constrainSize(Size requestedSize)
	{
		if (_view) {
			Steinberg::ViewRect r{};
			r.right = requestedSize.width;
			r.bottom = requestedSize.height;
			if (_view->checkSizeConstraint(&r) != Steinberg::kResultTrue) {
				requestedSize.width = r.right - r.left;
				requestedSize.height = r.bottom - r.top;
			}
		}
		return requestedSize;
	}

	[[nodiscard]] bool getClosedState() const { return _wasClosed; }

	RunLoopImpl *getRunLoop() { return _runloop; }

	void handleDebouncedResize()
	{
		_resizeScheduled = false;
		Size newSize = _pendingResize;

		auto constraintSize = constrainSize(newSize);
		if (constraintSize != mCurrentSize) {
			mCurrentSize = constraintSize;
			onResize(constraintSize);
		}

		if (_plugWindow)
			XResizeWindow(_display, _plugWindow, constraintSize.width, constraintSize.height);

		XSync(_display, False);
	}

	class PlugFrameImpl : public Steinberg::IPlugFrame {
	public:
		PlugFrameImpl(Display *display, Window win, Impl *owner) : _display(display), _win(win), _owner(owner)
		{
		}
		Steinberg::tresult PLUGIN_API resizeView(Steinberg::IPlugView *view,
							 Steinberg::ViewRect *newSize) override
		{
			if (_display && newSize) {
				if (_owner->_view->checkSizeConstraint(newSize) == Steinberg::kResultOk) {
				};
				XResizeWindow(_display, _win, newSize->getWidth(), newSize->getHeight());
				if (_owner->_plugWindow)
					XResizeWindow(_display, _owner->_plugWindow, newSize->getWidth(),
						      newSize->getHeight());
				view->onSize(newSize);
				return Steinberg::kResultTrue;
			}
			return Steinberg::kResultFalse;
		}
		Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid, void **obj) override
		{
			if (Steinberg::FUnknownPrivate::iidEqual(_iid, Steinberg::IPlugFrame::iid)) {
				*obj = this;
				return Steinberg::kResultOk;
			}
			if (Steinberg::FUnknownPrivate::iidEqual(_iid, Steinberg::Linux::IRunLoop::iid)) {

				/**obj = nullptr;
				return Steinberg::kResultFalse;*/
				*obj = static_cast<Steinberg::Linux::IRunLoop *>(_owner->getRunLoop());
				return Steinberg::kResultOk;
			}
			*obj = nullptr;
			return Steinberg::kNoInterface;
		}
		uint32_t PLUGIN_API addRef() override { return 1; }
		uint32_t PLUGIN_API release() override { return 1; }

	private:
		Display *_display;
		Window _win;
		Impl *_owner;
	};

	Steinberg::IPlugView *_view;
	//Steinberg::IPlugViewContentScaleSupport *scaleInterface = nullptr;
	Window _win;
	Window _plugWindow = 0;
	PlugFrameImpl *_frame;
	std::string _title;
	Display *_display;
	RunLoopImpl *_runloop;
	bool _resizeable;
	bool _wasClosed = false;
	Size mCurrentSize{};
	bool isMapped = false;
	Atom xEmbedInfoAtom{None};
	Atom xEmbedAtom{None};
	XEmbedInfo *xembedInfo{nullptr};
	bool _attached = false;
	Atom _wmDelete{None};
	Atom _wmProtocols{None};
	Size _pendingResize = {0, 0};
	bool _resizeScheduled = false;
	QTimer *_resizeDebounceTimer = nullptr;
};

// --- Interface implementation ---
VST3EditorWindow::VST3EditorWindow(Steinberg::IPlugView *view, const std::string &title, Display *display,
				   RunLoopImpl *runloop)
	: _impl(new Impl(view, title, display, runloop))
{
}
VST3EditorWindow::~VST3EditorWindow()
{
	delete _impl;
}
bool VST3EditorWindow::create(int width, int height)
{
	return _impl->create(width, height);
}
void VST3EditorWindow::show()
{
	_impl->show();
}
void VST3EditorWindow::close()
{
	_impl->close();
}
bool VST3EditorWindow::getClosedState()
{
	return _impl->getClosedState();
}
