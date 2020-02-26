#include <QMessageBox>
#include <QUrl>

#include "window-basic-settings.hpp"
#include "obs-frontend-api.h"
#include "obs-app.hpp"
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"
#include "url-push-button.hpp"

#ifdef BROWSER_AVAILABLE
#include <browser-panel.hpp>
#endif

#ifdef AUTH_ENABLED
#include "auth-oauth.hpp"
#endif

#if CAFFEINE_ENABLED
#include "auth-caffeine.hpp"
#endif

enum class ListOpt : int {
	ShowAll = 1,
	Custom,
};

enum class Section : int {
	Connect,
	StreamKey,
};

inline bool OBSBasicSettings::IsCustomService() const
{
	return ui->service->currentData().toInt() == (int)ListOpt::Custom;
}

void OBSBasicSettings::InitStreamPage()
{
	ui->connectAccount2->setVisible(false);
	ui->disconnectAccount->setVisible(false);
	ui->bandwidthTestEnable->setVisible(false);
	ui->authSignedInAs->setVisible(false);
	//ui->authSignedInAsLabel->setVisible(false);
	ui->twitchAddonDropdown->setVisible(false);
	ui->twitchAddonLabel->setVisible(false);
	ui->mixerAddonDropdown->setVisible(false);
	ui->mixerAddonLabel->setVisible(false);

	int vertSpacing = ui->topStreamLayout->verticalSpacing();

	QMargins m = ui->topStreamLayout->contentsMargins();
	m.setBottom(vertSpacing / 2);
	ui->topStreamLayout->setContentsMargins(m);

	m = ui->loginPageLayout->contentsMargins();
	m.setTop(vertSpacing / 2);
	ui->loginPageLayout->setContentsMargins(m);

	m = ui->streamkeyPageLayout->contentsMargins();
	m.setTop(vertSpacing / 2);
	ui->streamkeyPageLayout->setContentsMargins(m);

	LoadServices(false);

	ui->twitchAddonDropdown->addItem(
		QTStr("Basic.Settings.Stream.TTVAddon.None"));
	ui->twitchAddonDropdown->addItem(
		QTStr("Basic.Settings.Stream.TTVAddon.BTTV"));
	ui->twitchAddonDropdown->addItem(
		QTStr("Basic.Settings.Stream.TTVAddon.FFZ"));
	ui->twitchAddonDropdown->addItem(
		QTStr("Basic.Settings.Stream.TTVAddon.Both"));

	ui->mixerAddonDropdown->addItem(
		QTStr("Basic.Settings.Stream.MixerAddon.None"));
	ui->mixerAddonDropdown->addItem(
		QTStr("Basic.Settings.Stream.MixerAddon.MEE"));

	connect(ui->service, SIGNAL(currentIndexChanged(int)), this,
		SLOT(UpdateServerList()));
	connect(ui->service, SIGNAL(currentIndexChanged(int)), this,
		SLOT(UpdateKeyLink()));
}

void OBSBasicSettings::LoadStream1Settings()
{
	obs_service_t *service_obj = main->GetService();
	const char *type = obs_service_get_type(service_obj);

	loading = true;

	obs_data_t *settings = obs_service_get_settings(service_obj);

	const char *service = obs_data_get_string(settings, "service");
	const char *server = obs_data_get_string(settings, "server");
	const char *key = obs_data_get_string(settings, "key");
	const char *username = obs_data_get_string(settings, "username");

	if (strcmp(type, "rtmp_custom") == 0) {
		ui->service->setCurrentIndex(0);
		ui->customServer->setText(server);

		bool use_auth = obs_data_get_bool(settings, "use_auth");
		const char *password =
			obs_data_get_string(settings, "password");
		ui->authUsername->setText(QT_UTF8(username));
		ui->authPw->setText(QT_UTF8(password));
		ui->useAuth->setChecked(use_auth);
	} else {
		int idx = ui->service->findText(service);
		if (idx == -1) {
			if (service && *service) {
				// Insert placeholder for unrecognized service
				ui->service->insertItem(1, service);
				idx = 1;
			} else {
				// Default to twitch or first non-custom service
				idx = ui->service->findText("Twitch");
				if (idx == -1)
					idx = 1;
			}
		}
		ui->service->setCurrentIndex(idx);

		bool bw_test = obs_data_get_bool(settings, "bwtest");
		ui->bandwidthTestEnable->setChecked(bw_test);

		idx = config_get_int(main->Config(), "Twitch", "AddonChoice");
		ui->twitchAddonDropdown->setCurrentIndex(idx);

		idx = config_get_int(main->Config(), "Mixer", "AddonChoice");
		ui->mixerAddonDropdown->setCurrentIndex(idx);
	}

	UpdateServerList();

	if (strcmp(type, "rtmp_common") == 0) {
		int idx = ui->server->findData(server);
		if (idx == -1) {
			if (server && *server)
				ui->server->insertItem(0, server, server);
			idx = 0;
		}
		ui->server->setCurrentIndex(idx);

#if CAFFEINE_ENABLED
		if (username && username[0]) {
			lastSignedInAs = username;
			//ui->authSignedInAsLabel->setVisible(true);
			ui->authSignedInAs->setVisible(true);
			ui->authSignedInAs->setText(QT_UTF8(username));
		} else {
			//ui->authSignedInAsLabel->setVisible(false);
			ui->authSignedInAs->setVisible(false);
		}
#endif
	}

	ui->key->setText(key);

	lastService.clear();
	on_service_currentIndexChanged(0);

	obs_data_release(settings);

	UpdateKeyLink();

	bool streamActive = obs_frontend_streaming_active();
	ui->streamPage->setEnabled(!streamActive);

	loading = false;
}

void OBSBasicSettings::SaveStream1Settings()
{
	bool customServer = IsCustomService();
	const char *service_id = customServer ? "rtmp_custom" : "rtmp_common";

	obs_service_t *oldService = main->GetService();
	OBSData hotkeyData = obs_hotkeys_save_service(oldService);
	obs_data_release(hotkeyData);

	OBSData settings = obs_data_create();
	obs_data_release(settings);

	if (!customServer) {
		obs_data_set_string(settings, "service",
				    QT_TO_UTF8(ui->service->currentText()));
		obs_data_set_string(
			settings, "server",
			QT_TO_UTF8(ui->server->currentData().toString()));
#if CAFFEINE_ENABLED
		if (ui->service->currentText() == "Caffeine") {
			obs_data_set_string(settings, "username",
					    QT_TO_UTF8(lastSignedInAs));
		}
#endif
	} else {
		obs_data_set_string(settings, "server",
				    QT_TO_UTF8(ui->customServer->text()));
		obs_data_set_bool(settings, "use_auth",
				  ui->useAuth->isChecked());
		if (ui->useAuth->isChecked()) {
			obs_data_set_string(
				settings, "username",
				QT_TO_UTF8(ui->authUsername->text()));
			obs_data_set_string(settings, "password",
					    QT_TO_UTF8(ui->authPw->text()));
		}
	}

	obs_data_set_bool(settings, "bwtest",
			  ui->bandwidthTestEnable->isChecked());

	if (!!auth && strcmp(auth->service(), "Twitch") == 0) {
		bool choiceExists = config_has_user_value(
			main->Config(), "Twitch", "AddonChoice");
		int currentChoice =
			config_get_int(main->Config(), "Twitch", "AddonChoice");
		int newChoice = ui->twitchAddonDropdown->currentIndex();

		config_set_int(main->Config(), "Twitch", "AddonChoice",
			       newChoice);

		if (choiceExists && currentChoice != newChoice)
			forceAuthReload = true;
	}
	if (!!auth && strcmp(auth->service(), "Mixer") == 0) {
		bool choiceExists = config_has_user_value(
			main->Config(), "Mixer", "AddonChoice");
		int currentChoice =
			config_get_int(main->Config(), "Mixer", "AddonChoice");
		int newChoice = ui->mixerAddonDropdown->currentIndex();

		config_set_int(main->Config(), "Mixer", "AddonChoice",
			       newChoice);

		if (choiceExists && currentChoice != newChoice)
			forceAuthReload = true;
	}

	obs_data_set_string(settings, "key", QT_TO_UTF8(ui->key->text()));

	OBSService newService = obs_service_create(
		service_id, "default_service", settings, hotkeyData);
	obs_service_release(newService);

	if (!newService)
		return;

	main->SetService(newService);
	main->SaveService();
	main->auth = auth;
	if (!!main->auth)
		main->auth->LoadUI();
}

void OBSBasicSettings::UpdateKeyLink()
{
	if (IsCustomService()) {
		ui->getStreamKeyButton->hide();
		return;
	}

	QString serviceName = ui->service->currentText();
	QString streamKeyLink;
	if (serviceName == "Twitch") {
		streamKeyLink =
			"https://www.twitch.tv/broadcast/dashboard/streamkey";
	} else if (serviceName == "YouTube / YouTube Gaming") {
		streamKeyLink = "https://www.youtube.com/live_dashboard";
	} else if (serviceName.startsWith("Restream.io")) {
		streamKeyLink =
			"https://restream.io/settings/streaming-setup?from=OBS";
	} else if (serviceName == "Facebook Live") {
		streamKeyLink = "https://www.facebook.com/live/create?ref=OBS";
	} else if (serviceName.startsWith("Twitter")) {
		streamKeyLink = "https://www.pscp.tv/account/producer";
	} else if (serviceName.startsWith("YouStreamer")) {
		streamKeyLink = "https://app.youstreamer.com/stream/";
	}

	if (QString(streamKeyLink).isNull()) {
		ui->getStreamKeyButton->hide();
	} else {
		ui->getStreamKeyButton->setTargetUrl(QUrl(streamKeyLink));
		ui->getStreamKeyButton->show();
	}
}

void OBSBasicSettings::LoadServices(bool showAll)
{
	obs_properties_t *props = obs_get_service_properties("rtmp_common");

	OBSData settings = obs_data_create();
	obs_data_release(settings);

	obs_data_set_bool(settings, "show_all", showAll);

	obs_property_t *prop = obs_properties_get(props, "show_all");
	obs_property_modified(prop, settings);

	ui->service->blockSignals(true);
	ui->service->clear();

	QStringList names;

	obs_property_t *services = obs_properties_get(props, "service");
	size_t services_count = obs_property_list_item_count(services);
	for (size_t i = 0; i < services_count; i++) {
		const char *name = obs_property_list_item_string(services, i);
		names.push_back(name);
	}

	if (showAll)
		names.sort();

	for (QString &name : names)
		ui->service->addItem(name);

	if (!showAll) {
		ui->service->addItem(
			QTStr("Basic.AutoConfig.StreamPage.Service.ShowAll"),
			QVariant((int)ListOpt::ShowAll));
	}

	ui->service->insertItem(
		0, QTStr("Basic.AutoConfig.StreamPage.Service.Custom"),
		QVariant((int)ListOpt::Custom));

	if (!lastService.isEmpty()) {
		int idx = ui->service->findText(lastService);
		if (idx != -1)
			ui->service->setCurrentIndex(idx);
	}

	obs_properties_destroy(props);

	ui->service->blockSignals(false);
}

void OBSBasicSettings::on_service_currentIndexChanged(int)
{
	bool showMore = ui->service->currentData().toInt() ==
			(int)ListOpt::ShowAll;
	if (showMore)
		return;

	std::string service = QT_TO_UTF8(ui->service->currentText());
	bool custom = IsCustomService();

	ui->disconnectAccount->setVisible(false);
	ui->bandwidthTestEnable->setVisible(false);
	ui->twitchAddonDropdown->setVisible(false);
	ui->twitchAddonLabel->setVisible(false);
	ui->mixerAddonDropdown->setVisible(false);
	ui->mixerAddonLabel->setVisible(false);

#ifdef AUTH_ENABLED
	auth.reset();

	if (!!main->auth &&
	    service.find(main->auth->service()) != std::string::npos) {
		auth = main->auth;
		OnAuthConnected();
	}

	bool can_auth = Auth::CanAuthService(service);
	bool hidden_key = Auth::IsKeyHidden(service);
	QString connectString =
		QTStr("Basic.AutoConfig.StreamPage.ConnectAccount")
			.arg(hidden_key ? QTStr("Required")
					: QTStr("Optional"));
	ui->connectAccount->setText(connectString);
	ui->connectAccount2->setText(connectString);

	if (lastService.isEmpty()) {
		lastService = service.c_str();
		lastServiceKey = ui->key->text();
	} else if (lastService != service.c_str()) {
		// Don't show the stream key from the previous service
		ui->key->clear();
	} else {
		ui->key->setText(lastServiceKey);
	}
	QString key = ui->key->text();
	bool authenticated = !key.isEmpty();
	int page = can_auth && (!loading || key.isEmpty())
			   ? (int)Section::Connect
			   : (int)Section::StreamKey;
	if (hidden_key)
		page = (int)Section::StreamKey;
	ui->useStreamKey->setVisible(!hidden_key);
	ui->streamStackWidget->setCurrentIndex(page);
	ui->streamKeyWidget->setVisible(!hidden_key);
	ui->streamKeyLabel->setVisible(!hidden_key);
#if CAFFEINE_ENABLED
	bool isCaffeine = service == "Caffeine";
	ui->authSignedInAs->setVisible(authenticated && isCaffeine);
	ui->authSignedInAs->setText(lastSignedInAs);
#endif
	ui->connectAccount->setVisible(can_auth && !authenticated);
	ui->disconnectAccount->setVisible(can_auth && authenticated);
	ui->connectAccount2->setVisible(can_auth && !authenticated);
#else
	ui->connectAccount2->setVisible(false);
#endif

	ui->useAuth->setVisible(custom);
	ui->authUsernameLabel->setVisible(custom);
	ui->authUsername->setVisible(custom);
	ui->authPwLabel->setVisible(custom);
	ui->authPwWidget->setVisible(custom);

	if (custom) {
		ui->streamkeyPageLayout->insertRow(1, ui->serverLabel,
						   ui->serverStackedWidget);

		ui->serverStackedWidget->setCurrentIndex(1);
		ui->serverStackedWidget->setVisible(true);
		ui->serverLabel->setVisible(true);
		on_useAuth_toggled();
	} else {
		ui->serverStackedWidget->setCurrentIndex(0);
	}

	update();
}

void OBSBasicSettings::UpdateServerList()
{
	QString serviceName = ui->service->currentText();
	bool showMore = ui->service->currentData().toInt() ==
			(int)ListOpt::ShowAll;

	if (showMore) {
		LoadServices(true);
		ui->service->showPopup();
		return;
	}

	obs_properties_t *props = obs_get_service_properties("rtmp_common");
	obs_property_t *services = obs_properties_get(props, "service");

	OBSData settings = obs_data_create();
	obs_data_release(settings);

	obs_data_set_string(settings, "service", QT_TO_UTF8(serviceName));
	obs_property_modified(services, settings);

	obs_property_t *servers = obs_properties_get(props, "server");

	ui->server->clear();

	size_t servers_count = obs_property_list_item_count(servers);
	for (size_t i = 0; i < servers_count; i++) {
		const char *name = obs_property_list_item_name(servers, i);
		const char *server = obs_property_list_item_string(servers, i);
		ui->server->addItem(name, server);
	}

	obs_properties_destroy(props);
}

void OBSBasicSettings::on_show_clicked()
{
	if (ui->key->echoMode() == QLineEdit::Password) {
		ui->key->setEchoMode(QLineEdit::Normal);
		ui->show->setText(QTStr("Hide"));
	} else {
		ui->key->setEchoMode(QLineEdit::Password);
		ui->show->setText(QTStr("Show"));
	}
}

void OBSBasicSettings::on_authPwShow_clicked()
{
	if (ui->authPw->echoMode() == QLineEdit::Password) {
		ui->authPw->setEchoMode(QLineEdit::Normal);
		ui->authPwShow->setText(QTStr("Hide"));
	} else {
		ui->authPw->setEchoMode(QLineEdit::Password);
		ui->authPwShow->setText(QTStr("Show"));
	}
}

OBSService OBSBasicSettings::SpawnTempService()
{
	bool custom = IsCustomService();
	const char *service_id = custom ? "rtmp_custom" : "rtmp_common";

	OBSData settings = obs_data_create();
	obs_data_release(settings);

	if (!custom) {
		obs_data_set_string(settings, "service",
				    QT_TO_UTF8(ui->service->currentText()));
		obs_data_set_string(
			settings, "server",
			QT_TO_UTF8(ui->server->currentData().toString()));
	} else {
		obs_data_set_string(settings, "server",
				    QT_TO_UTF8(ui->customServer->text()));
	}
	obs_data_set_string(settings, "key", QT_TO_UTF8(ui->key->text()));

	OBSService newService = obs_service_create(service_id, "temp_service",
						   settings, nullptr);
	obs_service_release(newService);

	return newService;
}

void OBSBasicSettings::OnOAuthStreamKeyConnected()
{
#ifdef AUTH_ENABLED
	OAuthStreamKey *a = reinterpret_cast<OAuthStreamKey *>(auth.get());

	if (a) {
		bool validKey = !a->key().empty();

		if (validKey)
			ui->key->setText(QT_UTF8(a->key().c_str()));

		lastService = a->service();
		lastServiceKey = a->key().c_str();
		ui->streamKeyWidget->setVisible(false);
		ui->streamKeyLabel->setVisible(false);
		ui->connectAccount->setVisible(false);
		ui->connectAccount2->setVisible(false);
		ui->disconnectAccount->setVisible(true);

#if CAFFEINE_ENABLED
		if (std::string(a->service()) == "Caffeine") {
			auto caffeine = dynamic_cast<CaffeineAuth *>(a);
			lastSignedInAs = caffeine->GetUsername().c_str();
			//ui->authSignedInAsLabel->setVisible(true);
			ui->authSignedInAs->setVisible(true);
			ui->authSignedInAs->setText(lastSignedInAs);
		}
#endif

		if (strcmp(a->service(), "Twitch") == 0) {
			ui->bandwidthTestEnable->setVisible(true);
			ui->twitchAddonLabel->setVisible(true);
			ui->twitchAddonDropdown->setVisible(true);
		}
		if (strcmp(a->service(), "Mixer") == 0) {
			ui->mixerAddonLabel->setVisible(true);
			ui->mixerAddonDropdown->setVisible(true);
		}
	}

	ui->streamStackWidget->setCurrentIndex((int)Section::StreamKey);
	update();
#endif
}

void OBSBasicSettings::OnAuthConnected()
{
	std::string service = QT_TO_UTF8(ui->service->currentText());
	Auth::Type type = Auth::AuthType(service);

	if (type != Auth::Type::None) {
		OnOAuthStreamKeyConnected();
	}

	if (!loading) {
		stream1Changed = true;
		EnableApplyButton(true);
	}
}

void OBSBasicSettings::on_connectAccount_clicked()
{
#ifdef AUTH_ENABLED
	std::string service = QT_TO_UTF8(ui->service->currentText());

	OAuth::DeleteCookies(service);

	auth = OAuthStreamKey::Login(this, service);
	if (!!auth)
		OnAuthConnected();
#endif
}

#define DISCONNECT_COMFIRM_TITLE \
	"Basic.AutoConfig.StreamPage.DisconnectAccount.Confirm.Title"
#define DISCONNECT_COMFIRM_TEXT \
	"Basic.AutoConfig.StreamPage.DisconnectAccount.Confirm.Text"

void OBSBasicSettings::on_disconnectAccount_clicked()
{
	QMessageBox::StandardButton button;

	button = OBSMessageBox::question(this, QTStr(DISCONNECT_COMFIRM_TITLE),
					 QTStr(DISCONNECT_COMFIRM_TEXT));

	if (button == QMessageBox::No) {
		return;
	}

	// Remove the auth here if caffeine account is not associated
#if !CAFFEINE_ENABLED
	main->auth.reset();
	auth.reset();
#endif

	std::string service = QT_TO_UTF8(ui->service->currentText());

	bool hidden_key = false;
#ifdef AUTH_ENABLED
	OAuth::DeleteCookies(service);
	hidden_key = Auth::IsKeyHidden(service);
#endif
	ui->useStreamKey->setVisible(!hidden_key);
	ui->streamKeyWidget->setVisible(!hidden_key);
	ui->streamKeyLabel->setVisible(!hidden_key);
	ui->connectAccount2->setVisible(true);
	ui->connectAccount->setVisible(true);
	ui->disconnectAccount->setVisible(false);
	ui->bandwidthTestEnable->setVisible(false);
	ui->authSignedInAs->setVisible(false);
	//ui->authSignedInAsLabel->setVisible(false);#
	ui->twitchAddonDropdown->setVisible(false);
	ui->twitchAddonLabel->setVisible(false);
	ui->key->setText("");
}

void OBSBasicSettings::on_useStreamKey_clicked()
{
	ui->streamStackWidget->setCurrentIndex((int)Section::StreamKey);
}

void OBSBasicSettings::on_useAuth_toggled()
{
	if (!IsCustomService())
		return;

	bool use_auth = ui->useAuth->isChecked();

	ui->authUsernameLabel->setVisible(use_auth);
	ui->authUsername->setVisible(use_auth);
	ui->authPwLabel->setVisible(use_auth);
	ui->authPwWidget->setVisible(use_auth);
}
