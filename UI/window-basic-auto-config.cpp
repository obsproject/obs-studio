#include <QMessageBox>
#include <QScreen>
#include <QStandardItemModel>

#include <obs.hpp>

#include "window-basic-auto-config.hpp"
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"
#include "obs-app.hpp"
#include "url-push-button.hpp"

#include "service-sort-filter.hpp"

#include "ui_AutoConfigStartPage.h"
#include "ui_AutoConfigVideoPage.h"
#include "ui_AutoConfigStreamPage.h"

#include "ui-config.h"

#define wiz reinterpret_cast<AutoConfig *>(wizard())

/* ------------------------------------------------------------------------- */

AutoConfigStartPage::AutoConfigStartPage(QWidget *parent)
	: QWizardPage(parent),
	  ui(new Ui_AutoConfigStartPage)
{
	ui->setupUi(this);
	setTitle(QTStr("Basic.AutoConfig.StartPage"));
	setSubTitle(QTStr("Basic.AutoConfig.StartPage.SubTitle"));

	OBSBasic *main = OBSBasic::Get();
	if (main->VCamEnabled()) {
		QRadioButton *prioritizeVCam = new QRadioButton(
			QTStr("Basic.AutoConfig.StartPage.PrioritizeVirtualCam"),
			this);
		QBoxLayout *box = reinterpret_cast<QBoxLayout *>(layout());
		box->insertWidget(2, prioritizeVCam);

		connect(prioritizeVCam, &QPushButton::clicked, this,
			&AutoConfigStartPage::PrioritizeVCam);
	}
}

AutoConfigStartPage::~AutoConfigStartPage() {}

int AutoConfigStartPage::nextId() const
{
	return wiz->type == AutoConfig::Type::VirtualCam
		       ? AutoConfig::TestPage
		       : AutoConfig::VideoPage;
}

void AutoConfigStartPage::on_prioritizeStreaming_clicked()
{
	wiz->type = AutoConfig::Type::Streaming;
}

void AutoConfigStartPage::on_prioritizeRecording_clicked()
{
	wiz->type = AutoConfig::Type::Recording;
}

void AutoConfigStartPage::PrioritizeVCam()
{
	wiz->type = AutoConfig::Type::VirtualCam;
}

/* ------------------------------------------------------------------------- */

#define RES_TEXT(x) "Basic.AutoConfig.VideoPage." x
#define RES_USE_CURRENT RES_TEXT("BaseResolution.UseCurrent")
#define RES_USE_DISPLAY RES_TEXT("BaseResolution.Display")
#define FPS_USE_CURRENT RES_TEXT("FPS.UseCurrent")
#define FPS_PREFER_HIGH_FPS RES_TEXT("FPS.PreferHighFPS")
#define FPS_PREFER_HIGH_RES RES_TEXT("FPS.PreferHighRes")

AutoConfigVideoPage::AutoConfigVideoPage(QWidget *parent)
	: QWizardPage(parent),
	  ui(new Ui_AutoConfigVideoPage)
{
	ui->setupUi(this);

	setTitle(QTStr("Basic.AutoConfig.VideoPage"));
	setSubTitle(QTStr("Basic.AutoConfig.VideoPage.SubTitle"));

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	long double fpsVal =
		(long double)ovi.fps_num / (long double)ovi.fps_den;

	QString fpsStr = (ovi.fps_den > 1) ? QString::number(fpsVal, 'f', 2)
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

	// Auto config only supports testing down to 240p, don't allow current
	// resolution if it's lower than that.
	if (ovi.base_height >= 240)
		ui->canvasRes->addItem(QTStr(RES_USE_CURRENT).arg(cxStr, cyStr),
				       (int)encRes);

	QList<QScreen *> screens = QGuiApplication::screens();
	for (int i = 0; i < screens.size(); i++) {
		QScreen *screen = screens[i];
		QSize as = screen->size();
		int as_width = as.width();
		int as_height = as.height();

		// Calculate physical screen resolution based on the virtual screen resolution
		// They might differ if scaling is enabled, e.g. for HiDPI screens
		as_width = round(as_width * screen->devicePixelRatio());
		as_height = round(as_height * screen->devicePixelRatio());

		encRes = as_width << 16 | as_height;

		QString str = QTStr(RES_USE_DISPLAY)
				      .arg(QString::number(i + 1),
					   QString::number(as_width),
					   QString::number(as_height));

		ui->canvasRes->addItem(str, encRes);
	}

	auto addRes = [&](int cx, int cy) {
		encRes = (cx << 16) | cy;
		QString str = QString("%1x%2").arg(QString::number(cx),
						   QString::number(cy));
		ui->canvasRes->addItem(str, encRes);
	};

	addRes(1920, 1080);
	addRes(1280, 720);

	ui->canvasRes->setCurrentIndex(0);
}

AutoConfigVideoPage::~AutoConfigVideoPage() {}

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

enum class ListOpt : int {
	ShowAll = 1,
	Custom,
};

constexpr const char *TEMP_SERVICE_NAME = "temp_service";

AutoConfigStreamPage::AutoConfigStreamPage(obs_service_t *service,
					   QWidget *parent)
	: QWizardPage(parent),
	  ui(new Ui_AutoConfigStreamPage)
{
	ui->setupUi(this);
	ui->bitrateLabel->setVisible(false);
	ui->bitrate->setVisible(false);

	int vertSpacing = ui->topLayout->verticalSpacing();

	QMargins m = ui->topLayout->contentsMargins();
	m.setBottom(vertSpacing / 2);
	ui->topLayout->setContentsMargins(m);

	m = ui->serviceProps->contentsMargins();
	m.setTop(vertSpacing / 2);
	ui->serviceProps->setContentsMargins(m);

	m = ui->streamkeyPageLayout->contentsMargins();
	m.setTop(vertSpacing / 2);
	ui->streamkeyPageLayout->setContentsMargins(m);

	setTitle(QTStr("Basic.AutoConfig.StreamPage"));
	setSubTitle(QTStr("Basic.AutoConfig.StreamPage.SubTitle"));

	LoadServices(false);

	OBSDataAutoRelease settings = obs_service_get_settings(service);
	const char *id = obs_service_get_id(service);
	uint32_t flags = obs_service_get_flags(service);

	tempService =
		obs_service_create_private(id, TEMP_SERVICE_NAME, nullptr);

	/* Avoid sharing the same obs_data_t pointer,
	 * between the service ,the temp service and the properties view */
	const char *settingsJson = obs_data_get_json(settings);
	settings = obs_data_create_from_json(settingsJson);
	obs_service_update(tempService, settings);

	/* Reset to index 0 if internal or deprecated */
	if ((flags & OBS_SERVICE_INTERNAL) == 0 &&
	    (flags & OBS_SERVICE_DEPRECATED) == 0) {
		if ((flags & OBS_SERVICE_UNCOMMON) != 0)
			LoadServices(true);

		int idx = ui->service->findData(QT_UTF8(id));

		QSignalBlocker s(ui->service);
		ui->service->setCurrentIndex(idx);
		delete streamServiceProps;
		streamServiceProps = CreateTempServicePropertyView(settings);
		ui->serviceLayout->addWidget(streamServiceProps);

		ServiceChanged();
	} else {
		ui->service->currentIndexChanged(0);
	}

	connect(ui->doBandwidthTest, SIGNAL(toggled(bool)), this,
		SLOT(ServiceChanged()));
}

AutoConfigStreamPage::~AutoConfigStreamPage() {}

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
	int bitrate;
	if (!(ui->doBandwidthTest->isChecked() &&
	      ui->doBandwidthTest->isEnabled())) {
		bitrate = ui->bitrate->value();
		wiz->idealBitrate = bitrate;
	} else {
		/* Default test target is 10 Mbps */
		bitrate = 10000;
	}

	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_int(settings, "bitrate", bitrate);
	obs_service_apply_encoder_settings2(tempService, "obs_x264", settings);

	wiz->service = obs_service_get_ref(tempService);

	wiz->bandwidthTest = ui->doBandwidthTest->isChecked() &&
			     ui->doBandwidthTest->isEnabled();
	wiz->startingBitrate = (int)obs_data_get_int(settings, "bitrate");
	wiz->idealBitrate = wiz->startingBitrate;
	wiz->serviceName = QT_TO_UTF8(ui->service->currentText());
	if (ui->preferHardware)
		wiz->preferHardware = ui->preferHardware->isChecked();

	if (wiz->bandwidthTest &&
	    !obs_service_can_bandwidth_test(tempService)) {
		QMessageBox::StandardButton button;
#define WARNING_TEXT(x) QTStr("Basic.AutoConfig.StreamPage.StreamWarning." x)
		button = OBSMessageBox::question(this, WARNING_TEXT("Title"),
						 WARNING_TEXT("Text"));
#undef WARNING_TEXT

		if (button == QMessageBox::No)
			return false;
	}

	return true;
}

void AutoConfigStreamPage::ServiceChanged()
{
	obs_service_update(tempService, streamServiceProps->GetSettings());

	/* Enable bandwidth test if available */
	bool canBandwidthTest = obs_service_can_bandwidth_test(tempService);
	if (canBandwidthTest)
		obs_service_enable_bandwidth_test(tempService, true);

	/* Check if the service can connect to allow to do a bandwidth test */
	bool canTryToConnect = obs_service_can_try_to_connect(tempService);

	/* Disable bandwidth test */
	if (canBandwidthTest)
		obs_service_enable_bandwidth_test(tempService, false);

	/* Make wizard bandwidth test available if the service can connect.
	 * Otherwise, disable it */
	ui->doBandwidthTest->setEnabled(canTryToConnect);

	bool testBandwidth = ui->doBandwidthTest->isChecked() &&
			     ui->doBandwidthTest->isEnabled();

	ui->bitrateLabel->setHidden(testBandwidth);
	ui->bitrate->setHidden(testBandwidth);

	UpdateCompleted();
}

constexpr int SHOW_ALL = 1;

/* Note: Identical to OBSBasicSettings function except it does not show deprecated services */
void AutoConfigStreamPage::LoadServices(bool showAll)
{
	const char *id;
	size_t idx = 0;
	bool needShowAllOption = false;

	QSignalBlocker sb(ui->service);

	ui->service->clear();
	ui->service->setModel(new QStandardItemModel(0, 1, ui->service));

	while (obs_enum_service_types(idx++, &id)) {
		uint32_t flags = obs_get_service_flags(id);

		if ((flags & OBS_SERVICE_INTERNAL) != 0 ||
		    (flags & OBS_SERVICE_DEPRECATED) != 0)
			continue;

		QStringList protocols =
			QT_UTF8(obs_get_service_supported_protocols(id))
				.split(";");

		if (protocols.empty()) {
			blog(LOG_WARNING, "No protocol found for service '%s'",
			     id);
			continue;
		}

		bool protocolRegistered = false;
		for (uint32_t i = 0; i < protocols.size(); i++) {
			protocolRegistered |= obs_is_output_protocol_registered(
				QT_TO_UTF8(protocols[i]));
		}

		if (!protocolRegistered) {
			blog(LOG_WARNING,
			     "No registered protocol compatible with service '%s'",
			     id);
			continue;
		}

		bool isUncommon = (flags & OBS_SERVICE_UNCOMMON) != 0;

		QString name(obs_service_get_display_name(id));

		if (showAll || !isUncommon)
			ui->service->addItem(name, QT_UTF8(id));

		if (isUncommon && !showAll)
			needShowAllOption = true;
	}

	if (needShowAllOption) {
		ui->service->addItem(
			QTStr("Basic.AutoConfig.StreamPage.Service.ShowAll"),
			QVariant(SHOW_ALL));
	}

	QSortFilterProxyModel *model =
		new ServiceSortFilterProxyModel(ui->service);
	model->setSourceModel(ui->service->model());
	// Combo's current model must be reparented,
	// Otherwise QComboBox::setModel() will delete it
	ui->service->model()->setParent(model);

	ui->service->setModel(model);

	ui->service->model()->sort(0);
}

void AutoConfigStreamPage::UpdateCompleted()
{
	/* Assume ready if an URL is present, we can't add a specific behavior
	 * for each service. */
	const char *streamUrl = obs_service_get_connect_info(
		tempService, OBS_SERVICE_CONNECT_INFO_SERVER_URL);

	ready = (streamUrl != NULL && streamUrl[0] != '\0');

	emit completeChanged();
}

void AutoConfigStreamPage::on_service_currentIndexChanged(int)
{
	if (ui->service->currentData().toInt() == SHOW_ALL) {
		LoadServices(true);
		ui->service->showPopup();
		return;
	}

	OBSServiceAutoRelease oldService =
		obs_service_get_ref(tempService.Get());

	QString service = ui->service->currentData().toString();
	OBSDataAutoRelease newSettings =
		obs_service_defaults(QT_TO_UTF8(service));
	tempService = obs_service_create_private(
		QT_TO_UTF8(service), TEMP_SERVICE_NAME, newSettings);

	bool cancelChange = false;
	if (!obs_service_get_protocol(tempService)) {
		/*
	 	 * Cancel the change if the service happen to be without default protocol.
	 	 *
	 	 * This is better than generating dozens of obs_service_t to check
	 	 * if there is a default protocol while filling the combo box.
	 	 */
		OBSMessageBox::warning(
			this,
			QTStr("Basic.Settings.Stream.NoDefaultProtocol.Title"),
			QTStr("Basic.Settings.Stream.NoDefaultProtocol.Text")
				.arg(ui->service->currentText()));
		cancelChange = true;
	}

	if (cancelChange) {
		tempService = obs_service_get_ref(oldService);
		const char *id = obs_service_get_id(tempService);
		uint32_t flags = obs_get_service_flags(id);
		if ((flags & OBS_SERVICE_INTERNAL) != 0) {
			QString name(obs_service_get_display_name(id));
			if ((flags & OBS_SERVICE_DEPRECATED) != 0)
				name = QTStr("Basic.Settings.Stream.DeprecatedType")
					       .arg(name);

			ui->service->setPlaceholderText(name);
		}
		QSignalBlocker s(ui->service);
		ui->service->setCurrentIndex(
			ui->service->findData(QT_UTF8(id)));

		return;
	}

	delete streamServiceProps;
	streamServiceProps = CreateTempServicePropertyView(nullptr);
	ui->serviceLayout->addWidget(streamServiceProps);

	ServiceChanged();
}

OBSPropertiesView *
AutoConfigStreamPage::CreateTempServicePropertyView(obs_data_t *settings)
{
	OBSDataAutoRelease defaultSettings =
		obs_service_defaults(obs_service_get_id(tempService));
	OBSPropertiesView *view;

	view = new OBSPropertiesView(
		settings ? settings : defaultSettings.Get(), tempService,
		(PropertiesReloadCallback)obs_service_properties, nullptr,
		nullptr, 170);
	;
	view->setFrameShape(QFrame::NoFrame);
	view->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	QObject::connect(view, &OBSPropertiesView::Changed, this,
			 &AutoConfigStreamPage::ServiceChanged);

	return view;
}

/* ------------------------------------------------------------------------- */

AutoConfig::AutoConfig(QWidget *parent) : QWizard(parent)
{
	EnableThreadedMessageBoxes(true);

	calldata_t cd = {0};
	calldata_set_int(&cd, "seconds", 5);

	OBSBasic *main = reinterpret_cast<OBSBasic *>(parent);
	main->EnableOutputs(false);

	installEventFilter(CreateShortcutFilter());

#if defined(_WIN32) || defined(__APPLE__)
	setWizardStyle(QWizard::ModernStyle);
#endif
	obs_service_t *service = main->GetService();
	streamPage = new AutoConfigStreamPage(service);

	setPage(StartPage, new AutoConfigStartPage());
	setPage(VideoPage, new AutoConfigVideoPage());
	setPage(StreamPage, streamPage);
	setPage(TestPage, new AutoConfigTestPage());
	setWindowTitle(QTStr("Basic.AutoConfig"));
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	baseResolutionCX = ovi.base_width;
	baseResolutionCY = ovi.base_height;

	int bitrate =
		config_get_int(main->Config(), "SimpleOutput", "VBitrate");
	streamPage->ui->bitrate->setValue(bitrate);

	TestHardwareEncoding();
	if (!hardwareEncodingAvailable) {
		delete streamPage->ui->preferHardware;
		streamPage->ui->preferHardware = nullptr;
	} else {
		/* Newer generations of NVENC have a high enough quality to
		 * bitrate ratio that if NVENC is available, it makes sense to
		 * just always prefer hardware encoding by default */
		bool preferHardware = nvencAvailable || appleAvailable ||
				      os_get_physical_cores() <= 4;
		streamPage->ui->preferHardware->setChecked(preferHardware);
	}

	setOptions(QWizard::WizardOptions());
	setButtonText(QWizard::FinishButton,
		      QTStr("Basic.AutoConfig.ApplySettings"));
	setButtonText(QWizard::BackButton, QTStr("Back"));
	setButtonText(QWizard::NextButton, QTStr("Next"));
	setButtonText(QWizard::CancelButton, QTStr("Cancel"));
}

AutoConfig::~AutoConfig()
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	main->EnableOutputs(true);
	EnableThreadedMessageBoxes(false);
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
		else if (strcmp(id, "h264_texture_amf") == 0)
			hardwareEncodingAvailable = vceAvailable = true;
#ifdef __APPLE__
		else if (strcmp(id,
				"com.apple.videotoolbox.videoencoder.ave.avc") ==
				 0
#ifndef __aarch64__
			 && os_get_emulation_status() == true
#endif
		)
			if (__builtin_available(macOS 13.0, *))
				hardwareEncodingAvailable = appleAvailable =
					true;
#endif
	}
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
	case Encoder::Apple:
		return SIMPLE_ENCODER_APPLE_H264;
	default:
		return SIMPLE_ENCODER_X264;
	}
};

void AutoConfig::SaveStreamSettings()
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

	/* ---------------------------------- */
	/* save service                       */

	OBSDataAutoRelease settings = obs_service_get_settings(service);
	const char *settingsJson = obs_data_get_json(settings);
	settings = obs_data_create_from_json(settingsJson);

	OBSServiceAutoRelease newService =
		obs_service_create(obs_service_get_id(service),
				   "default_service", settings, nullptr);

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
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

	if (recordingEncoder != Encoder::Stream)
		config_set_string(main->Config(), "SimpleOutput", "RecEncoder",
				  GetEncoderId(recordingEncoder));

	const char *quality = recordingQuality == Quality::High ? "Small"
								: "Stream";

	config_set_string(main->Config(), "Output", "Mode", "Simple");
	config_set_string(main->Config(), "SimpleOutput", "RecQuality",
			  quality);
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
