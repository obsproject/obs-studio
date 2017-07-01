#include "window-basic-auto-config.hpp"
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"
#include "obs-app.hpp"

#include <QMessageBox>
#include <QScreen>

#include <obs.hpp>

#include "ui_AutoConfigStartPage.h"
#include "ui_AutoConfigVideoPage.h"
#include "ui_AutoConfigStreamPage.h"

#define wiz reinterpret_cast<AutoConfig*>(wizard())

/* ------------------------------------------------------------------------- */

#define SERVICE_PATH "service.json"

static OBSData OpenServiceSettings(std::string &type)
{
	char serviceJsonPath[512];
	int ret = GetProfilePath(serviceJsonPath, sizeof(serviceJsonPath),
			SERVICE_PATH);
	if (ret <= 0)
		return OBSData();

	OBSData data = obs_data_create_from_json_file_safe(serviceJsonPath,
			"bak");
	obs_data_release(data);

	obs_data_set_default_string(data, "type", "rtmp_common");
	type = obs_data_get_string(data, "type");

	OBSData settings = obs_data_get_obj(data, "settings");
	obs_data_release(settings);

	return settings;
}

static void GetServiceInfo(std::string &type, std::string &service,
		std::string &server, std::string &key)
{
	OBSData settings = OpenServiceSettings(type);

	service = obs_data_get_string(settings, "service");
	server = obs_data_get_string(settings, "server");
	key = obs_data_get_string(settings, "key");
}

/* ------------------------------------------------------------------------- */

AutoConfigStartPage::AutoConfigStartPage(QWidget *parent)
	: QWizardPage (parent),
	  ui          (new Ui_AutoConfigStartPage)
{
	ui->setupUi(this);
	setTitle(QTStr("Basic.AutoConfig.StartPage"));
	setSubTitle(QTStr("Basic.AutoConfig.StartPage.SubTitle"));
}

AutoConfigStartPage::~AutoConfigStartPage()
{
	delete ui;
}

int AutoConfigStartPage::nextId() const
{
	return AutoConfig::VideoPage;
}

void AutoConfigStartPage::on_prioritizeStreaming_clicked()
{
	wiz->type = AutoConfig::Type::Streaming;
}

void AutoConfigStartPage::on_prioritizeRecording_clicked()
{
	wiz->type = AutoConfig::Type::Recording;
}

/* ------------------------------------------------------------------------- */

#define RES_TEXT(x)             "Basic.AutoConfig.VideoPage." x
#define RES_USE_CURRENT         RES_TEXT("BaseResolution.UseCurrent")
#define RES_USE_DISPLAY         RES_TEXT("BaseResolution.Display")
#define FPS_USE_CURRENT         RES_TEXT("FPS.UseCurrent")
#define FPS_PREFER_HIGH_FPS     RES_TEXT("FPS.PreferHighFPS")
#define FPS_PREFER_HIGH_RES     RES_TEXT("FPS.PreferHighRes")

AutoConfigVideoPage::AutoConfigVideoPage(QWidget *parent)
	: QWizardPage (parent),
	  ui          (new Ui_AutoConfigVideoPage)
{
	ui->setupUi(this);

	setTitle(QTStr("Basic.AutoConfig.VideoPage"));
	setSubTitle(QTStr("Basic.AutoConfig.VideoPage.SubTitle"));

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	long double fpsVal =
		(long double)ovi.fps_num / (long double)ovi.fps_den;

	QString fpsStr = (ovi.fps_den > 1)
		? QString::number(fpsVal, 'f', 2)
		: QString::number(fpsVal, 'g', 2);

	ui->fps->addItem(QTStr(FPS_PREFER_HIGH_FPS),
			(int)AutoConfig::FPSType::PreferHighFPS);
	ui->fps->addItem(QTStr(FPS_PREFER_HIGH_RES),
			(int)AutoConfig::FPSType::PreferHighRes);
	ui->fps->addItem(QTStr(FPS_USE_CURRENT).arg(fpsStr),
			(int)AutoConfig::FPSType::UseCurrent);
	ui->fps->addItem(QStringLiteral("30"), (int)AutoConfig::FPSType::fps30);
	ui->fps->addItem(QStringLiteral("60"), (int)AutoConfig::FPSType::fps60);
	ui->fps->setCurrentIndex(0);

	QString cxStr = QString::number(ovi.base_width);
	QString cyStr = QString::number(ovi.base_height);

	int encRes = int(ovi.base_width << 16) | int(ovi.base_height);
	ui->canvasRes->addItem(QTStr(RES_USE_CURRENT).arg(cxStr, cyStr),
			(int)encRes);

	QList<QScreen*> screens = QGuiApplication::screens();
	for (int i = 0; i < screens.size(); i++) {
		QScreen *screen = screens[i];
		QSize as = screen->size();

		encRes = int(as.width() << 16) | int(as.height());

		QString str = QTStr(RES_USE_DISPLAY)
			.arg(QString::number(i + 1),
			     QString::number(as.width()),
			     QString::number(as.height()));

		ui->canvasRes->addItem(str, encRes);
	}

	auto addRes = [&] (int cx, int cy)
	{
		encRes = (cx << 16) | cy;
		QString str = QString("%1x%2").arg(
				QString::number(cx),
				QString::number(cy));
		ui->canvasRes->addItem(str, encRes);
	};

	addRes(1920, 1080);
	addRes(1280, 720);

	ui->canvasRes->setCurrentIndex(0);
}

AutoConfigVideoPage::~AutoConfigVideoPage()
{
	delete ui;
}

int AutoConfigVideoPage::nextId() const
{
	return wiz->type == AutoConfig::Type::Recording
		? AutoConfig::TestPage
		: AutoConfig::StreamPage;
}

bool AutoConfigVideoPage::validatePage()
{
	int encRes = ui->canvasRes->currentData().toInt();
	wiz->baseResolutionCX = encRes >> 16;
	wiz->baseResolutionCY = encRes & 0xFFFF;
	wiz->fpsType = (AutoConfig::FPSType)ui->fps->currentData().toInt();

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	switch (wiz->fpsType) {
	case AutoConfig::FPSType::PreferHighFPS:
		wiz->specificFPSNum = 0;
		wiz->specificFPSDen = 0;
		wiz->preferHighFPS = true;
		break;
	case AutoConfig::FPSType::PreferHighRes:
		wiz->specificFPSNum = 0;
		wiz->specificFPSDen = 0;
		wiz->preferHighFPS = false;
		break;
	case AutoConfig::FPSType::UseCurrent:
		wiz->specificFPSNum = ovi.fps_num;
		wiz->specificFPSDen = ovi.fps_den;
		wiz->preferHighFPS = false;
		break;
	case AutoConfig::FPSType::fps30:
		wiz->specificFPSNum = 30;
		wiz->specificFPSDen = 1;
		wiz->preferHighFPS = false;
		break;
	case AutoConfig::FPSType::fps60:
		wiz->specificFPSNum = 60;
		wiz->specificFPSDen = 1;
		wiz->preferHighFPS = false;
		break;
	}

	return true;
}

/* ------------------------------------------------------------------------- */

AutoConfigStreamPage::AutoConfigStreamPage(QWidget *parent)
	: QWizardPage (parent),
	  ui          (new Ui_AutoConfigStreamPage)
{
	ui->setupUi(this);
	ui->bitrateLabel->setVisible(false);
	ui->bitrate->setVisible(false);

	ui->streamType->addItem(obs_service_get_display_name("rtmp_common"));
	ui->streamType->addItem(obs_service_get_display_name("rtmp_custom"));

	setTitle(QTStr("Basic.AutoConfig.StreamPage"));
	setSubTitle(QTStr("Basic.AutoConfig.StreamPage.SubTitle"));

	LoadServices(false);

	connect(ui->streamType, SIGNAL(currentIndexChanged(int)),
			this, SLOT(ServiceChanged()));
	connect(ui->service, SIGNAL(currentIndexChanged(int)),
			this, SLOT(ServiceChanged()));
	connect(ui->customServer, SIGNAL(textChanged(const QString &)),
			this, SLOT(ServiceChanged()));
	connect(ui->doBandwidthTest, SIGNAL(toggled(bool)),
			this, SLOT(ServiceChanged()));

	connect(ui->service, SIGNAL(currentIndexChanged(int)),
			this, SLOT(UpdateServerList()));

	connect(ui->streamType, SIGNAL(currentIndexChanged(int)),
			this, SLOT(UpdateKeyLink()));
	connect(ui->service, SIGNAL(currentIndexChanged(int)),
			this, SLOT(UpdateKeyLink()));

	connect(ui->key, SIGNAL(textChanged(const QString &)),
			this, SLOT(UpdateCompleted()));
	connect(ui->regionUS, SIGNAL(toggled(bool)),
			this, SLOT(UpdateCompleted()));
	connect(ui->regionEU, SIGNAL(toggled(bool)),
			this, SLOT(UpdateCompleted()));
	connect(ui->regionAsia, SIGNAL(toggled(bool)),
			this, SLOT(UpdateCompleted()));
	connect(ui->regionOther, SIGNAL(toggled(bool)),
			this, SLOT(UpdateCompleted()));
}

AutoConfigStreamPage::~AutoConfigStreamPage()
{
	delete ui;
}

bool AutoConfigStreamPage::isComplete() const
{
	return ready;
}

int AutoConfigStreamPage::nextId() const
{
	return AutoConfig::TestPage;
}

bool AutoConfigStreamPage::validatePage()
{
	OBSData service_settings = obs_data_create();
	obs_data_release(service_settings);

	wiz->customServer = ui->streamType->currentIndex() == 1;

	const char *serverType = wiz->customServer
		? "rtmp_custom"
		: "rtmp_common";

	if (!wiz->customServer) {
		obs_data_set_string(service_settings, "service",
				QT_TO_UTF8(ui->service->currentText()));
	}

	OBSService service = obs_service_create(serverType, "temp_service",
			service_settings, nullptr);
	obs_service_release(service);

	int bitrate = 10000;
	if (!ui->doBandwidthTest->isChecked()) {
		bitrate = ui->bitrate->value();
		wiz->idealBitrate = bitrate;
	}

	OBSData settings = obs_data_create();
	obs_data_release(settings);
	obs_data_set_int(settings, "bitrate", bitrate);
	obs_service_apply_encoder_settings(service, settings, nullptr);

	if (wiz->customServer) {
		QString server = ui->customServer->text();
		wiz->server = wiz->serverName = QT_TO_UTF8(server);
	} else {
		wiz->serverName = QT_TO_UTF8(ui->server->currentText());
		wiz->server = QT_TO_UTF8(ui->server->currentData().toString());
	}

	wiz->bandwidthTest = ui->doBandwidthTest->isChecked();
	wiz->startingBitrate = (int)obs_data_get_int(settings, "bitrate");
	wiz->idealBitrate = wiz->startingBitrate;
	wiz->regionUS = ui->regionUS->isChecked();
	wiz->regionEU = ui->regionEU->isChecked();
	wiz->regionAsia = ui->regionAsia->isChecked();
	wiz->regionOther = ui->regionOther->isChecked();
	wiz->serviceName = QT_TO_UTF8(ui->service->currentText());
	if (ui->preferHardware)
		wiz->preferHardware = ui->preferHardware->isChecked();
	wiz->key = QT_TO_UTF8(ui->key->text());

	if (!wiz->customServer) {
		if (wiz->serviceName == "Twitch")
			wiz->service = AutoConfig::Service::Twitch;
		else if (wiz->serviceName == "hitbox.tv")
			wiz->service = AutoConfig::Service::Hitbox;
		else if (wiz->serviceName == "beam.pro")
			wiz->service = AutoConfig::Service::Beam;
		else
			wiz->service = AutoConfig::Service::Other;
	} else {
		wiz->service = AutoConfig::Service::Other;
	}

	if (wiz->service != AutoConfig::Service::Twitch && wiz->bandwidthTest) {
		QMessageBox::StandardButton button;
#define WARNING_TEXT(x) QTStr("Basic.AutoConfig.StreamPage.StreamWarning." x)
		button = OBSMessageBox::question(this,
				WARNING_TEXT("Title"),
				WARNING_TEXT("Text"));
#undef WARNING_TEXT

		if (button == QMessageBox::No)
			return false;
	}

	return true;
}

void AutoConfigStreamPage::on_show_clicked()
{
	if (ui->key->echoMode() == QLineEdit::Password) {
		ui->key->setEchoMode(QLineEdit::Normal);
		ui->show->setText(QTStr("Hide"));
	} else {
		ui->key->setEchoMode(QLineEdit::Password);
		ui->show->setText(QTStr("Show"));
	}
}

void AutoConfigStreamPage::ServiceChanged()
{
	bool showMore = ui->service->currentData().toBool();
	if (showMore)
		return;

	std::string service = QT_TO_UTF8(ui->service->currentText());
	bool regionBased = service == "Twitch" ||
	                   service == "hitbox.tv" ||
	                   service == "beam.pro";
	bool testBandwidth = ui->doBandwidthTest->isChecked();
	bool custom = ui->streamType->currentIndex() == 1;

	ui->service->setVisible(!custom);
	ui->serviceLabel->setVisible(!custom);

	ui->formLayout->removeWidget(ui->serviceLabel);
	ui->formLayout->removeWidget(ui->service);

	ui->formLayout->removeWidget(ui->serverLabel);
	ui->formLayout->removeWidget(ui->serverStackedWidget);

	if (custom) {
		ui->formLayout->insertRow(1, ui->serverLabel,
				ui->serverStackedWidget);

		ui->region->setVisible(false);
		ui->serverStackedWidget->setCurrentIndex(1);
		ui->serverStackedWidget->setVisible(true);
		ui->serverLabel->setVisible(true);
	} else {
		ui->formLayout->insertRow(1, ui->serviceLabel, ui->service);

		if (!testBandwidth)
			ui->formLayout->insertRow(2, ui->serverLabel,
					ui->serverStackedWidget);

		ui->region->setVisible(regionBased && testBandwidth);
		ui->serverStackedWidget->setCurrentIndex(0);
		ui->serverStackedWidget->setHidden(testBandwidth);
		ui->serverLabel->setHidden(testBandwidth);
	}

	wiz->testRegions = regionBased && testBandwidth;

	ui->bitrateLabel->setHidden(testBandwidth);
	ui->bitrate->setHidden(testBandwidth);

	UpdateCompleted();
}

void AutoConfigStreamPage::UpdateKeyLink()
{
	bool custom = ui->streamType->currentIndex() == 1;
	QString serviceName = ui->service->currentText();

	if (custom)
		serviceName = "";

	QString text = QTStr("Basic.AutoConfig.StreamPage.StreamKey");
	if (serviceName == "Twitch") {
		text += " <a href=\"https://";
		text += "www.twitch.tv/broadcast/dashboard/streamkey";
		text += "\">";
		text += QTStr("Basic.AutoConfig.StreamPage.StreamKey.LinkToSite");
		text += "</a>";
	} else if (serviceName == "YouTube / YouTube Gaming") {
		text += " <a href=\"https://";
		text += "www.youtube.com/live_dashboard";
		text += "\">";
		text += QTStr("Basic.AutoConfig.StreamPage.StreamKey.LinkToSite");
		text += "</a>";
	}

	ui->streamKeyLabel->setText(text);
}

void AutoConfigStreamPage::LoadServices(bool showAll)
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

	if (!lastService.isEmpty()) {
		int idx = ui->service->findText(lastService);
		if (idx != -1)
			ui->service->setCurrentIndex(idx);
	}

	if (!showAll) {
		ui->service->addItem(
			QTStr("Basic.AutoConfig.StreamPage.Service.ShowAll"),
			QVariant(true));
	}

	obs_properties_destroy(props);

	ui->service->blockSignals(false);
}

void AutoConfigStreamPage::UpdateServerList()
{
	QString serviceName = ui->service->currentText();
	bool showMore = ui->service->currentData().toBool();

	if (showMore) {
		LoadServices(true);
		ui->service->showPopup();
		return;
	} else {
		lastService = serviceName;
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

void AutoConfigStreamPage::UpdateCompleted()
{
	if (ui->key->text().isEmpty()) {
		ready = false;
	} else {
		bool custom = ui->streamType->currentIndex() == 1;
		if (custom) {
			ready = !ui->customServer->text().isEmpty();
		} else {
			ready = !wiz->testRegions ||
				ui->regionUS->isChecked() ||
				ui->regionEU->isChecked() ||
				ui->regionAsia->isChecked() ||
				ui->regionOther->isChecked();
		}
	}
	emit completeChanged();
}

/* ------------------------------------------------------------------------- */

AutoConfig::AutoConfig(QWidget *parent)
	: QWizard(parent)
{
	OBSBasic *main = reinterpret_cast<OBSBasic*>(parent);
	main->EnableOutputs(false);

	installEventFilter(CreateShortcutFilter());

	std::string serviceType;
	GetServiceInfo(serviceType, serviceName, server, key);
#ifdef _WIN32
	setWizardStyle(QWizard::ModernStyle);
#endif
	AutoConfigStreamPage *streamPage = new AutoConfigStreamPage();

	setPage(StartPage, new AutoConfigStartPage());
	setPage(VideoPage, new AutoConfigVideoPage());
	setPage(StreamPage, streamPage);
	setPage(TestPage, new AutoConfigTestPage());
	setWindowTitle(QTStr("Basic.AutoConfig.Beta"));

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	baseResolutionCX = ovi.base_width;
	baseResolutionCY = ovi.base_height;

	/* ----------------------------------------- */
	/* load service/servers                      */

	customServer = serviceType == "rtmp_custom";

	QComboBox *serviceList = streamPage->ui->service;

	if (!serviceName.empty()) {
		serviceList->blockSignals(true);

		int count = serviceList->count();
		bool found = false;

		for (int i = 0; i < count; i++) {
			QString name = serviceList->itemText(i);

			if (name == serviceName.c_str()) {
				serviceList->setCurrentIndex(i);
				found = true;
				break;
			}
		}

		if (!found) {
			serviceList->insertItem(0, serviceName.c_str());
			serviceList->setCurrentIndex(0);
		}

		serviceList->blockSignals(false);
	}

	streamPage->UpdateServerList();
	streamPage->UpdateKeyLink();

	if (!customServer) {
		QComboBox *serverList = streamPage->ui->server;
		int idx = serverList->findData(QString(server.c_str()));
		if (idx == -1)
			idx = 0;

		serverList->setCurrentIndex(idx);
	} else {
		streamPage->ui->customServer->setText(server.c_str());
		streamPage->ui->streamType->setCurrentIndex(1);
	}

	if (!key.empty())
		streamPage->ui->key->setText(key.c_str());

	int bitrate = config_get_int(main->Config(), "SimpleOutput", "VBitrate");
	streamPage->ui->bitrate->setValue(bitrate);
	streamPage->ServiceChanged();

	streamPage->ui->preferHardware->setChecked(os_get_physical_cores() <= 4);

	TestHardwareEncoding();
	if (!hardwareEncodingAvailable) {
		delete streamPage->ui->preferHardware;
		streamPage->ui->preferHardware = nullptr;
	}

	setOptions(0);
	setButtonText(QWizard::FinishButton,
			QTStr("Basic.AutoConfig.ApplySettings"));
	setButtonText(QWizard::BackButton, QTStr("Back"));
	setButtonText(QWizard::NextButton, QTStr("Next"));
	setButtonText(QWizard::CancelButton, QTStr("Cancel"));
}

AutoConfig::~AutoConfig()
{
	OBSBasic *main = reinterpret_cast<OBSBasic*>(App()->GetMainWindow());
	main->EnableOutputs(true);
}

void AutoConfig::TestHardwareEncoding()
{
	size_t idx = 0;
	const char *id;
	while (obs_enum_encoder_types(idx++, &id)) {
		if (strcmp(id, "ffmpeg_nvenc") == 0)
			hardwareEncodingAvailable = nvencAvailable = true;
		else if (strcmp(id, "obs_qsv11") == 0)
			hardwareEncodingAvailable = qsvAvailable = true;
		else if (strcmp(id, "amd_amf_h264") == 0)
			hardwareEncodingAvailable = vceAvailable = true;
	}
}

bool AutoConfig::CanTestServer(const char *server)
{
	if (!testRegions || (regionUS && regionEU && regionAsia && regionOther))
		return true;

	if (service == Service::Twitch) {
		if (astrcmp_n(server, "US West:", 8) == 0 ||
		    astrcmp_n(server, "US East:", 8) == 0 ||
		    astrcmp_n(server, "US Central:", 11) == 0) {
			return regionUS;
		} else if (astrcmp_n(server, "EU:", 3) == 0) {
			return regionEU;
		} else if (astrcmp_n(server, "Asia:", 5) == 0) {
			return regionAsia;
		} else if (regionOther) {
			return true;
		}
	} else if (service == Service::Hitbox) {
		if (strcmp(server, "Default") == 0) {
			return true;
		} else if (astrcmp_n(server, "US-West:", 8) == 0 ||
		           astrcmp_n(server, "US-East:", 8) == 0) {
			return regionUS;
		} else if (astrcmp_n(server, "EU-", 3) == 0) {
			return regionEU;
		} else if (astrcmp_n(server, "South Korea:", 12) == 0 ||
		           astrcmp_n(server, "Asia:", 5) == 0 ||
		           astrcmp_n(server, "China:", 6) == 0) {
			return regionAsia;
		} else if (regionOther) {
			return true;
		}
	} else if (service == Service::Beam) {
		if (astrcmp_n(server, "US:", 3) == 0) {
			return regionUS;
		} else if (astrcmp_n(server, "EU:", 3) == 0) {
			return regionEU;
		} else if (astrcmp_n(server, "South Korea:", 12) == 0 ||
		           astrcmp_n(server, "Asia:", 5) == 0) {
			return regionAsia;
		} else if (regionOther) {
			return true;
		}
	} else {
		return true;
	}

	return false;
}

void AutoConfig::done(int result)
{
	QWizard::done(result);

	if (result == QDialog::Accepted) {
		if (type == Type::Streaming)
			SaveStreamSettings();
		SaveSettings();
	}
}

inline const char *AutoConfig::GetEncoderId(Encoder enc)
{
	switch (enc) {
	case Encoder::NVENC:
		return SIMPLE_ENCODER_NVENC;
	case Encoder::QSV:
		return SIMPLE_ENCODER_QSV;
	case Encoder::AMD:
		return SIMPLE_ENCODER_AMD;
	default:
		return SIMPLE_ENCODER_X264;
	}
};

void AutoConfig::SaveStreamSettings()
{
	OBSBasic *main = reinterpret_cast<OBSBasic*>(App()->GetMainWindow());

	/* ---------------------------------- */
	/* save service                       */

	const char *service_id = customServer
		? "rtmp_custom"
		: "rtmp_common";

	obs_service_t *oldService = main->GetService();
	OBSData hotkeyData = obs_hotkeys_save_service(oldService);
	obs_data_release(hotkeyData);

	OBSData settings = obs_data_create();
	obs_data_release(settings);

	if (!customServer)
		obs_data_set_string(settings, "service", serviceName.c_str());
	obs_data_set_string(settings, "server", server.c_str());
	obs_data_set_string(settings, "key", key.c_str());

	OBSService newService = obs_service_create(service_id,
			"default_service", settings, hotkeyData);
	obs_service_release(newService);

	if (!newService)
		return;

	main->SetService(newService);
	main->SaveService();

	/* ---------------------------------- */
	/* save stream settings               */

	config_set_int(main->Config(), "SimpleOutput", "VBitrate",
			idealBitrate);
	config_set_string(main->Config(), "SimpleOutput", "StreamEncoder",
			GetEncoderId(streamingEncoder));
	config_remove_value(main->Config(), "SimpleOutput", "UseAdvanced");
}

void AutoConfig::SaveSettings()
{
	OBSBasic *main = reinterpret_cast<OBSBasic*>(App()->GetMainWindow());

	if (recordingEncoder != Encoder::Stream)
		config_set_string(main->Config(), "SimpleOutput", "RecEncoder",
				GetEncoderId(recordingEncoder));

	const char *quality = recordingQuality == Quality::High
		? "Small"
		: "Stream";

	config_set_string(main->Config(), "Output", "Mode", "Simple");
	config_set_string(main->Config(), "SimpleOutput", "RecQuality", quality);
	config_set_int(main->Config(), "Video", "BaseCX", baseResolutionCX);
	config_set_int(main->Config(), "Video", "BaseCY", baseResolutionCY);
	config_set_int(main->Config(), "Video", "OutputCX", idealResolutionCX);
	config_set_int(main->Config(), "Video", "OutputCY", idealResolutionCY);

	if (fpsType != FPSType::UseCurrent) {
		config_set_uint(main->Config(), "Video", "FPSType", 0);
		config_set_string(main->Config(), "Video", "FPSCommon",
				std::to_string(idealFPSNum).c_str());
	}

	main->ResetVideo();
	main->ResetOutputs();
	config_save_safe(main->Config(), "tmp", nullptr);
}
