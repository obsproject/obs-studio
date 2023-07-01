#include "twitch-browser-widget.hpp"

#include <QVBoxLayout>

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/platform.h>
#include <util/dstr.h>

constexpr const char *FFZ_SCRIPT = "\
var ffz = document.createElement('script');\
ffz.setAttribute('src','https://cdn.frankerfacez.com/script/script.min.js');\
document.head.appendChild(ffz);";

constexpr const char *BTTV_SCRIPT = "\
localStorage.setItem('bttv_clickTwitchEmotes', true);\
localStorage.setItem('bttv_darkenedMode', true);\
localStorage.setItem('bttv_bttvGIFEmotes', true);\
var bttv = document.createElement('script');\
bttv.setAttribute('src','https://cdn.betterttv.net/betterttv.js');\
document.head.appendChild(bttv);";

TwitchBrowserWidget::TwitchBrowserWidget(const Addon &addon_,
					 const QString &url_,
					 const QString &startupScriptBase_,
					 const QStringList &forcePopupUrl_,
					 const QStringList &popupWhitelistUrl_)
	: addon(addon_),
	  url(url_.toStdString()),
	  startupScriptBase(startupScriptBase_.toStdString()),
	  QWidget()
{
	setMinimumSize(200, 300);

	QString urlList = forcePopupUrl_.join(";");
	forcePopupUrl = strlist_split(urlList.toUtf8().constData(), ';', false);

	urlList = popupWhitelistUrl_.join(";");
	popupWhitelistUrl =
		strlist_split(urlList.toUtf8().constData(), ';', false);

	QVBoxLayout *layout = new QVBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);

	setLayout(layout);
}

void TwitchBrowserWidget::SetAddon(const Addon &newAddon)
{
	if (addon == newAddon)
		return;

	addon = newAddon;

	if (!cefWidget.isNull())
		QMetaObject::invokeMethod(this, "UpdateCefWidget",
					  Qt::QueuedConnection);
}

void TwitchBrowserWidget::showEvent(QShowEvent *event)
{
	UpdateCefWidget();

	QWidget::showEvent(event);
}

void TwitchBrowserWidget::hideEvent(QHideEvent *event)
{
	cefWidget.reset(nullptr);
	QWidget::hideEvent(event);
}

void TwitchBrowserWidget::UpdateCefWidget()
{
	obs_frontend_browser_params params = {0};

	startupScript = obs_frontend_is_theme_dark()
				? "localStorage.setItem('twilight.theme', 1);"
				: "localStorage.setItem('twilight.theme', 0);";
	startupScript += startupScriptBase;

	switch (addon) {
	case NONE:
		break;
	case BOTH:
	case FFZ:
		startupScript += FFZ_SCRIPT;
		if (addon != BOTH)
			break;
		[[fallthrough]];
	case BTTV:
		startupScript += BTTV_SCRIPT;
		break;
	}

	params.url = url.c_str();
	params.startup_script = startupScript.c_str();
	params.enable_cookie = true;

	if (forcePopupUrl)
		params.force_popup_urls = (const char **)forcePopupUrl.Get();
	if (popupWhitelistUrl)
		params.popup_whitelist_urls =
			(const char **)popupWhitelistUrl.Get();

	cefWidget.reset((QWidget *)obs_frontend_get_browser_widget(&params));

	QVBoxLayout *layout = (QVBoxLayout *)this->layout();
	layout->addWidget(cefWidget.get());
	cefWidget->setParent(this);
}
