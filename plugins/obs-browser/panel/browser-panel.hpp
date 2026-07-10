#pragma once

#include <obs-module.h>
#include <util/platform.h>
#include <util/util.hpp>
#include <QWidget>

#include <functional>
#include <string>

#if defined(__APPLE__)
#include <dlfcn.h>
#endif

#ifdef ENABLE_WAYLAND
#include <obs-nix-platform.h>
#endif

struct QCefCookieManager {
	virtual ~QCefCookieManager() {}

	virtual bool DeleteCookies(const std::string &url, const std::string &name) = 0;
	virtual bool SetStoragePath(const std::string &storage_path, bool persist_session_cookies = false) = 0;
	virtual bool FlushStore() = 0;

	typedef std::function<void(bool)> cookie_exists_cb;

	virtual void CheckForCookie(const std::string &site, const std::string &cookie, cookie_exists_cb callback) = 0;
};

/* ------------------------------------------------------------------------- */

class QCefWidget : public QWidget {
	Q_OBJECT

protected:
	inline QCefWidget(QWidget *parent) : QWidget(parent) {}

public:
	virtual void setURL(const std::string &url) = 0;
	virtual void setStartupScript(const std::string &script) = 0;
	virtual void allowAllPopups(bool allow) = 0;
	virtual void closeBrowser() = 0;
	virtual void reloadPage() = 0;
	virtual bool zoomPage(int direction) = 0;
	virtual void executeJavaScript(const std::string &script) = 0;

signals:
	void titleChanged(const QString &title);
	void urlChanged(const QString &url);
};

/* ------------------------------------------------------------------------- */

struct QCef {
	virtual ~QCef() {}

	virtual bool init_browser(void) = 0;
	virtual bool initialized(void) = 0;
	virtual bool wait_for_browser_init(void) = 0;

	virtual QCefWidget *create_widget(QWidget *parent, const std::string &url,
					  QCefCookieManager *cookie_manager = nullptr) = 0;

	virtual QCefCookieManager *create_cookie_manager(const std::string &storage_path,
							 bool persist_session_cookies = false) = 0;

	virtual BPtr<char> get_cookie_path(const std::string &storage_path) = 0;

	virtual void add_popup_whitelist_url(const std::string &url, QObject *obj) = 0;
	virtual void add_force_popup_url(const std::string &url, QObject *obj) = 0;
};

static inline void *get_browser_lib()
{
	// Disable panels on Wayland for now
	bool isWayland = false;
#ifdef ENABLE_WAYLAND
	isWayland = obs_get_nix_platform() == OBS_NIX_PLATFORM_WAYLAND;
#endif
	if (isWayland)
		return nullptr;

	obs_module_t *browserModule = obs_get_module("obs-browser");

	if (!browserModule)
		return nullptr;

	return obs_get_module_lib(browserModule);
}

static inline QCef *obs_browser_init_panel(void)
{
	void *lib = get_browser_lib();
	QCef *(*create_qcef)(void) = nullptr;

	if (!lib)
		return nullptr;

	create_qcef = (decltype(create_qcef))os_dlsym(lib, "obs_browser_create_qcef");

	if (!create_qcef)
		return nullptr;

	return create_qcef();
}

static inline int obs_browser_qcef_version(void)
{
	void *lib = get_browser_lib();
	int (*qcef_version)(void) = nullptr;

	if (!lib)
		return 0;

	qcef_version = (decltype(qcef_version))os_dlsym(lib, "obs_browser_qcef_version_export");

	if (!qcef_version)
		return 0;

	return qcef_version();
}
