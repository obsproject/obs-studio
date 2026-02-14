/******************************************************************************
    Copyright (C) 2025-2026 pkv <pkv@obsproject.com>
    This file is part of obs-vst3.
    It uses the Steinberg VST3 SDK, which is licensed under MIT license.
    See https://github.com/steinbergmedia/vst3sdk for details.
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#include "VST3EditorWindow.h"
#include "pluginterfaces/base/funknown.h"
#include "util/c99defs.h"

#include <string>
#include <cstring>
#include <utility>

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
		: view_(view),
		  win_(0),
		  frame_(nullptr),
		  title_(std::move(title)),
		  display_(display),
		  runloop_(runloop)
	{
		resizeable_ = view->canResize() == Steinberg::kResultTrue;
		resizeDebounceTimer_ = new QTimer();
		resizeDebounceTimer_->setSingleShot(true);
		resizeDebounceTimer_->setInterval(100);
		QObject::connect(resizeDebounceTimer_, &QTimer::timeout, [this]() { this->handleDebouncedResize(); });
	}
	~Impl()
	{
		if (win_ && display_) {
			if (plugWindow_) {
				runloop_->unregisterWindow(plugWindow_);
				XUnmapWindow(display_, plugWindow_);
				XReparentWindow(display_, plugWindow_, RootWindow(display_, DefaultScreen(display_)), 0,
						0);
				XSync(display_, False);
				plugWindow_ = 0;
			}
			XDestroyWindow(display_, win_);
			win_ = 0;
		}

		if (frame_) {
			delete frame_;
			frame_ = nullptr;
		}
		view_ = nullptr;
	}

	auto getXEmbedInfo() -> XEmbedInfo *
	{
		int actualFormat;
		unsigned long itemsReturned;
		unsigned long bytesAfterReturn;
		Atom actualType;
		XEmbedInfo *xembedInfo = NULL;
		if (xEmbedInfoAtom == None)
			xEmbedInfoAtom = XInternAtom(display_, "_XEMBED_INFO", true);
		auto err = XGetWindowProperty(display_, plugWindow_, xEmbedInfoAtom, 0, sizeof(xembedInfo), false,
					      xEmbedInfoAtom, &actualType, &actualFormat, &itemsReturned,
					      &bytesAfterReturn, reinterpret_cast<unsigned char **>(&xembedInfo));
		if (err != Success)
			return nullptr;
		return xembedInfo;
	}

	void setClient(Window client)
	{
		if (client != 0) {
			plugWindow_ = client;
			fprintf(stderr, "[vst3][xembed] handshake parent=0x%lx child=0x%lx\n", (unsigned long)win_,
				(unsigned long)client);

			XClearArea(display_, client, 0, 0, 1, 1, True);
			runloop_->registerWindow(plugWindow_,
						 [this](const XEvent &e) { return handleMainWindowEvent(e); });
			XResizeWindow(display_, plugWindow_, mCurrentSize.width, mCurrentSize.height);
			auto eventMask = StructureNotifyMask | PropertyChangeMask | FocusChangeMask | ExposureMask;
			XWindowAttributes parentAttr, clientAttr;
			XGetWindowAttributes(display_, win_, &parentAttr);
			XGetWindowAttributes(display_, plugWindow_, &clientAttr);

			if (parentAttr.colormap != clientAttr.colormap) {
				fprintf(stderr, "WARNING: Colormap mismatch; fixing\n");
				XSetWindowColormap(display_, win_, clientAttr.colormap);
			}

			if ((eventMask & clientAttr.your_event_mask) != eventMask)
				XSelectInput(display_, client, clientAttr.your_event_mask | eventMask);

			if (xEmbedAtom == None)
				xEmbedAtom = XInternAtom(display_, "_XEMBED", False);
			send_xembed_message(display_, plugWindow_, xEmbedAtom, XEMBED_EMBEDDED_NOTIFY, 0, win_, 0);

			if (xEmbedInfoAtom == None)
				xEmbedInfoAtom = XInternAtom(display_, "_XEMBED_INFO", False);

			struct XEmbedInfo info{0, 0};
			XChangeProperty(display_, plugWindow_, xEmbedInfoAtom, xEmbedInfoAtom, 32, PropModeReplace,
					reinterpret_cast<const unsigned char *>(&info), 2);

			XMapWindow(display_, plugWindow_);
			XSync(display_, False);
			XFlush(display_);
		}
	}

	bool handleMainWindowEvent(const XEvent &event)
	{
		bool handled = false;
		if (event.xany.window == plugWindow_ && plugWindow_ != 0) {
			switch (event.type) {
			case Expose:
				/* DEBUG
				 if (view_) {
					fprintf(stderr, "[vst3] Expose event received by plugWindow_\n");
				}*/
				return true;
			case PropertyNotify:
				if (event.xproperty.atom == xEmbedInfoAtom) {
					// should Map or UnMap
				}
				return true;
			case ConfigureNotify:
				if (resizeable_) {
					XWindowAttributes hostAttr;
					XGetWindowAttributes(display_, win_, &hostAttr);
					XWindowAttributes attr;
					XGetWindowAttributes(display_, plugWindow_, &attr);
					if (attr.width != hostAttr.width || attr.height != hostAttr.height)
						XResizeWindow(display_, win_, attr.width, attr.height);
				}
				return true;
			case ClientMessage: {
				/* DEBUG:
				  auto name = XGetAtomName(display_, event.xclient.message_type);
				  printf("child x11 window received the ClientMessage atom %s", name);*/
				if (event.xclient.message_type == xEmbedAtom) {
					switch (event.xclient.data.l[1]) {
					case XEMBED_REQUEST_FOCUS: {
						XSetInputFocus(display_, plugWindow_, RevertToParent, CurrentTime);
						send_xembed_message(display_, plugWindow_, xEmbedAtom, XEMBED_FOCUS_IN,
								    0, win_, 0);
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
		} else if (event.xany.window == win_ && win_ != 0) {
			switch (event.type) {
			case MapNotify:
			case VisibilityNotify:
			case Expose:
				if (plugWindow_) {
					XClearArea(display_, plugWindow_, 0, 0, 0, 0, True);
					XFlush(display_);
				}
				return false;
			case ClientMessage: {
				/* DEBUG:
				 fprintf(stderr, "[handler] ClientMessage: win=0x%lx atom=%ld\n", event.xclient.window,
					event.xclient.message_type);*/
				if ((Atom)event.xclient.message_type == _wmProtocols &&
				    (Atom)event.xclient.data.l[0] == _wmDelete) {
					close();
					wasClosed_ = true;
					return true;
				}
				break;
			}
			case DestroyNotify:
				if (event.xdestroywindow.window == win_) {
					close();
					wasClosed_ = true;
					return true;
				}
				break;

			case UnmapNotify:
				if (event.xunmap.window == win_) {
					close();
					wasClosed_ = true;
					return false;
				}
				break;
			case ReparentNotify:
				if (event.xreparent.parent == win_ && event.xreparent.window != plugWindow_) {
					setClient(event.xreparent.window);
					return true;
				}
				break;
			case CreateNotify:
				if (event.xcreatewindow.parent != event.xcreatewindow.window &&
				    event.xcreatewindow.parent == win_ && event.xcreatewindow.window != plugWindow_) {
					setClient(event.xcreatewindow.window);
					return true;
				}
				break;
			case ConfigureNotify: {
				int newWidth = event.xconfigure.width;
				int newHeight = event.xconfigure.height;
				Size newSize = {newWidth, newHeight};

				if (mCurrentSize != newSize) {
					pendingResize_ = newSize;
					if (!resizeScheduled_) {
						resizeScheduled_ = true;
						resizeDebounceTimer_->start();
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
		if (!display_)
			return false;
		xEmbedInfoAtom = XInternAtom(display_, "_XEMBED_INFO", true);

		int screen = DefaultScreen(display_);

		XSetWindowAttributes winAttr{};
		winAttr.border_pixel = BlackPixel(display_, screen);
		winAttr.background_pixmap = None;
		winAttr.colormap = DefaultColormap(display_, screen);
		winAttr.override_redirect = False;
		unsigned long mask = CWEventMask | CWBorderPixel | CWBackPixmap | CWColormap;

		win_ = XCreateWindow(display_, RootWindow(display_, screen), 0, 0, width, height, 0,
				     DefaultDepth(display_, screen), InputOutput, DefaultVisual(display_, screen), mask,
				     &winAttr);

		// Register for events
		runloop_->registerWindow(win_, [this](const XEvent &event) { return handleMainWindowEvent(event); });

		Size size = {width, height};
		resize(size, true);
		XFlush(display_);
		XSelectInput(display_, win_,
			     ExposureMask | StructureNotifyMask | SubstructureNotifyMask | FocusChangeMask |
				     VisibilityChangeMask);

		auto sizeHints = XAllocSizeHints();
		sizeHints->flags = PMinSize;
		if (!resizeable_) {
			sizeHints->flags |= PMaxSize;
			sizeHints->min_width = sizeHints->max_width = size.width;
			sizeHints->min_height = sizeHints->max_height = size.height;
		} else {
			sizeHints->min_width = sizeHints->min_height = 80;
		}
		XSetWMNormalHints(display_, win_, sizeHints);
		XFree(sizeHints);

		XStoreName(display_, win_, title_.data());

		_wmProtocols = XInternAtom(display_, "WM_PROTOCOLS", False);
		_wmDelete = XInternAtom(display_, "WM_DELETE_WINDOW", False);
		Atom protos[1] = {_wmDelete};
		XSetWMProtocols(display_, win_, protos, 1);

		Atom windowType = XInternAtom(display_, "_NET_WM_WINDOW_TYPE", False);
		Atom normalType = XInternAtom(display_, "_NET_WM_WINDOW_TYPE_NORMAL", False);
		XChangeProperty(display_, win_, windowType, XA_ATOM, 32, PropModeReplace, (unsigned char *)&normalType,
				1);

		Atom allowedActions = XInternAtom(display_, "_NET_WM_ALLOWED_ACTIONS", False);
		Atom actions[3] = {XInternAtom(display_, "_NET_WM_ACTION_MINIMIZE", False),
				   XInternAtom(display_, "_NET_WM_ACTION_CLOSE", False),
				   XInternAtom(display_, "_NET_WM_ACTION_MOVE", False)};
		XChangeProperty(display_, win_, allowedActions, XA_ATOM, 32, PropModeReplace, (unsigned char *)actions,
				3);

		XMapWindow(display_, win_);
		XFlush(display_);

		frame_ = new PlugFrameImpl(display_, win_, this);
		if (view_) {
			view_->setFrame(frame_);
			if (view_->attached((void *)win_, Steinberg::kPlatformTypeX11EmbedWindowID) !=
			    Steinberg::kResultOk) {
				view_->setFrame(nullptr);
				return false;
			}
			attached_ = true;
			// "Lazy size" plugin support
			int _width = mCurrentSize.width;
			int _height = mCurrentSize.height;
			Steinberg::ViewRect vr(0, 0, _width, _height);
			Steinberg::ViewRect rect;
			if (view_->getSize(&rect) == Steinberg::kResultOk) {
				if (rect.getWidth() != _width || rect.getHeight() != _height) {
					view_->onSize(&rect);
				}

			} else {
				view_->onSize(&vr);
			}
		}
		XSync(display_, False);
		return true;
	}

	void reattach()
	{
		view_->setFrame(frame_);
		Steinberg::tresult res = view_->attached((void *)win_, Steinberg::kPlatformTypeX11EmbedWindowID);
		if (res != Steinberg::kResultOk) {
			view_->setFrame(nullptr);
		} else {
			attached_ = true;
		}
	}

	void show()
	{
		if (win_ && display_) {
			if (!attached_) {
				reattach();
				setClient(plugWindow_);
				XReparentWindow(display_, plugWindow_, win_, 0, 0);
			}

			if (attached_) {
				Steinberg::ViewRect cur{};
				if (view_->getSize(&cur) != Steinberg::kResultTrue)
					view_->onSize(&cur);
				mCurrentSize.width = cur.getWidth();
				mCurrentSize.height = cur.getHeight();
				XMapWindow(display_, win_);
				XMapWindow(display_, plugWindow_);
				XMapRaised(display_, win_);
				XSync(display_, False);
				XFlush(display_);
			}
		}
		wasClosed_ = false;
	}

	void detach()
	{
		if (!attached_)
			return;
		if (plugWindow_) {
			runloop_->unregisterWindow(plugWindow_);
			XUnmapWindow(display_, plugWindow_);
			XReparentWindow(display_, plugWindow_, RootWindow(display_, DefaultScreen(display_)), 0, 0);
			XSync(display_, False);
		}

		if (view_) {
			view_->removed();
			view_->setFrame(nullptr);
			attached_ = false;
		}
	}

	void close()
	{
		detach();
		if (win_ && display_)
			XUnmapWindow(display_, win_);

		XFlush(display_);
	}

	void resize(Size newSize, bool force)
	{
		if (!force && mCurrentSize == newSize)
			return;
		if (win_)
			XResizeWindow(display_, win_, newSize.width, newSize.height);
		mCurrentSize = newSize;
	}

	void onResize(Size newSize)
	{
		if (view_) {
			Steinberg::ViewRect r{};
			r.right = newSize.width;
			r.bottom = newSize.height;
			Steinberg::ViewRect r2{};
			if (view_->getSize(&r2) == Steinberg::kResultTrue && r != r2)
				view_->onSize(&r);
		}
	}

	Size constrainSize(Size requestedSize)
	{
		if (view_) {
			Steinberg::ViewRect r{};
			r.right = requestedSize.width;
			r.bottom = requestedSize.height;
			if (view_->checkSizeConstraint(&r) != Steinberg::kResultTrue) {
				requestedSize.width = r.right - r.left;
				requestedSize.height = r.bottom - r.top;
			}
		}
		return requestedSize;
	}

	[[nodiscard]] bool getClosedState() const { return wasClosed_; }

	RunLoopImpl *getRunLoop() { return runloop_; }

	void handleDebouncedResize()
	{
		resizeScheduled_ = false;
		Size newSize = pendingResize_;

		auto constraintSize = constrainSize(newSize);
		if (constraintSize != mCurrentSize) {
			mCurrentSize = constraintSize;
			onResize(constraintSize);
		}

		if (plugWindow_)
			XResizeWindow(display_, plugWindow_, constraintSize.width, constraintSize.height);

		XSync(display_, False);
	}

	class PlugFrameImpl : public Steinberg::IPlugFrame {
	public:
		PlugFrameImpl(Display *display, Window win, Impl *owner) : display_(display), win_(win), owner_(owner)
		{
		}
		Steinberg::tresult PLUGIN_API resizeView(Steinberg::IPlugView *view,
							 Steinberg::ViewRect *newSize) override
		{
			if (display_ && newSize) {
				if (owner_->view_->checkSizeConstraint(newSize) == Steinberg::kResultOk) {
				};
				XResizeWindow(display_, win_, newSize->getWidth(), newSize->getHeight());
				if (owner_->plugWindow_)
					XResizeWindow(display_, owner_->plugWindow_, newSize->getWidth(),
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
				*obj = static_cast<Steinberg::Linux::IRunLoop *>(owner_->getRunLoop());
				return Steinberg::kResultOk;
			}
			*obj = nullptr;
			return Steinberg::kNoInterface;
		}
		uint32_t PLUGIN_API addRef() override { return 1; }
		uint32_t PLUGIN_API release() override { return 1; }

	private:
		Display *display_;
		Window win_;
		Impl *owner_;
	};

	Steinberg::IPlugView *view_;
	Window win_;
	Window plugWindow_ = 0;
	PlugFrameImpl *frame_;
	std::string title_;
	Display *display_;
	RunLoopImpl *runloop_;
	bool resizeable_;
	bool wasClosed_ = false;
	Size mCurrentSize{};
	bool isMapped = false;
	Atom xEmbedInfoAtom{None};
	Atom xEmbedAtom{None};
	XEmbedInfo *xembedInfo{nullptr};
	bool attached_ = false;
	Atom _wmDelete{None};
	Atom _wmProtocols{None};
	// A single-shot debounce timer used to coalesce rapid X11 ConfigureNotify events
	// during user or plugin-driven resizing. Without this, parent and child windows
	// can enter a resize ping-pong loop (plugin resizes → host resizes → plugin resizes).
	Size pendingResize_ = {0, 0};
	bool resizeScheduled_ = false;
	QTimer *resizeDebounceTimer_ = nullptr;
};

// --- Interface implementation ---
VST3EditorWindow::VST3EditorWindow(Steinberg::IPlugView *view, const std::string &title, Display *display,
				   RunLoopImpl *runloop)
	: impl_(new Impl(view, title, display, runloop))
{
}
VST3EditorWindow::~VST3EditorWindow()
{
	delete impl_;
}
bool VST3EditorWindow::create(int width, int height)
{
	return impl_->create(width, height);
}
void VST3EditorWindow::show()
{
	impl_->show();
}
void VST3EditorWindow::close()
{
	impl_->close();
}
bool VST3EditorWindow::getClosedState()
{
	return impl_->getClosedState();
}
