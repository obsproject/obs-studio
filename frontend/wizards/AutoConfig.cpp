#include "AutoConfig.hpp"
#include "AutoConfigStartPage.hpp"
#include "AutoConfigStreamPage.hpp"
#include "AutoConfigTestPage.hpp"
#include "AutoConfigVideoPage.hpp"
#include "ui_AutoConfigStartPage.h"
#include "ui_AutoConfigStreamPage.h"
#include "ui_AutoConfigVideoPage.h"

#ifdef YOUTUBE_ENABLED
#include <docks/YouTubeAppDock.hpp>
#include <utility/YoutubeApiWrappers.hpp>
#endif
#include <widgets/OBSBasic.hpp>

#include <qt-wrappers.hpp>

#include "moc_AutoConfig.cpp"

constexpr std::string_view OBSServiceFileName = "service.json";

enum class ListOpt : int {
	ShowAll = 1,
	Custom,
};

static OBSData OpenServiceSettings(std::string &type)
{
	const OBSBasic *basic = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	const OBSProfile &currentProfile = basic->GetCurrentProfile();

	const std::filesystem::path jsonFilePath = currentProfile.path / std::filesystem::u8path(OBSServiceFileName);

	if (!std::filesystem::exists(jsonFilePath)) {
		return OBSData();
	}

	OBSDataAutoRelease data = obs_data_create_from_json_file_safe(jsonFilePath.u8string().c_str(), "bak");

	obs_data_set_default_string(data, "type", "rtmp_common");
	type = obs_data_get_string(data, "type");

	OBSDataAutoRelease settings = obs_data_get_obj(data, "settings");

	return settings.Get();
}

static void GetServiceInfo(std::string &type, std::string &service, std::string &server, std::string &key)
{
	OBSData settings = OpenServiceSettings(type);

	service = obs_data_get_string(settings, "service");
	server = obs_data_get_string(settings, "server");
	key = obs_data_get_string(settings, "key");
}

AutoConfig::AutoConfig(QWidget *parent) : QWizard(parent)
{
	EnableThreadedMessageBoxes(true);

	calldata_t cd = {0};
	calldata_set_int(&cd, "seconds", 5);

	proc_handler_t *ph = obs_get_proc_handler();
	proc_handler_call(ph, "twitch_ingests_refresh", &cd);
	proc_handler_call(ph, "amazon_ivs_ingests_refresh", &cd);
	calldata_free(&cd);

	OBSBasic *main = reinterpret_cast<OBSBasic *>(parent);
	main->EnableOutputs(false);

	installEventFilter(CreateShortcutFilter());

	std::string serviceType;
	GetServiceInfo(serviceType, serviceName, server, key);
#if defined(_WIN32) || defined(__APPLE__)
	setWizardStyle(QWizard::ModernStyle);
#endif
	streamPage = new AutoConfigStreamPage();

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

	/* ----------------------------------------- */
	/* check to see if Twitch's "auto" available */

	OBSDataAutoRelease twitchSettings = obs_data_create();

	obs_data_set_string(twitchSettings, "service", "Twitch");

	obs_properties_t *props = obs_get_service_properties("rtmp_common");
	obs_properties_apply_settings(props, twitchSettings);

	obs_property_t *p = obs_properties_get(props, "server");
	const char *first = obs_property_list_item_string(p, 0);
	twitchAuto = strcmp(first, "auto") == 0;

	obs_properties_destroy(props);

	/* ----------------------------------------- */
	/* check to see if Amazon IVS "auto" entries are available */

	OBSDataAutoRelease amazonIVSSettings = obs_data_create();

	obs_data_set_string(amazonIVSSettings, "service", "Amazon IVS");

	props = obs_get_service_properties("rtmp_common");
	obs_properties_apply_settings(props, amazonIVSSettings);

	p = obs_properties_get(props, "server");
	first = obs_property_list_item_string(p, 0);
	amazonIVSAuto = strncmp(first, "auto", 4) == 0;

	obs_properties_destroy(props);

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
	streamPage->UpdateMoreInfoLink();
	streamPage->lastService.clear();

	if (!customServer) {
		QComboBox *serverList = streamPage->ui->server;
		int idx = serverList->findData(QString(server.c_str()));
		if (idx == -1)
			idx = 0;

		serverList->setCurrentIndex(idx);
	} else {
		streamPage->ui->customServer->setText(server.c_str());
		int idx = streamPage->ui->service->findData(QVariant((int)ListOpt::Custom));
		streamPage->ui->service->setCurrentIndex(idx);
	}

	if (!key.empty())
		streamPage->ui->key->setText(key.c_str());

	TestHardwareEncoding();

	int bitrate = config_get_int(main->Config(), "SimpleOutput", "VBitrate");
	bool multitrackVideoEnabled = config_has_user_value(main->Config(), "Stream1", "EnableMultitrackVideo")
					      ? config_get_bool(main->Config(), "Stream1", "EnableMultitrackVideo")
					      : true;
	streamPage->ui->bitrate->setValue(bitrate);
	streamPage->ui->useMultitrackVideo->setChecked(hardwareEncodingAvailable && multitrackVideoEnabled);
	streamPage->ServiceChanged();

	if (!hardwareEncodingAvailable) {
		delete streamPage->ui->preferHardware;
		streamPage->ui->preferHardware = nullptr;
	} else {
		/* Newer generations of NVENC have a high enough quality to
		 * bitrate ratio that if NVENC is available, it makes sense to
		 * just always prefer hardware encoding by default */
		bool preferHardware = nvencAvailable || appleAvailable || os_get_physical_cores() <= 4;
		streamPage->ui->preferHardware->setChecked(preferHardware);
	}

	setOptions(QWizard::WizardOptions());
	setButtonText(QWizard::FinishButton, QTStr("Basic.AutoConfig.ApplySettings"));
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
		else if (strcmp(id, "com.apple.videotoolbox.videoencoder.ave.avc") == 0
#ifndef __aarch64__
			 && os_get_emulation_status() == true
#endif
		)
			if (__builtin_available(macOS 13.0, *))
				hardwareEncodingAvailable = appleAvailable = true;
#endif
	}
}

bool AutoConfig::CanTestServer(const char *server)
{
	if (!testRegions || (regionUS && regionEU && regionAsia && regionOther))
		return true;

	if (service == Service::Twitch) {
		if (astrcmp_n(server, "US West:", 8) == 0 || astrcmp_n(server, "US East:", 8) == 0 ||
		    astrcmp_n(server, "US Central:", 11) == 0) {
			return regionUS;
		} else if (astrcmp_n(server, "EU:", 3) == 0) {
			return regionEU;
		} else if (astrcmp_n(server, "Asia:", 5) == 0) {
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

#ifdef YOUTUBE_ENABLED
		if (YouTubeAppDock::IsYTServiceSelected()) {
			OBSBasic *main = OBSBasic::Get();
			main->NewYouTubeAppDock();
		}
#endif
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

	const char *service_id = customServer ? "rtmp_custom" : "rtmp_common";

	obs_service_t *oldService = main->GetService();
	OBSDataAutoRelease hotkeyData = obs_hotkeys_save_service(oldService);

	OBSDataAutoRelease settings = obs_data_create();

	if (!customServer)
		obs_data_set_string(settings, "service", serviceName.c_str());
	obs_data_set_string(settings, "server", server.c_str());
#ifdef YOUTUBE_ENABLED
	if (!streamPage->auth || !IsYouTubeService(serviceName))
		obs_data_set_string(settings, "key", key.c_str());
#else
	obs_data_set_string(settings, "key", key.c_str());
#endif

	OBSServiceAutoRelease newService = obs_service_create(service_id, "default_service", settings, hotkeyData);

	if (!newService)
		return;

	main->SetService(newService);
	main->SaveService();
	main->auth = streamPage->auth;
	if (!!main->auth) {
		main->auth->LoadUI();
		main->SetBroadcastFlowEnabled(main->auth->broadcastFlow());
	} else {
		main->SetBroadcastFlowEnabled(false);
	}

	/* ---------------------------------- */
	/* save stream settings               */

	config_set_int(main->Config(), "SimpleOutput", "VBitrate", idealBitrate);
	config_set_string(main->Config(), "SimpleOutput", "StreamEncoder", GetEncoderId(streamingEncoder));
	config_remove_value(main->Config(), "SimpleOutput", "UseAdvanced");

	config_set_bool(main->Config(), "Stream1", "EnableMultitrackVideo", multitrackVideo.testSuccessful);

	if (multitrackVideo.targetBitrate.has_value())
		config_set_int(main->Config(), "Stream1", "MultitrackVideoTargetBitrate",
			       *multitrackVideo.targetBitrate);
	else
		config_remove_value(main->Config(), "Stream1", "MultitrackVideoTargetBitrate");

	if (multitrackVideo.bitrate.has_value() && multitrackVideo.targetBitrate.has_value() &&
	    (static_cast<double>(*multitrackVideo.bitrate) / *multitrackVideo.targetBitrate) >= 0.90) {
		config_set_bool(main->Config(), "Stream1", "MultitrackVideoMaximumAggregateBitrateAuto", true);
	} else if (multitrackVideo.bitrate.has_value()) {
		config_set_bool(main->Config(), "Stream1", "MultitrackVideoMaximumAggregateBitrateAuto", false);
		config_set_int(main->Config(), "Stream1", "MultitrackVideoMaximumAggregateBitrate",
			       *multitrackVideo.bitrate);
	}
}

void AutoConfig::SaveSettings()
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

	if (recordingEncoder != Encoder::Stream)
		config_set_string(main->Config(), "SimpleOutput", "RecEncoder", GetEncoderId(recordingEncoder));

	const char *quality = recordingQuality == Quality::High ? "Small" : "Stream";

	config_set_string(main->Config(), "Output", "Mode", "Simple");
	config_set_string(main->Config(), "SimpleOutput", "RecQuality", quality);
	config_set_int(main->Config(), "Video", "BaseCX", baseResolutionCX);
	config_set_int(main->Config(), "Video", "BaseCY", baseResolutionCY);
	config_set_int(main->Config(), "Video", "OutputCX", idealResolutionCX);
	config_set_int(main->Config(), "Video", "OutputCY", idealResolutionCY);

	if (fpsType != FPSType::UseCurrent) {
		config_set_uint(main->Config(), "Video", "FPSType", 0);
		config_set_string(main->Config(), "Video", "FPSCommon", std::to_string(idealFPSNum).c_str());
	}

	main->ResetVideo();
	main->ResetOutputs();
	config_save_safe(main->Config(), "tmp", nullptr);
}
