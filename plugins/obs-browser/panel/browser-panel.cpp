#include "browser-panel-internal.hpp"
#include "browser-panel-client.hpp"
#include "cef-headers.hpp"
#include "browser-app.hpp"

#include <QWindow>
#include <QApplication>

#ifdef ENABLE_BROWSER_QT_LOOP
#include <QEventLoop>
#include <QThread>
#endif

#ifdef __APPLE__
#include <objc/objc.h>
#endif

#include <obs-module.h>
#include <util/threading.h>
#include <util/base.h>
#include <thread>
#include <cmath>

#if !defined(_WIN32) && !defined(__APPLE__)
#include <X11/Xlib.h>
#endif

extern bool QueueCEFTask(std::function<void()> task);
extern "C" void obs_browser_initialize(void);
extern os_event_t *cef_started_event;

std::mutex popup_whitelist_mutex;
std::vector<PopupWhitelistInfo> popup_whitelist;
std::vector<PopupWhitelistInfo> forced_popups;

static int zoomLvls[] = {25, 33, 50, 67, 75, 80, 90, 100, 110, 125, 150, 175, 200, 250, 300, 400};

namespace {
void detachBrowserWindow(CefRefPtr<CefBrowserHost> host)
{
#ifdef _WIN32
	HWND hwnd = (HWND)host->GetWindowHandle();
	if (hwnd) {
		ShowWindow(hwnd, SW_HIDE);
		SetParent(hwnd, nullptr);
	}
#elif __APPLE__
	SEL retain = sel_getUid("retain");
	SEL release = sel_getUid("release");
	SEL removeFromSuperview = sel_getUid("removeFromSuperview");
	void *(*msgSend)(id, SEL) = (void *(*)(id, SEL))objc_msgSend;

	id view = static_cast<id>(host->GetWindowHandle());
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	if (view && view->isa) {
		msgSend(view, retain);
		msgSend(view, removeFromSuperview);
		msgSend(view, release);
	}
#pragma clang diagnostic pop
#else
	UNUSED_PARAMETER(host);
#endif
}
} // namespace

/* ------------------------------------------------------------------------- */

class CookieCheck : public CefCookieVisitor {
public:
	QCefCookieManager::cookie_exists_cb callback;
	std::string target;
	bool cookie_found = false;

	inline CookieCheck(QCefCookieManager::cookie_exists_cb callback_, const std::string target_)
		: callback(callback_),
		  target(target_)
	{
	}

	virtual ~CookieCheck() { callback(cookie_found); }

	virtual bool Visit(const CefCookie &cookie, int, int, bool &) override
	{
		CefString cef_name = cookie.name.str;
		std::string name = cef_name;

		if (name == target) {
			cookie_found = true;
			return false;
		}
		return true;
	}

	IMPLEMENT_REFCOUNTING(CookieCheck);
};

struct QCefCookieManagerInternal : QCefCookieManager {
	CefRefPtr<CefCookieManager> cm;
	CefRefPtr<CefRequestContext> rc;

	QCefCookieManagerInternal(const std::string &storage_path, bool persist_session_cookies)
	{
		if (os_event_try(cef_started_event) != 0)
			throw "Browser thread not initialized";

		BPtr<char> rpath = obs_module_config_path(storage_path.c_str());
		if (os_mkdirs(rpath.Get()) == MKDIR_ERROR)
			throw "Failed to create cookie directory";

		BPtr<char> path = os_get_abs_path_ptr(rpath.Get());

		CefRequestContextSettings settings;
#if CHROME_VERSION_BUILD <= 6533
		settings.persist_user_preferences = 1;
#endif
		CefString(&settings.cache_path) = path.Get();
		rc = CefRequestContext::CreateContext(settings, CefRefPtr<CefRequestContextHandler>());
		if (rc)
			cm = rc->GetCookieManager(nullptr);

		UNUSED_PARAMETER(persist_session_cookies);
	}

	virtual bool DeleteCookies(const std::string &url, const std::string &name) override
	{
		return !!cm ? cm->DeleteCookies(url, name, nullptr) : false;
	}

	virtual bool SetStoragePath(const std::string &storage_path, bool persist_session_cookies) override
	{
		BPtr<char> rpath = obs_module_config_path(storage_path.c_str());
		BPtr<char> path = os_get_abs_path_ptr(rpath.Get());

		CefRequestContextSettings settings;
#if CHROME_VERSION_BUILD <= 6533
		settings.persist_user_preferences = 1;
#endif
		CefString(&settings.cache_path) = storage_path;
		rc = CefRequestContext::CreateContext(settings, CefRefPtr<CefRequestContextHandler>());
		if (rc)
			cm = rc->GetCookieManager(nullptr);

		UNUSED_PARAMETER(persist_session_cookies);
		return true;
	}

	virtual bool FlushStore() override { return !!cm ? cm->FlushStore(nullptr) : false; }

	virtual void CheckForCookie(const std::string &site, const std::string &cookie,
				    cookie_exists_cb callback) override
	{
		if (!cm)
			return;

		CefRefPtr<CookieCheck> c = new CookieCheck(callback, cookie);
		cm->VisitUrlCookies(site, false, c);
	}
};

/* ------------------------------------------------------------------------- */

QCefWidgetInternal::QCefWidgetInternal(QWidget *parent, const std::string &url_, CefRefPtr<CefRequestContext> rqc_)
	: QCefWidget(parent),
	  url(url_),
	  rqc(rqc_)
{
	setAttribute(Qt::WA_PaintOnScreen);
	setAttribute(Qt::WA_StaticContents);
	setAttribute(Qt::WA_NoSystemBackground);
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAttribute(Qt::WA_DontCreateNativeAncestors);
	setAttribute(Qt::WA_NativeWindow);

	setFocusPolicy(Qt::ClickFocus);

#ifndef __APPLE__
	window = new QWindow();
	window->setFlags(Qt::FramelessWindowHint);
#endif
}

QCefWidgetInternal::~QCefWidgetInternal()
{
	closeBrowser();
}

void QCefWidgetInternal::closeBrowser()
{
	if (!cefBrowser) {
		return;
	}

	CefRefPtr<CefBrowserHost> host{cefBrowser->GetHost()};

	if (!host) {
		return;
	}

	QEventLoop browserCloseLoop;

	// Ensure that the native window used by CEF is not attached to the widget view hierarchy while the browser
	// is closed.
	//
	// If the host window is not considered "destroyed" by the time CEF destroys the web contents of the associated
	// browser object, it will close the host window itself. The "host" window in this case would be OBS Studio's
	// main window however. So to ensure this cannot happen, the native window needs to be detached from the Qt
	// view hierarchy so there is no associated host window to close.
	auto preCloseBrowser = [&host]() {
		detachBrowserWindow(host);
	};

	auto closeBrowser = [&host]() {
		host->CloseBrowser(true);
	};

	connect(this, &QCefWidgetInternal::readyToClose, &browserCloseLoop, &QEventLoop::quit);

	QTimer::singleShot(0, &browserCloseLoop, preCloseBrowser);
	QTimer::singleShot(0, &browserCloseLoop, closeBrowser);
	QTimer::singleShot(1000, &browserCloseLoop, &QEventLoop::quit);

	browserCloseLoop.exec();

	CefRefPtr<CefClient> client{host->GetClient()};

	if (client) {
		QCefBrowserClient *browserClient{static_cast<QCefBrowserClient *>(client.get())};
		browserClient->widget = nullptr;
	}

	cefBrowser = nullptr;
}

#ifdef __linux__
static bool XWindowHasAtom(Display *display, Window w, Atom a)
{
	Atom type;
	int format;
	unsigned long nItems;
	unsigned long bytesAfter;
	unsigned char *data = NULL;

	if (XGetWindowProperty(display, w, a, 0, LONG_MAX, False, AnyPropertyType, &type, &format, &nItems, &bytesAfter,
			       &data) != Success)
		return false;

	if (data)
		XFree(data);

	return type != None;
}

/* On Linux / X11, CEF sets the XdndProxy of the toplevel window
 * it's attached to, so that it can read drag events. When this
 * toplevel happens to be OBS Studio's main window (e.g. when a
 * browser panel is docked into to the main window), setting the
 * XdndProxy atom ends up breaking DnD of sources and scenes. Thus,
 * we have to manually unset this atom.
 */
void QCefWidgetInternal::unsetToplevelXdndProxy()
{
	if (!cefBrowser)
		return;

	CefWindowHandle browserHandle = cefBrowser->GetHost()->GetWindowHandle();
	Display *xDisplay = cef_get_xdisplay();
	Window toplevel, root, parent, *children;
	unsigned int nChildren;
	bool found = false;

	toplevel = browserHandle;

	// Find the toplevel
	Atom netWmPidAtom = XInternAtom(xDisplay, "_NET_WM_PID", False);
	do {
		if (XQueryTree(xDisplay, toplevel, &root, &parent, &children, &nChildren) == 0)
			return;

		if (children)
			XFree(children);

		if (root == parent || !XWindowHasAtom(xDisplay, parent, netWmPidAtom)) {
			found = true;
			break;
		}
		toplevel = parent;
	} while (true);

	if (!found)
		return;

	// Check if the XdndProxy property is set
	Atom xDndProxyAtom = XInternAtom(xDisplay, "XdndProxy", False);
	if (needsDeleteXdndProxy && !XWindowHasAtom(xDisplay, toplevel, xDndProxyAtom)) {
		QueueCEFTask([this]() { unsetToplevelXdndProxy(); });
		return;
	}

	XDeleteProperty(xDisplay, toplevel, xDndProxyAtom);
	needsDeleteXdndProxy = false;
}
#endif

void QCefWidgetInternal::Init()
{
#ifndef __APPLE__
	WId handle = window->winId();
	QSize size = this->size();
	size *= devicePixelRatioF();
	bool success = QueueCEFTask(
		[this, handle, size]()
#else
	WId handle = winId();
	bool success = QueueCEFTask(
		[this, handle]()
#endif
		{
			CefWindowInfo windowInfo;

			/* Make sure Init isn't called more than once. */
			if (cefBrowser)
				return;

#ifdef __APPLE__
			QSize size = this->size();
#endif

#if CHROME_VERSION_BUILD >= 6533
			windowInfo.runtime_style = CEF_RUNTIME_STYLE_ALLOY;
#endif

			windowInfo.SetAsChild((CefWindowHandle)handle, CefRect(0, 0, size.width(), size.height()));

			CefRefPtr<QCefBrowserClient> browserClient =
				new QCefBrowserClient(this, script, allowAllPopups_);

			CefBrowserSettings cefBrowserSettings;
			cefBrowser = CefBrowserHost::CreateBrowserSync(windowInfo, browserClient, url,
								       cefBrowserSettings,
								       CefRefPtr<CefDictionaryValue>(), rqc);

#ifdef __linux__
			QueueCEFTask([this]() { unsetToplevelXdndProxy(); });
#endif
		});

	if (success) {
		timer.stop();
#ifndef __APPLE__
		if (!container) {
			container = QWidget::createWindowContainer(window, this);
			container->show();
		}

		Resize();
#endif
	}
}

void QCefWidgetInternal::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
#ifndef __APPLE__
	Resize();
}

void QCefWidgetInternal::Resize()
{
	QSize size = this->size() * devicePixelRatioF();

	bool success = QueueCEFTask([this, size]() {
		if (!cefBrowser)
			return;

		CefWindowHandle handle = cefBrowser->GetHost()->GetWindowHandle();

		if (!handle)
			return;

#ifdef _WIN32
		SetWindowPos((HWND)handle, nullptr, 0, 0, size.width(), size.height(),
			     SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
		SendMessage((HWND)handle, WM_SIZE, 0, MAKELPARAM(size.width(), size.height()));
#else
		Display *xDisplay = cef_get_xdisplay();

		if (!xDisplay)
			return;

		XWindowChanges changes = {0};
		changes.x = 0;
		changes.y = 0;
		changes.width = size.width();
		changes.height = size.height();
		XConfigureWindow(xDisplay, (Window)handle, CWX | CWY | CWHeight | CWWidth, &changes);
#if CHROME_VERSION_BUILD >= 4638
		XSync(xDisplay, false);
#endif
#endif
	});

	if (success && container)
		container->resize(size.width(), size.height());
#endif
}

void QCefWidgetInternal::finishCloseBrowser()
{
	emit readyToClose();
}

void QCefWidgetInternal::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);

	if (!cefBrowser) {
		obs_browser_initialize();
		connect(&timer, &QTimer::timeout, this, &QCefWidgetInternal::Init);
		timer.start(500);
		Init();
	}
}

QPaintEngine *QCefWidgetInternal::paintEngine() const
{
	return nullptr;
}

void QCefWidgetInternal::setURL(const std::string &url_)
{
	url = url_;
	if (cefBrowser) {
		cefBrowser->GetMainFrame()->LoadURL(url);
	}
}

void QCefWidgetInternal::reloadPage()
{
	if (cefBrowser)
		cefBrowser->ReloadIgnoreCache();
}

void QCefWidgetInternal::setStartupScript(const std::string &script_)
{
	script = script_;
}

void QCefWidgetInternal::executeJavaScript(const std::string &script_)
{
	if (!cefBrowser)
		return;

	CefRefPtr<CefFrame> frame = cefBrowser->GetMainFrame();
	std::string url = frame->GetURL();
	frame->ExecuteJavaScript(script_, url, 0);
}

void QCefWidgetInternal::allowAllPopups(bool allow)
{
	allowAllPopups_ = allow;
}

bool QCefWidgetInternal::zoomPage(int direction)
{
	if (!cefBrowser || direction < -1 || direction > 1)
		return false;

	CefRefPtr<CefBrowserHost> host = cefBrowser->GetHost();
	if (direction == 0) {
		// Reset zoom
		host->SetZoomLevel(0);
		return true;
	}

	int currentZoomPercent = round(pow(1.2, host->GetZoomLevel()) * 100.0);
	int zoomCount = sizeof(zoomLvls) / sizeof(zoomLvls[0]);
	int zoomIdx = 0;

	while (zoomIdx < zoomCount) {
		if (zoomLvls[zoomIdx] == currentZoomPercent) {
			break;
		}
		zoomIdx++;
	}
	if (zoomIdx == zoomCount)
		return false;

	int newZoomIdx = zoomIdx;
	if (direction == -1 && zoomIdx > 0) {
		// Zoom out
		newZoomIdx -= 1;
	} else if (direction == 1 && zoomIdx >= 0 && zoomIdx < zoomCount - 1) {
		// Zoom in
		newZoomIdx += 1;
	}

	if (newZoomIdx != zoomIdx) {
		int newZoomLvl = zoomLvls[newZoomIdx];
		// SetZoomLevel only accepts a zoomLevel, not a percentage
		host->SetZoomLevel(log(newZoomLvl / 100.0) / log(1.2));
		return true;
	}
	return false;
}

/* ------------------------------------------------------------------------- */

struct QCefInternal : QCef {
	virtual bool init_browser(void) override;
	virtual bool initialized(void) override;
	virtual bool wait_for_browser_init(void) override;

	virtual QCefWidget *create_widget(QWidget *parent, const std::string &url,
					  QCefCookieManager *cookie_manager) override;

	virtual QCefCookieManager *create_cookie_manager(const std::string &storage_path,
							 bool persist_session_cookies) override;

	virtual BPtr<char> get_cookie_path(const std::string &storage_path) override;

	virtual void add_popup_whitelist_url(const std::string &url, QObject *obj) override;
	virtual void add_force_popup_url(const std::string &url, QObject *obj) override;
};

bool QCefInternal::init_browser(void)
{
	if (os_event_try(cef_started_event) == 0)
		return true;

	obs_browser_initialize();
	return false;
}

bool QCefInternal::initialized(void)
{
	return os_event_try(cef_started_event) == 0;
}

bool QCefInternal::wait_for_browser_init(void)
{
	return os_event_wait(cef_started_event) == 0;
}

QCefWidget *QCefInternal::create_widget(QWidget *parent, const std::string &url, QCefCookieManager *cm)
{
	QCefCookieManagerInternal *cmi = reinterpret_cast<QCefCookieManagerInternal *>(cm);

	return new QCefWidgetInternal(parent, url, cmi ? cmi->rc : nullptr);
}

QCefCookieManager *QCefInternal::create_cookie_manager(const std::string &storage_path, bool persist_session_cookies)
{
	try {
		return new QCefCookieManagerInternal(storage_path, persist_session_cookies);
	} catch (const char *error) {
		blog(LOG_ERROR, "Failed to create cookie manager: %s", error);
		return nullptr;
	}
}

BPtr<char> QCefInternal::get_cookie_path(const std::string &storage_path)
{
	BPtr<char> rpath = obs_module_config_path(storage_path.c_str());
	return os_get_abs_path_ptr(rpath.Get());
}

void QCefInternal::add_popup_whitelist_url(const std::string &url, QObject *obj)
{
	std::lock_guard<std::mutex> lock(popup_whitelist_mutex);
	popup_whitelist.emplace_back(url, obj);
}

void QCefInternal::add_force_popup_url(const std::string &url, QObject *obj)
{
	std::lock_guard<std::mutex> lock(popup_whitelist_mutex);
	forced_popups.emplace_back(url, obj);
}

extern "C" EXPORT QCef *obs_browser_create_qcef(void)
{
	return new QCefInternal();
}

#define BROWSER_PANEL_VERSION 3

extern "C" EXPORT int obs_browser_qcef_version_export(void)
{
	return BROWSER_PANEL_VERSION;
}
