#include <QMessageBox>
#include <QUrl>
#include <QStandardItemModel>

#include "window-basic-settings.hpp"
#include "obs-frontend-api.h"
#include "obs-app.hpp"
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"
#include "service-sort-filter.hpp"

constexpr int SHOW_ALL = 1;
constexpr const char *TEMP_SERVICE_NAME = "temp_service";

inline bool OBSBasicSettings::IsCustomOrInternalService() const
{
	return ui->service->currentIndex() == -1 ||
	       ui->service->currentData().toString() == "custom_service";
}

void OBSBasicSettings::InitStreamPage()
{
	int vertSpacing = ui->topStreamLayout->verticalSpacing();

	QMargins m = ui->topStreamLayout->contentsMargins();
	m.setBottom(vertSpacing / 2);
	ui->topStreamLayout->setContentsMargins(m);

	LoadServices(false);

	connect(ui->ignoreRecommended, SIGNAL(clicked(bool)), this,
		SLOT(DisplayEnforceWarning(bool)));
	connect(ui->ignoreRecommended, SIGNAL(toggled(bool)), this,
		SLOT(UpdateResFPSLimits()));
}

void OBSBasicSettings::LoadStream1Settings()
{
	loading = true;

	bool ignoreRecommended =
		config_get_bool(main->Config(), "Stream1", "IgnoreRecommended");
	obs_service_t *service = main->GetService();
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

	if ((flags & OBS_SERVICE_UNCOMMON) != 0)
		LoadServices(true);

	int idx = ui->service->findData(QT_UTF8(id));
	if (idx == -1) {
		QString name(obs_service_get_display_name(id));
		if ((flags & OBS_SERVICE_DEPRECATED) != 0)
			name = QTStr("Basic.Settings.Stream.DeprecatedType")
				       .arg(name);

		ui->service->setPlaceholderText(name);
	}

	QSignalBlocker s(ui->service);
	QSignalBlocker i(ui->ignoreRecommended);

	ui->service->setCurrentIndex(idx);

	ui->ignoreRecommended->setEnabled(!IsCustomOrInternalService());
	ui->ignoreRecommended->setChecked(ignoreRecommended);

	delete streamServiceProps;
	streamServiceProps = CreateTempServicePropertyView(settings);
	ui->serviceLayout->addWidget(streamServiceProps);

	UpdateServiceRecommendations();
	DisplayEnforceWarning(ignoreRecommended);

	loading = false;

	QMetaObject::invokeMethod(this, &OBSBasicSettings::UpdateResFPSLimits,
				  Qt::QueuedConnection);
}

void OBSBasicSettings::SaveStream1Settings()
{
	OBSDataAutoRelease settings = obs_service_get_settings(tempService);
	const char *settingsJson = obs_data_get_json(settings);
	settings = obs_data_create_from_json(settingsJson);

	OBSServiceAutoRelease newService =
		obs_service_create(obs_service_get_id(tempService),
				   "default_service", settings, nullptr);

	if (!newService)
		return;

	main->SetService(newService);
	main->SaveService();

	SaveCheckBox(ui->ignoreRecommended, "Stream1", "IgnoreRecommended");
}

/* NOTE: Identical to AutoConfigStreamPage function except it shows deprecated services */
void OBSBasicSettings::LoadServices(bool showAll)
{
	const char *id;
	size_t idx = 0;
	bool needShowAllOption = false;

	QSignalBlocker sb(ui->service);

	ui->service->clear();
	ui->service->setModel(new QStandardItemModel(0, 1, ui->service));

	while (obs_enum_service_types(idx++, &id)) {
		uint32_t flags = obs_get_service_flags(id);

		if ((flags & OBS_SERVICE_INTERNAL) != 0)
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
		bool isDeprecated = (flags & OBS_SERVICE_DEPRECATED) != 0;

		QString name(obs_service_get_display_name(id));
		if (isDeprecated)
			name = QTStr("Basic.Settings.Stream.DeprecatedType")
				       .arg(name);

		if (showAll || !(isUncommon || isDeprecated))
			ui->service->addItem(name, QT_UTF8(id));

		if ((isUncommon || isDeprecated) && !showAll)
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

void OBSBasicSettings::on_service_currentIndexChanged(int)
{
	ui->service->setPlaceholderText("");

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

	if (cancelChange ||
	    !(ServiceSupportsCodecCheck() && UpdateResFPSLimits())) {
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

	ui->ignoreRecommended->setEnabled(!IsCustomOrInternalService());

	delete streamServiceProps;
	streamServiceProps = CreateTempServicePropertyView(nullptr, true);
	ui->serviceLayout->addWidget(streamServiceProps);

	UpdateServiceRecommendations();

	UpdateVodTrackSetting();
	UpdateAdvNetworkGroup();
}

void OBSBasicSettings::UpdateVodTrackSetting()
{
	bool enableForCustomServer = config_get_bool(
		GetGlobalConfig(), "General", "EnableCustomServerVodTrack");
	bool enableVodTrack = obs_service_get_audio_track_cap(tempService) ==
			      OBS_SERVICE_AUDIO_ARCHIVE_TRACK;
	bool wasEnabled = !!vodTrackCheckbox;

	if (enableForCustomServer && IsCustomOrInternalService())
		enableVodTrack = true;

	if (enableVodTrack == wasEnabled)
		return;

	if (!enableVodTrack) {
		delete vodTrackCheckbox;
		delete vodTrackContainer;
		delete simpleVodTrack;
		return;
	}

	/* -------------------------------------- */
	/* simple output mode vod track widgets   */

	bool simpleAdv = ui->simpleOutAdvanced->isChecked();
	bool vodTrackEnabled = config_get_bool(main->Config(), "SimpleOutput",
					       "VodTrackEnabled");

	simpleVodTrack = new QCheckBox(this);
	simpleVodTrack->setText(
		QTStr("Basic.Settings.Output.Simple.TwitchVodTrack"));
	simpleVodTrack->setVisible(simpleAdv);
	simpleVodTrack->setChecked(vodTrackEnabled);

	int pos;
	ui->simpleStreamingLayout->getWidgetPosition(ui->simpleOutAdvanced,
						     &pos, nullptr);
	ui->simpleStreamingLayout->insertRow(pos + 1, nullptr, simpleVodTrack);

	HookWidget(simpleVodTrack, SIGNAL(clicked(bool)),
		   SLOT(OutputsChanged()));
	connect(ui->simpleOutAdvanced, SIGNAL(toggled(bool)),
		simpleVodTrack.data(), SLOT(setVisible(bool)));

	/* -------------------------------------- */
	/* advanced output mode vod track widgets */

	vodTrackCheckbox = new QCheckBox(this);
	vodTrackCheckbox->setText(
		QTStr("Basic.Settings.Output.Adv.TwitchVodTrack"));
	vodTrackCheckbox->setLayoutDirection(Qt::RightToLeft);

	vodTrackContainer = new QWidget(this);
	QHBoxLayout *vodTrackLayout = new QHBoxLayout();
	for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
		vodTrack[i] = new QRadioButton(QString::number(i + 1));
		vodTrackLayout->addWidget(vodTrack[i]);

		HookWidget(vodTrack[i], SIGNAL(clicked(bool)),
			   SLOT(OutputsChanged()));
	}

	HookWidget(vodTrackCheckbox, SIGNAL(clicked(bool)),
		   SLOT(OutputsChanged()));

	vodTrackLayout->addStretch();
	vodTrackLayout->setContentsMargins(0, 0, 0, 0);

	vodTrackContainer->setLayout(vodTrackLayout);

	ui->advOutTopLayout->insertRow(2, vodTrackCheckbox, vodTrackContainer);

	vodTrackEnabled =
		config_get_bool(main->Config(), "AdvOut", "VodTrackEnabled");
	vodTrackCheckbox->setChecked(vodTrackEnabled);
	vodTrackContainer->setEnabled(vodTrackEnabled);

	connect(vodTrackCheckbox, SIGNAL(clicked(bool)), vodTrackContainer,
		SLOT(setEnabled(bool)));

	int trackIndex =
		config_get_int(main->Config(), "AdvOut", "VodTrackIndex");
	for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
		vodTrack[i]->setChecked((i + 1) == trackIndex);
	}
}

void OBSBasicSettings::UpdateServiceRecommendations()
{
	int vbitrate, abitrate;
	BPtr<obs_service_resolution> res_list;
	size_t res_count;
	int fps;

	obs_service_get_max_bitrate(tempService, &vbitrate, &abitrate);
	obs_service_get_supported_resolutions(tempService, &res_list,
					      &res_count);
	obs_service_get_max_fps(tempService, &fps);

	QString text;

#define ENFORCE_TEXT(x) QTStr("Basic.Settings.Stream.Recommended." x)
	if (vbitrate)
		text += ENFORCE_TEXT("MaxVideoBitrate")
				.arg(QString::number(vbitrate));
	if (abitrate) {
		if (!text.isEmpty())
			text += "<br>";
		text += ENFORCE_TEXT("MaxAudioBitrate")
				.arg(QString::number(abitrate));
	}
	if (res_count) {
		if (!text.isEmpty())
			text += "<br>";

		obs_service_resolution best_res = {};
		int best_res_pixels = 0;

		for (size_t i = 0; i < res_count; i++) {
			obs_service_resolution res = res_list[i];
			int res_pixels = res.cx + res.cy;
			if (res_pixels > best_res_pixels) {
				best_res = res;
				best_res_pixels = res_pixels;
			}
		}

		QString res_str =
			QString("%1x%2").arg(QString::number(best_res.cx),
					     QString::number(best_res.cy));
		text += ENFORCE_TEXT("MaxResolution").arg(res_str);
	}
	if (fps) {
		if (!text.isEmpty())
			text += "<br>";

		text += ENFORCE_TEXT("MaxFPS").arg(QString::number(fps));
	}
#undef ENFORCE_TEXT

	ui->enforceSettings->setVisible(!text.isEmpty());
	ui->enforceSettingsLabel->setText(text);
}

void OBSBasicSettings::DisplayEnforceWarning(bool checked)
{
	if (IsCustomOrInternalService())
		return;

	if (!checked) {
		SimpleRecordingEncoderChanged();
		return;
	}

	QMessageBox::StandardButton button;

#define ENFORCE_WARNING(x) \
	QTStr("Basic.Settings.Stream.IgnoreRecommended.Warn." x)

	button = OBSMessageBox::question(this, ENFORCE_WARNING("Title"),
					 ENFORCE_WARNING("Text"));
#undef ENFORCE_WARNING

	if (button == QMessageBox::No) {
		QMetaObject::invokeMethod(ui->ignoreRecommended, "setChecked",
					  Qt::QueuedConnection,
					  Q_ARG(bool, false));
		return;
	}

	SimpleRecordingEncoderChanged();
}

bool OBSBasicSettings::ResFPSValid(obs_service_resolution *res_list,
				   size_t res_count, int max_fps)
{
	if (!res_count && !max_fps)
		return true;

	if (res_count) {
		QString res = ui->outputResolution->currentText();
		bool found_res = false;

		int cx, cy;
		if (sscanf(QT_TO_UTF8(res), "%dx%d", &cx, &cy) != 2)
			return false;

		for (size_t i = 0; i < res_count; i++) {
			if (res_list[i].cx == cx && res_list[i].cy == cy) {
				found_res = true;
				break;
			}
		}

		if (!found_res)
			return false;
	}

	if (max_fps) {
		int fpsType = ui->fpsType->currentIndex();
		if (fpsType != 0)
			return false;

		std::string fps_str = QT_TO_UTF8(ui->fpsCommon->currentText());
		float fps;
		sscanf(fps_str.c_str(), "%f", &fps);
		if (fps > (float)max_fps)
			return false;
	}

	return true;
}

extern void set_closest_res(int &cx, int &cy,
			    struct obs_service_resolution *res_list,
			    size_t count);

/* Checks for and updates the resolution and FPS limits of a service, if any.
 *
 * If the service has a resolution and/or FPS limit, this will enforce those
 * limitations in the UI itself, preventing the user from selecting a
 * resolution or FPS that's not supported.
 *
 * This is an unpleasant thing to have to do to users, but there is no other
 * way to ensure that a service's restricted resolution/framerate values are
 * properly enforced, otherwise users will just be confused when things aren't
 * working correctly. The user can turn it off if they're partner (or if they
 * want to risk getting in trouble with their service) by selecting the "Ignore
 * recommended settings" option in the stream section of settings.
 *
 * This only affects services that have a resolution and/or framerate limit, of
 * which as of this writing, and hopefully for the foreseeable future, there is
 * only one.
 */
bool OBSBasicSettings::UpdateResFPSLimits()
{
	if (loading)
		return false;

	int idx = ui->service->currentIndex();
	if (idx == -1)
		return false;

	bool ignoreRecommended = ui->ignoreRecommended->isChecked();
	BPtr<obs_service_resolution> res_list;
	size_t res_count = 0;
	int max_fps = 0;

	if (!IsCustomOrInternalService() && !ignoreRecommended) {
		obs_service_get_supported_resolutions(tempService, &res_list,
						      &res_count);
		obs_service_get_max_fps(tempService, &max_fps);
	}

	/* ------------------------------------ */
	/* Check for enforced res/FPS           */

	QString res = ui->outputResolution->currentText();
	QString fps_str;
	int cx = 0, cy = 0;
	double max_fpsd = (double)max_fps;
	int closest_fps_index = -1;
	double fpsd;

	sscanf(QT_TO_UTF8(res), "%dx%d", &cx, &cy);

	if (res_count)
		set_closest_res(cx, cy, res_list, res_count);

	if (max_fps) {
		int fpsType = ui->fpsType->currentIndex();

		if (fpsType == 1) { //Integer
			fpsd = (double)ui->fpsInteger->value();
		} else if (fpsType == 2) { //Fractional
			fpsd = (double)ui->fpsNumerator->value() /
			       (double)ui->fpsDenominator->value();
		} else { //Common
			sscanf(QT_TO_UTF8(ui->fpsCommon->currentText()), "%lf",
			       &fpsd);
		}

		double closest_diff = 1000000000000.0;

		for (int i = 0; i < ui->fpsCommon->count(); i++) {
			double com_fpsd;
			sscanf(QT_TO_UTF8(ui->fpsCommon->itemText(i)), "%lf",
			       &com_fpsd);

			if (com_fpsd > max_fpsd) {
				continue;
			}

			double diff = fabs(com_fpsd - fpsd);
			if (diff < closest_diff) {
				closest_diff = diff;
				closest_fps_index = i;
				fps_str = ui->fpsCommon->itemText(i);
			}
		}
	}

	QString res_str =
		QString("%1x%2").arg(QString::number(cx), QString::number(cy));

	/* ------------------------------------ */
	/* Display message box if res/FPS bad   */

	bool valid = ResFPSValid(res_list, res_count, max_fps);

	if (!valid) {
		QMessageBox::StandardButton button;

#define WARNING_VAL(x) \
	QTStr("Basic.Settings.Output.Warn.EnforceResolutionFPS." x)

		QString str;
		if (res_count)
			str += WARNING_VAL("Resolution").arg(res_str);
		if (max_fps) {
			if (!str.isEmpty())
				str += "\n";
			str += WARNING_VAL("FPS").arg(fps_str);
		}

		button = OBSMessageBox::question(this, WARNING_VAL("Title"),
						 WARNING_VAL("Msg").arg(str));
#undef WARNING_VAL

		if (button == QMessageBox::No) {
			return false;
		}
	}

	/* ------------------------------------ */
	/* Update widgets/values if switching   */
	/* to/from enforced resolution/FPS      */

	ui->outputResolution->blockSignals(true);
	if (res_count) {
		ui->outputResolution->clear();
		ui->outputResolution->setEditable(false);
		HookWidget(ui->outputResolution,
			   SIGNAL(currentIndexChanged(int)),
			   SLOT(VideoChangedResolution()));

		int new_res_index = -1;

		for (size_t i = 0; i < res_count; i++) {
			obs_service_resolution val = res_list[i];
			QString str =
				QString("%1x%2").arg(QString::number(val.cx),
						     QString::number(val.cy));
			ui->outputResolution->addItem(str);

			if (val.cx == cx && val.cy == cy)
				new_res_index = (int)i;
		}

		ui->outputResolution->setCurrentIndex(new_res_index);
		if (!valid) {
			ui->outputResolution->setProperty("changed", true);
			videoChanged = true;
			EnableApplyButton(true);
		}
	} else {
		QString baseRes = ui->baseResolution->currentText();
		int baseCX, baseCY;
		sscanf(QT_TO_UTF8(baseRes), "%dx%d", &baseCX, &baseCY);

		if (!ui->outputResolution->isEditable()) {
			RecreateOutputResolutionWidget();
			ui->outputResolution->blockSignals(true);
			ResetDownscales((uint32_t)baseCX, (uint32_t)baseCY,
					true);
			ui->outputResolution->setCurrentText(res);
		}
	}
	ui->outputResolution->blockSignals(false);

	if (max_fps) {
		for (int i = 0; i < ui->fpsCommon->count(); i++) {
			double com_fpsd;
			sscanf(QT_TO_UTF8(ui->fpsCommon->itemText(i)), "%lf",
			       &com_fpsd);

			if (com_fpsd > max_fpsd) {
				SetComboItemEnabled(ui->fpsCommon, i, false);
				continue;
			}
		}

		ui->fpsType->setCurrentIndex(0);
		ui->fpsCommon->setCurrentIndex(closest_fps_index);
		if (!valid) {
			ui->fpsType->setProperty("changed", true);
			ui->fpsCommon->setProperty("changed", true);
			videoChanged = true;
			EnableApplyButton(true);
		}
	} else {
		for (int i = 0; i < ui->fpsCommon->count(); i++)
			SetComboItemEnabled(ui->fpsCommon, i, true);
	}

	SetComboItemEnabled(ui->fpsType, 1, !max_fps);
	SetComboItemEnabled(ui->fpsType, 2, !max_fps);

	/* ------------------------------------ */

	return true;
}

static bool service_supports_codec(const char **codecs, const char *codec)
{
	if (!codecs)
		return true;

	while (*codecs) {
		if (strcmp(*codecs, codec) == 0)
			return true;
		codecs++;
	}

	return false;
}

extern bool EncoderAvailable(const char *encoder);
extern const char *get_simple_output_encoder(const char *name);

static inline bool service_supports_encoder(const char **codecs,
					    const char *encoder)
{
	if (!EncoderAvailable(encoder))
		return false;

	const char *codec = obs_get_encoder_codec(encoder);
	return service_supports_codec(codecs, codec);
}

static bool return_first_id(void *data, const char *id)
{
	const char **output = (const char **)data;

	*output = id;
	return false;
}

bool OBSBasicSettings::ServiceAndVCodecCompatible()
{
	bool simple = (ui->outputMode->currentIndex() == 0);
	bool ret;

	const char *codec;

	if (simple) {
		QString encoder =
			ui->simpleOutStrEncoder->currentData().toString();
		const char *id = get_simple_output_encoder(QT_TO_UTF8(encoder));
		codec = obs_get_encoder_codec(id);
	} else {
		QString encoder = ui->advOutEncoder->currentData().toString();
		codec = obs_get_encoder_codec(QT_TO_UTF8(encoder));
	}

	const char **codecs =
		obs_service_get_supported_video_codecs(tempService);

	if (!codecs) {
		const char *output;
		char **output_codecs;

		obs_enum_output_types_with_protocol(
			obs_service_get_protocol(tempService), &output,
			return_first_id);

		output_codecs = strlist_split(
			obs_get_output_supported_video_codecs(output), ';',
			false);

		ret = service_supports_codec((const char **)output_codecs,
					     codec);

		strlist_free(output_codecs);
	} else {
		ret = service_supports_codec(codecs, codec);
	}

	return ret;
}

bool OBSBasicSettings::ServiceAndACodecCompatible()
{
	bool simple = (ui->outputMode->currentIndex() == 0);
	bool ret;

	QString codec;

	if (simple) {
		codec = ui->simpleOutStrAEncoder->currentData().toString();
	} else {
		QString encoder = ui->advOutAEncoder->currentData().toString();
		codec = obs_get_encoder_codec(QT_TO_UTF8(encoder));
	}

	const char **codecs =
		obs_service_get_supported_audio_codecs(tempService);

	if (!codecs) {
		const char *output;
		char **output_codecs;

		obs_enum_output_types_with_protocol(
			obs_service_get_protocol(tempService), &output,
			return_first_id);
		output_codecs = strlist_split(
			obs_get_output_supported_audio_codecs(output), ';',
			false);

		ret = service_supports_codec((const char **)output_codecs,
					     QT_TO_UTF8(codec));

		strlist_free(output_codecs);
	} else {
		ret = service_supports_codec(codecs, QT_TO_UTF8(codec));
	}

	return ret;
}

/* we really need a way to find fallbacks in a less hardcoded way. maybe. */
static QString get_adv_fallback(const QString &enc)
{
	if (enc == "jim_hevc_nvenc" || enc == "jim_av1_nvenc")
		return "jim_nvenc";
	if (enc == "h265_texture_amf" || enc == "av1_texture_amf")
		return "h264_texture_amf";
	if (enc == "com.apple.videotoolbox.videoencoder.ave.hevc")
		return "com.apple.videotoolbox.videoencoder.ave.avc";
	if (enc == "obs_qsv11_av1")
		return "obs_qsv11";
	return "obs_x264";
}

static QString get_adv_audio_fallback(const QString &enc)
{
	const char *codec = obs_get_encoder_codec(QT_TO_UTF8(enc));

	if (strcmp(codec, "aac") == 0)
		return "ffmpeg_opus";

	QString aac_default = "ffmpeg_aac";
	if (EncoderAvailable("CoreAudio_AAC"))
		aac_default = "CoreAudio_AAC";
	else if (EncoderAvailable("libfdk_aac"))
		aac_default = "libfdk_aac";

	return aac_default;
}

static QString get_simple_fallback(const QString &enc)
{
	if (enc == SIMPLE_ENCODER_NVENC_HEVC || enc == SIMPLE_ENCODER_NVENC_AV1)
		return SIMPLE_ENCODER_NVENC;
	if (enc == SIMPLE_ENCODER_AMD_HEVC || enc == SIMPLE_ENCODER_AMD_AV1)
		return SIMPLE_ENCODER_AMD;
	if (enc == SIMPLE_ENCODER_APPLE_HEVC)
		return SIMPLE_ENCODER_APPLE_H264;
	if (enc == SIMPLE_ENCODER_QSV_AV1)
		return SIMPLE_ENCODER_QSV;
	return SIMPLE_ENCODER_X264;
}

bool OBSBasicSettings::ServiceSupportsCodecCheck()
{
	if (loading)
		return false;

	bool vcodec_compat = ServiceAndVCodecCompatible();
	bool acodec_compat = ServiceAndACodecCompatible();

	if (vcodec_compat && acodec_compat) {
		ResetEncoders(true);
		return true;
	}

	QString service = ui->service->currentText();
	QString cur_video_name;
	QString fb_video_name;
	QString cur_audio_name;
	QString fb_audio_name;
	bool simple = (ui->outputMode->currentIndex() == 0);

	/* ------------------------------------------------- */
	/* get current codec                                 */

	if (simple) {
		QString cur_enc =
			ui->simpleOutStrEncoder->currentData().toString();
		QString fb_enc = get_simple_fallback(cur_enc);

		int cur_idx = ui->simpleOutStrEncoder->findData(cur_enc);
		int fb_idx = ui->simpleOutStrEncoder->findData(fb_enc);

		cur_video_name = ui->simpleOutStrEncoder->itemText(cur_idx);
		fb_video_name = ui->simpleOutStrEncoder->itemText(fb_idx);

		cur_enc = ui->simpleOutStrAEncoder->currentData().toString();
		fb_enc = (cur_enc == "opus") ? "aac" : "opus";

		cur_audio_name = ui->simpleOutStrAEncoder->itemText(
			ui->simpleOutStrAEncoder->findData(cur_enc));
		fb_audio_name =
			(cur_enc == "opus")
				? QTStr("Basic.Settings.Output.Simple.Codec.AAC")
				: QTStr("Basic.Settings.Output.Simple.Codec.Opus");
	} else {
		QString cur_enc = ui->advOutEncoder->currentData().toString();
		QString fb_enc = get_adv_fallback(cur_enc);

		cur_video_name =
			obs_encoder_get_display_name(QT_TO_UTF8(cur_enc));
		fb_video_name =
			obs_encoder_get_display_name(QT_TO_UTF8(fb_enc));

		cur_enc = ui->advOutAEncoder->currentData().toString();
		fb_enc = get_adv_audio_fallback(cur_enc);

		cur_audio_name =
			obs_encoder_get_display_name(QT_TO_UTF8(cur_enc));
		fb_audio_name =
			obs_encoder_get_display_name(QT_TO_UTF8(fb_enc));
	}

#define WARNING_VAL(x) \
	QTStr("Basic.Settings.Output.Warn.ServiceCodecCompatibility." x)

	QString msg = WARNING_VAL("Msg").arg(
		service, vcodec_compat ? cur_audio_name : cur_video_name,
		vcodec_compat ? fb_audio_name : fb_video_name);
	if (!vcodec_compat && !acodec_compat)
		msg = WARNING_VAL("Msg2").arg(service, cur_video_name,
					      cur_audio_name, fb_video_name,
					      fb_audio_name);

	auto button = OBSMessageBox::question(this, WARNING_VAL("Title"), msg);
#undef WARNING_VAL

	if (button == QMessageBox::No) {
		return false;
	}

	ResetEncoders(true);
	return true;
}

#define TEXT_USE_STREAM_ENC \
	QTStr("Basic.Settings.Output.Adv.Recording.UseStreamEncoder")

void OBSBasicSettings::ResetEncoders(bool streamOnly)
{
	QString lastAdvVideoEnc = ui->advOutEncoder->currentData().toString();
	QString lastVideoEnc =
		ui->simpleOutStrEncoder->currentData().toString();
	QString lastAdvAudioEnc = ui->advOutAEncoder->currentData().toString();
	QString lastAudioEnc =
		ui->simpleOutStrAEncoder->currentData().toString();
	const char *protocol = obs_service_get_protocol(tempService);
	const char **vcodecs =
		obs_service_get_supported_video_codecs(tempService);
	const char **acodecs =
		obs_service_get_supported_audio_codecs(tempService);
	const char *type;
	BPtr<char *> output_vcodecs;
	BPtr<char *> output_acodecs;
	size_t idx = 0;

	if (!vcodecs) {
		const char *output;

		obs_enum_output_types_with_protocol(protocol, &output,
						    return_first_id);
		output_vcodecs = strlist_split(
			obs_get_output_supported_video_codecs(output), ';',
			false);
		vcodecs = (const char **)output_vcodecs.Get();
	}

	if (!acodecs) {
		const char *output;

		obs_enum_output_types_with_protocol(protocol, &output,
						    return_first_id);
		output_acodecs = strlist_split(
			obs_get_output_supported_audio_codecs(output), ';',
			false);
		acodecs = (const char **)output_acodecs.Get();
	}

	QSignalBlocker s1(ui->simpleOutStrEncoder);
	QSignalBlocker s2(ui->advOutEncoder);
	QSignalBlocker s3(ui->simpleOutStrAEncoder);
	QSignalBlocker s4(ui->advOutAEncoder);

	/* ------------------------------------------------- */
	/* clear encoder lists                               */

	ui->simpleOutStrEncoder->clear();
	ui->advOutEncoder->clear();
	ui->simpleOutStrAEncoder->clear();
	ui->advOutAEncoder->clear();

	if (!streamOnly) {
		ui->advOutRecEncoder->clear();
		ui->advOutRecAEncoder->clear();
	}

	/* ------------------------------------------------- */
	/* load advanced stream/recording encoders           */

	while (obs_enum_encoder_types(idx++, &type)) {
		const char *name = obs_encoder_get_display_name(type);
		const char *codec = obs_get_encoder_codec(type);
		uint32_t caps = obs_get_encoder_caps(type);

		QString qName = QT_UTF8(name);
		QString qType = QT_UTF8(type);

		if (obs_get_encoder_type(type) == OBS_ENCODER_VIDEO) {
			if ((caps & ENCODER_HIDE_FLAGS) != 0)
				continue;

			if (service_supports_codec(vcodecs, codec))
				ui->advOutEncoder->addItem(qName, qType);
			if (!streamOnly)
				ui->advOutRecEncoder->addItem(qName, qType);
		}

		if (obs_get_encoder_type(type) == OBS_ENCODER_AUDIO) {
			if (service_supports_codec(acodecs, codec))
				ui->advOutAEncoder->addItem(qName, qType);
			if (!streamOnly)
				ui->advOutRecAEncoder->addItem(qName, qType);
		}
	}

	ui->advOutEncoder->model()->sort(0);
	ui->advOutAEncoder->model()->sort(0);

	if (!streamOnly) {
		ui->advOutRecEncoder->model()->sort(0);
		ui->advOutRecEncoder->insertItem(0, TEXT_USE_STREAM_ENC,
						 "none");
		ui->advOutRecAEncoder->model()->sort(0);
		ui->advOutRecAEncoder->insertItem(0, TEXT_USE_STREAM_ENC,
						  "none");
	}

	/* ------------------------------------------------- */
	/* load simple stream encoders                       */

#define ENCODER_STR(str) QTStr("Basic.Settings.Output.Simple.Encoder." str)

	ui->simpleOutStrEncoder->addItem(ENCODER_STR("Software"),
					 QString(SIMPLE_ENCODER_X264));
#ifdef _WIN32
	if (service_supports_encoder(vcodecs, "obs_qsv11"))
		ui->simpleOutStrEncoder->addItem(
			ENCODER_STR("Hardware.QSV.H264"),
			QString(SIMPLE_ENCODER_QSV));
	if (service_supports_encoder(vcodecs, "obs_qsv11_av1"))
		ui->simpleOutStrEncoder->addItem(
			ENCODER_STR("Hardware.QSV.AV1"),
			QString(SIMPLE_ENCODER_QSV_AV1));
#endif
	if (service_supports_encoder(vcodecs, "ffmpeg_nvenc"))
		ui->simpleOutStrEncoder->addItem(
			ENCODER_STR("Hardware.NVENC.H264"),
			QString(SIMPLE_ENCODER_NVENC));
	if (service_supports_encoder(vcodecs, "jim_av1_nvenc"))
		ui->simpleOutStrEncoder->addItem(
			ENCODER_STR("Hardware.NVENC.AV1"),
			QString(SIMPLE_ENCODER_NVENC_AV1));
#ifdef ENABLE_HEVC
	if (service_supports_encoder(vcodecs, "h265_texture_amf"))
		ui->simpleOutStrEncoder->addItem(
			ENCODER_STR("Hardware.AMD.HEVC"),
			QString(SIMPLE_ENCODER_AMD_HEVC));
	if (service_supports_encoder(vcodecs, "ffmpeg_hevc_nvenc"))
		ui->simpleOutStrEncoder->addItem(
			ENCODER_STR("Hardware.NVENC.HEVC"),
			QString(SIMPLE_ENCODER_NVENC_HEVC));
#endif
	if (service_supports_encoder(vcodecs, "h264_texture_amf"))
		ui->simpleOutStrEncoder->addItem(
			ENCODER_STR("Hardware.AMD.H264"),
			QString(SIMPLE_ENCODER_AMD));
	if (service_supports_encoder(vcodecs, "av1_texture_amf"))
		ui->simpleOutStrEncoder->addItem(
			ENCODER_STR("Hardware.AMD.AV1"),
			QString(SIMPLE_ENCODER_AMD_AV1));
/* Preprocessor guard required for the macOS version check */
#ifdef __APPLE__
	if (service_supports_encoder(
		    vcodecs, "com.apple.videotoolbox.videoencoder.ave.avc")
#ifndef __aarch64__
	    && os_get_emulation_status() == true
#endif
	) {
		if (__builtin_available(macOS 13.0, *)) {
			ui->simpleOutStrEncoder->addItem(
				ENCODER_STR("Hardware.Apple.H264"),
				QString(SIMPLE_ENCODER_APPLE_H264));
		}
	}
#ifdef ENABLE_HEVC
	if (service_supports_encoder(
		    vcodecs, "com.apple.videotoolbox.videoencoder.ave.hevc")
#ifndef __aarch64__
	    && os_get_emulation_status() == true
#endif
	) {
		if (__builtin_available(macOS 13.0, *)) {
			ui->simpleOutStrEncoder->addItem(
				ENCODER_STR("Hardware.Apple.HEVC"),
				QString(SIMPLE_ENCODER_APPLE_HEVC));
		}
	}
#endif
#endif
	if (service_supports_encoder(acodecs, "CoreAudio_AAC") ||
	    service_supports_encoder(acodecs, "libfdk_aac") ||
	    service_supports_encoder(acodecs, "ffmpeg_aac"))
		ui->simpleOutStrAEncoder->addItem(
			QTStr("Basic.Settings.Output.Simple.Codec.AAC.Default"),
			"aac");
	if (service_supports_encoder(acodecs, "ffmpeg_opus"))
		ui->simpleOutStrAEncoder->addItem(
			QTStr("Basic.Settings.Output.Simple.Codec.Opus"),
			"opus");
#undef ENCODER_STR

	/* ------------------------------------------------- */
	/* Find fallback encoders                            */

	if (!lastAdvVideoEnc.isEmpty()) {
		int idx = ui->advOutEncoder->findData(lastAdvVideoEnc);
		if (idx == -1) {
			lastAdvVideoEnc = get_adv_fallback(lastAdvVideoEnc);
			ui->advOutEncoder->setProperty("changed",
						       QVariant(true));
			OutputsChanged();
		}

		idx = ui->advOutEncoder->findData(lastAdvVideoEnc);
		s2.unblock();
		ui->advOutEncoder->setCurrentIndex(idx);
	}

	if (!lastAdvAudioEnc.isEmpty()) {
		int idx = ui->advOutAEncoder->findData(lastAdvAudioEnc);
		if (idx == -1) {
			lastAdvAudioEnc =
				get_adv_audio_fallback(lastAdvAudioEnc);
			ui->advOutAEncoder->setProperty("changed",
							QVariant(true));
			OutputsChanged();
		}

		idx = ui->advOutAEncoder->findData(lastAdvAudioEnc);
		s4.unblock();
		ui->advOutAEncoder->setCurrentIndex(idx);
	}

	if (!lastVideoEnc.isEmpty()) {
		int idx = ui->simpleOutStrEncoder->findData(lastVideoEnc);
		if (idx == -1) {
			lastVideoEnc = get_simple_fallback(lastVideoEnc);
			ui->simpleOutStrEncoder->setProperty("changed",
							     QVariant(true));
			OutputsChanged();
		}

		idx = ui->simpleOutStrEncoder->findData(lastVideoEnc);
		s1.unblock();
		ui->simpleOutStrEncoder->setCurrentIndex(idx);
	}

	if (!lastAudioEnc.isEmpty()) {
		int idx = ui->simpleOutStrAEncoder->findData(lastAudioEnc);
		if (idx == -1) {
			lastAudioEnc = (lastAudioEnc == "opus") ? "aac"
								: "opus";
			ui->simpleOutStrAEncoder->setProperty("changed",
							      QVariant(true));
			OutputsChanged();
		}

		idx = ui->simpleOutStrAEncoder->findData(lastAudioEnc);
		s3.unblock();
		ui->simpleOutStrAEncoder->setCurrentIndex(idx);
	}
}

OBSPropertiesView *
OBSBasicSettings::CreateTempServicePropertyView(obs_data_t *settings,
						bool changed)
{
	OBSDataAutoRelease defaultSettings =
		obs_service_defaults(obs_service_get_id(tempService));
	OBSPropertiesView *view;

	view = new OBSPropertiesView(
		settings ? settings : defaultSettings.Get(), tempService,
		(PropertiesReloadCallback)obs_service_properties, nullptr,
		nullptr, 170);
	view->setFrameShape(QFrame::NoFrame);
	view->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	view->setProperty("changed", QVariant(changed));
	/* NOTE: Stream1Changed is implemented inside ServicePropertyViewChanged,
	 * in case the settings are reverted. */
	QObject::connect(view, &OBSPropertiesView::Changed, this,
			 &OBSBasicSettings::ServicePropertyViewChanged);

	return view;
}

void OBSBasicSettings::ServicePropertyViewChanged()
{
	OBSDataAutoRelease settings = obs_service_get_settings(tempService);
	QString oldSettingsJson = QT_UTF8(obs_data_get_json(settings));
	obs_service_update(tempService, streamServiceProps->GetSettings());

	if (!(ServiceSupportsCodecCheck() && UpdateResFPSLimits())) {
		QMetaObject::invokeMethod(this, "RestoreServiceSettings",
					  Qt::QueuedConnection,
					  Q_ARG(QString, oldSettingsJson));
		return;
	}

	UpdateVodTrackSetting();
	UpdateAdvNetworkGroup();

	if (!loading) {
		stream1Changed = true;
		streamServiceProps->setProperty("changed", QVariant(true));
		EnableApplyButton(true);
	}
}

void OBSBasicSettings::RestoreServiceSettings(QString settingsJson)
{
	OBSDataAutoRelease settings =
		obs_data_create_from_json(QT_TO_UTF8(settingsJson));
	obs_service_update(tempService, settings);

	bool changed = streamServiceProps->property("changed").toBool();

	delete streamServiceProps;
	streamServiceProps = CreateTempServicePropertyView(settings, changed);
	ui->serviceLayout->addWidget(streamServiceProps);
}
