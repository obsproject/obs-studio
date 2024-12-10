/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>
                          Philippe Groarke <philippe.groarke@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <obs.hpp>
#include <util/util.hpp>
#include <util/lexer.h>
#include <graphics/math-defs.h>
#include <initializer_list>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <QCompleter>
#include <QGuiApplication>
#include <QLineEdit>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDirIterator>
#include <QVariant>
#include <QTreeView>
#include <QScreen>
#include <QStandardItemModel>
#include <QSpacerItem>
#include <qt-wrappers.hpp>

#include "audio-encoders.hpp"
#include "hotkey-edit.hpp"
#include "source-label.hpp"
#include "obs-app.hpp"
#include "platform.hpp"
#include "properties-view.hpp"
#include "window-basic-main.hpp"
#include "moc_window-basic-settings.cpp"
#include "window-basic-main-outputs.hpp"
#include "window-projector.hpp"

#ifdef YOUTUBE_ENABLED
#include "youtube-api-wrappers.hpp"
#endif

#include <util/platform.h>
#include <util/dstr.hpp>
#include "ui-config.h"

using namespace std;

class SettingsEventFilter : public QObject {
	QScopedPointer<OBSEventFilter> shortcutFilter;

public:
	inline SettingsEventFilter() : shortcutFilter((OBSEventFilter *)CreateShortcutFilter()) {}

protected:
	bool eventFilter(QObject *obj, QEvent *event) override
	{
		int key;

		switch (event->type()) {
		case QEvent::KeyPress:
		case QEvent::KeyRelease:
			key = static_cast<QKeyEvent *>(event)->key();
			if (key == Qt::Key_Escape) {
				return false;
			}
		default:
			break;
		}

		return shortcutFilter->filter(obj, event);
	}
};

static inline bool ResTooHigh(uint32_t cx, uint32_t cy)
{
	return cx > 16384 || cy > 16384;
}

static inline bool ResTooLow(uint32_t cx, uint32_t cy)
{
	return cx < 32 || cy < 32;
}

/* parses "[width]x[height]", string, i.e. 1024x768 */
static bool ConvertResText(const char *res, uint32_t &cx, uint32_t &cy)
{
	BaseLexer lex;
	base_token token;

	lexer_start(lex, res);

	/* parse width */
	if (!lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;
	if (token.type != BASETOKEN_DIGIT)
		return false;

	cx = std::stoul(token.text.array);

	/* parse 'x' */
	if (!lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;
	if (strref_cmpi(&token.text, "x") != 0)
		return false;

	/* parse height */
	if (!lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;
	if (token.type != BASETOKEN_DIGIT)
		return false;

	cy = std::stoul(token.text.array);

	/* shouldn't be any more tokens after this */
	if (lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;

	if (ResTooHigh(cx, cy) || ResTooLow(cx, cy)) {
		cx = cy = 0;
		return false;
	}

	return true;
}

static inline bool WidgetChanged(QWidget *widget)
{
	return widget->property("changed").toBool();
}

static inline void SetComboByName(QComboBox *combo, const char *name)
{
	int idx = combo->findText(QT_UTF8(name));
	if (idx != -1)
		combo->setCurrentIndex(idx);
}

static inline bool SetComboByValue(QComboBox *combo, const char *name)
{
	int idx = combo->findData(QT_UTF8(name));
	if (idx != -1) {
		combo->setCurrentIndex(idx);
		return true;
	}

	return false;
}

static inline bool SetInvalidValue(QComboBox *combo, const char *name, const char *data = nullptr)
{
	combo->insertItem(0, name, data);

	QStandardItemModel *model = dynamic_cast<QStandardItemModel *>(combo->model());
	if (!model)
		return false;

	QStandardItem *item = model->item(0);
	item->setFlags(Qt::NoItemFlags);

	combo->setCurrentIndex(0);
	return true;
}

static inline QString GetComboData(QComboBox *combo)
{
	int idx = combo->currentIndex();
	if (idx == -1)
		return QString();

	return combo->itemData(idx).toString();
}

static int FindEncoder(QComboBox *combo, const char *name, int id)
{
	FFmpegCodec codec{name, id};

	for (int i = 0; i < combo->count(); i++) {
		QVariant v = combo->itemData(i);
		if (!v.isNull()) {
			if (codec == v.value<FFmpegCodec>()) {
				return i;
			}
		}
	}
	return -1;
}

#define INVALID_BITRATE 10000
static int FindClosestAvailableAudioBitrate(QComboBox *box, int bitrate)
{
	QList<int> bitrates;
	int prev = 0;
	int next = INVALID_BITRATE;

	for (int i = 0; i < box->count(); i++)
		bitrates << box->itemText(i).toInt();

	for (int val : bitrates) {
		if (next > val) {
			if (val == bitrate)
				return bitrate;

			if (val < next && val > bitrate)
				next = val;
			if (val > prev && val < bitrate)
				prev = val;
		}
	}

	if (next != INVALID_BITRATE)
		return next;
	if (prev != 0)
		return prev;
	return 192;
}
#undef INVALID_BITRATE

static void PopulateSimpleBitrates(QComboBox *box, bool opus)
{
	auto &bitrateMap = opus ? GetSimpleOpusEncoderBitrateMap() : GetSimpleAACEncoderBitrateMap();
	if (bitrateMap.empty())
		return;

	vector<pair<QString, QString>> pairs;
	for (auto &entry : bitrateMap)
		pairs.emplace_back(QString::number(entry.first), obs_encoder_get_display_name(entry.second.c_str()));

	QString currentBitrate = box->currentText();
	box->clear();

	for (auto &pair : pairs) {
		box->addItem(pair.first);
		box->setItemData(box->count() - 1, pair.second, Qt::ToolTipRole);
	}

	if (box->findData(currentBitrate) == -1) {
		int bitrate = FindClosestAvailableAudioBitrate(box, currentBitrate.toInt());
		box->setCurrentText(QString::number(bitrate));
	} else
		box->setCurrentText(currentBitrate);
}

static void PopulateAdvancedBitrates(initializer_list<QComboBox *> boxes, const char *stream_id, const char *rec_id)
{
	auto &streamBitrates = GetAudioEncoderBitrates(stream_id);
	auto &recBitrates = GetAudioEncoderBitrates(rec_id);
	if (streamBitrates.empty() || recBitrates.empty())
		return;

	QList<int> streamBitratesList;
	for (auto &bitrate : streamBitrates)
		streamBitratesList << bitrate;

	for (auto box : boxes) {
		QString currentBitrate = box->currentText();
		box->clear();

		for (auto &bitrate : recBitrates) {
			if (streamBitratesList.indexOf(bitrate) == -1)
				continue;

			box->addItem(QString::number(bitrate));
		}

		if (box->findData(currentBitrate) == -1) {
			int bitrate = FindClosestAvailableAudioBitrate(box, currentBitrate.toInt());
			box->setCurrentText(QString::number(bitrate));
		} else
			box->setCurrentText(currentBitrate);
	}
}

static std::tuple<int, int> aspect_ratio(int cx, int cy)
{
	int common = std::gcd(cx, cy);
	int newCX = cx / common;
	int newCY = cy / common;

	if (newCX == 8 && newCY == 5) {
		newCX = 16;
		newCY = 10;
	}

	return std::make_tuple(newCX, newCY);
}

static inline void HighlightGroupBoxLabel(QGroupBox *gb, QWidget *widget, QString objectName)
{
	QFormLayout *layout = qobject_cast<QFormLayout *>(gb->layout());

	if (!layout)
		return;

	QLabel *label = qobject_cast<QLabel *>(layout->labelForField(widget));

	if (label) {
		label->setObjectName(objectName);

		label->style()->unpolish(label);
		label->style()->polish(label);
	}
}

void RestrictResetBitrates(initializer_list<QComboBox *> boxes, int maxbitrate);

/* clang-format off */
#define COMBO_CHANGED   &QComboBox::currentIndexChanged
#define EDIT_CHANGED    &QLineEdit::textChanged
#define CBEDIT_CHANGED  &QComboBox::editTextChanged
#define CHECK_CHANGED   &QCheckBox::toggled
#define GROUP_CHANGED   &QGroupBox::toggled
#define SCROLL_CHANGED  &QSpinBox::valueChanged
#define DSCROLL_CHANGED &QDoubleSpinBox::valueChanged
#define TEXT_CHANGED    &QPlainTextEdit::textChanged

#define GENERAL_CHANGED &OBSBasicSettings::GeneralChanged
#define STREAM1_CHANGED &OBSBasicSettings::Stream1Changed
#define OUTPUTS_CHANGED &OBSBasicSettings::OutputsChanged
#define AUDIO_RESTART   &OBSBasicSettings::AudioChangedRestart
#define AUDIO_CHANGED   &OBSBasicSettings::AudioChanged
#define VIDEO_RES       &OBSBasicSettings::VideoChangedResolution
#define VIDEO_CHANGED   &OBSBasicSettings::VideoChanged
#define A11Y_CHANGED    &OBSBasicSettings::A11yChanged
#define APPEAR_CHANGED  &OBSBasicSettings::AppearanceChanged
#define ADV_CHANGED     &OBSBasicSettings::AdvancedChanged
#define ADV_RESTART     &OBSBasicSettings::AdvancedChangedRestart
/* clang-format on */

OBSBasicSettings::OBSBasicSettings(QWidget *parent)
	: QDialog(parent),
	  main(qobject_cast<OBSBasic *>(parent)),
	  ui(new Ui::OBSBasicSettings)
{
	string path;

	EnableThreadedMessageBoxes(true);

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->setupUi(this);

	main->EnableOutputs(false);

	ui->listWidget->setAttribute(Qt::WA_MacShowFocusRect, false);

	/* clang-format off */
	HookWidget(ui->language,             COMBO_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->updateChannelBox,     COMBO_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->enableAutoUpdates,    CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->openStatsOnStartup,   CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->hideOBSFromCapture,   CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->warnBeforeStreamStart,CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->warnBeforeStreamStop, CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->warnBeforeRecordStop, CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->hideProjectorCursor,  CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->projectorAlwaysOnTop, CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->recordWhenStreaming,  CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->keepRecordStreamStops,CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->replayWhileStreaming, CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->keepReplayStreamStops,CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->systemTrayEnabled,    CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->systemTrayWhenStarted,CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->systemTrayAlways,     CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->saveProjectors,       CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->closeProjectors,      CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->snappingEnabled,      CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->screenSnapping,       CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->centerSnapping,       CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->sourceSnapping,       CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->snapDistance,         DSCROLL_CHANGED,GENERAL_CHANGED);
	HookWidget(ui->overflowHide,         CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->overflowAlwaysVisible,CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->overflowSelectionHide,CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->previewSafeAreas,     CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->automaticSearch,      CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->previewSpacingHelpers,CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->doubleClickSwitch,    CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->studioPortraitLayout, CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->prevProgLabelToggle,  CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->multiviewMouseSwitch, CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->multiviewDrawNames,   CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->multiviewDrawAreas,   CHECK_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->multiviewLayout,      COMBO_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->theme, 		     COMBO_CHANGED,  APPEAR_CHANGED);
	HookWidget(ui->themeVariant,	     COMBO_CHANGED,  APPEAR_CHANGED);
	HookWidget(ui->service,              COMBO_CHANGED,  STREAM1_CHANGED);
	HookWidget(ui->server,               COMBO_CHANGED,  STREAM1_CHANGED);
	HookWidget(ui->customServer,         EDIT_CHANGED,   STREAM1_CHANGED);
	HookWidget(ui->serviceCustomServer,  EDIT_CHANGED,   STREAM1_CHANGED);
	HookWidget(ui->key,                  EDIT_CHANGED,   STREAM1_CHANGED);
	HookWidget(ui->bandwidthTestEnable,  CHECK_CHANGED,  STREAM1_CHANGED);
	HookWidget(ui->twitchAddonDropdown,  COMBO_CHANGED,  STREAM1_CHANGED);
	HookWidget(ui->useAuth,              CHECK_CHANGED,  STREAM1_CHANGED);
	HookWidget(ui->authUsername,         EDIT_CHANGED,   STREAM1_CHANGED);
	HookWidget(ui->authPw,               EDIT_CHANGED,   STREAM1_CHANGED);
	HookWidget(ui->ignoreRecommended,    CHECK_CHANGED,  STREAM1_CHANGED);
	HookWidget(ui->enableMultitrackVideo,      CHECK_CHANGED,  STREAM1_CHANGED);
	HookWidget(ui->multitrackVideoMaximumAggregateBitrateAuto, CHECK_CHANGED,  STREAM1_CHANGED);
	HookWidget(ui->multitrackVideoMaximumAggregateBitrate,     SCROLL_CHANGED, STREAM1_CHANGED);
	HookWidget(ui->multitrackVideoMaximumVideoTracksAuto, CHECK_CHANGED,  STREAM1_CHANGED);
	HookWidget(ui->multitrackVideoMaximumVideoTracks,     SCROLL_CHANGED, STREAM1_CHANGED);
	HookWidget(ui->multitrackVideoStreamDumpEnable,            CHECK_CHANGED,  STREAM1_CHANGED);
	HookWidget(ui->multitrackVideoConfigOverrideEnable,        CHECK_CHANGED,  STREAM1_CHANGED);
	HookWidget(ui->multitrackVideoConfigOverride,              TEXT_CHANGED,   STREAM1_CHANGED);
	HookWidget(ui->outputMode,           COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutputPath,     EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->simpleNoSpace,        CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutRecFormat,   COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutputVBitrate, SCROLL_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutStrEncoder,  COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutStrAEncoder, COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutputABitrate, COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutAdvanced,    CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutPreset,      COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutCustom,      EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutRecQuality,  COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutRecEncoder,  COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutRecAEncoder, COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutRecTrack1,   CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutRecTrack2,   CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutRecTrack3,   CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutRecTrack4,   CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutRecTrack5,   CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutRecTrack6,   CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutMuxCustom,   EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->simpleReplayBuf,      GROUP_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleRBSecMax,       SCROLL_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->simpleRBMegsMax,      SCROLL_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->advOutEncoder,        COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutAEncoder,       COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutRescale,        CBEDIT_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->advOutRescaleFilter,  COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack1,         CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack2,         CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack3,         CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack4,         CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack5,         CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack6,         CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutMultiTrack1,    CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutMultiTrack2,    CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutMultiTrack3,    CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutMultiTrack4,    CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutMultiTrack5,    CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutMultiTrack6,    CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutRecType,        COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutRecPath,        EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->advOutNoSpace,        CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutRecFormat,      COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutRecEncoder,     COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutRecAEncoder,    COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutRecRescale,     CBEDIT_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->advOutRecRescaleFilter, COMBO_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->advOutMuxCustom,      EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->advOutSplitFile,      CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutSplitFileType,  COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutSplitFileTime,  SCROLL_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->advOutSplitFileSize,  SCROLL_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->advOutRecTrack1,      CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutRecTrack2,      CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutRecTrack3,      CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutRecTrack4,      CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutRecTrack5,      CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutRecTrack6,      CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->flvTrack1,            CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->flvTrack2,            CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->flvTrack3,            CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->flvTrack4,            CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->flvTrack5,            CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->flvTrack6,            CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFType,         COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFRecPath,      EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFNoSpace,      CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFURL,          EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFFormat,       COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFMCfg,         EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFVBitrate,     SCROLL_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFVGOPSize,     SCROLL_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFUseRescale,   CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFIgnoreCompat, CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFRescale,      CBEDIT_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFVEncoder,     COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFVCfg,         EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFABitrate,     SCROLL_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFTrack1,       CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFTrack2,       CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFTrack3,       CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFTrack4,       CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFTrack5,       CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFTrack6,       CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFAEncoder,     COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFACfg,         EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack1Bitrate,  COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack1Name,     EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack2Bitrate,  COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack2Name,     EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack3Bitrate,  COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack3Name,     EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack4Bitrate,  COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack4Name,     EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack5Bitrate,  COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack5Name,     EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack6Bitrate,  COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack6Name,     EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->advReplayBuf,         CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advRBSecMax,          SCROLL_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->advRBMegsMax,         SCROLL_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->channelSetup,         COMBO_CHANGED,  AUDIO_RESTART);
	HookWidget(ui->sampleRate,           COMBO_CHANGED,  AUDIO_RESTART);
	HookWidget(ui->meterDecayRate,       COMBO_CHANGED,  AUDIO_CHANGED);
	HookWidget(ui->peakMeterType,        COMBO_CHANGED,  AUDIO_CHANGED);
	HookWidget(ui->desktopAudioDevice1,  COMBO_CHANGED,  AUDIO_CHANGED);
	HookWidget(ui->desktopAudioDevice2,  COMBO_CHANGED,  AUDIO_CHANGED);
	HookWidget(ui->auxAudioDevice1,      COMBO_CHANGED,  AUDIO_CHANGED);
	HookWidget(ui->auxAudioDevice2,      COMBO_CHANGED,  AUDIO_CHANGED);
	HookWidget(ui->auxAudioDevice3,      COMBO_CHANGED,  AUDIO_CHANGED);
	HookWidget(ui->auxAudioDevice4,      COMBO_CHANGED,  AUDIO_CHANGED);
	HookWidget(ui->baseResolution,       CBEDIT_CHANGED, VIDEO_RES);
	HookWidget(ui->outputResolution,     CBEDIT_CHANGED, VIDEO_RES);
	HookWidget(ui->downscaleFilter,      COMBO_CHANGED,  VIDEO_CHANGED);
	HookWidget(ui->fpsType,              COMBO_CHANGED,  VIDEO_CHANGED);
	HookWidget(ui->fpsCommon,            COMBO_CHANGED,  VIDEO_CHANGED);
	HookWidget(ui->fpsInteger,           SCROLL_CHANGED, VIDEO_CHANGED);
	HookWidget(ui->fpsNumerator,         SCROLL_CHANGED, VIDEO_CHANGED);
	HookWidget(ui->fpsDenominator,       SCROLL_CHANGED, VIDEO_CHANGED);
	HookWidget(ui->colorsGroupBox,       GROUP_CHANGED,  A11Y_CHANGED);
	HookWidget(ui->colorPreset,          COMBO_CHANGED,  A11Y_CHANGED);
	HookWidget(ui->renderer,             COMBO_CHANGED,  ADV_RESTART);
	HookWidget(ui->adapter,              COMBO_CHANGED,  ADV_RESTART);
	HookWidget(ui->colorFormat,          COMBO_CHANGED,  ADV_CHANGED);
	HookWidget(ui->colorSpace,           COMBO_CHANGED,  ADV_CHANGED);
	HookWidget(ui->colorRange,           COMBO_CHANGED,  ADV_CHANGED);
	HookWidget(ui->sdrWhiteLevel,        SCROLL_CHANGED, ADV_CHANGED);
	HookWidget(ui->hdrNominalPeakLevel,  SCROLL_CHANGED, ADV_CHANGED);
	HookWidget(ui->disableOSXVSync,      CHECK_CHANGED,  ADV_CHANGED);
	HookWidget(ui->resetOSXVSync,        CHECK_CHANGED,  ADV_CHANGED);
	if (obs_audio_monitoring_available())
		HookWidget(ui->monitoringDevice,     COMBO_CHANGED,  ADV_CHANGED);
#ifdef _WIN32
	HookWidget(ui->disableAudioDucking,  CHECK_CHANGED,  ADV_CHANGED);
#endif
#if defined(_WIN32) || defined(__APPLE__)
	HookWidget(ui->browserHWAccel,       CHECK_CHANGED,  ADV_RESTART);
#endif
	HookWidget(ui->filenameFormatting,   EDIT_CHANGED,   ADV_CHANGED);
	HookWidget(ui->overwriteIfExists,    CHECK_CHANGED,  ADV_CHANGED);
	HookWidget(ui->simpleRBPrefix,       EDIT_CHANGED,   ADV_CHANGED);
	HookWidget(ui->simpleRBSuffix,       EDIT_CHANGED,   ADV_CHANGED);
	HookWidget(ui->streamDelayEnable,    CHECK_CHANGED,  ADV_CHANGED);
	HookWidget(ui->streamDelaySec,       SCROLL_CHANGED, ADV_CHANGED);
	HookWidget(ui->streamDelayPreserve,  CHECK_CHANGED,  ADV_CHANGED);
	HookWidget(ui->reconnectEnable,      CHECK_CHANGED,  ADV_CHANGED);
	HookWidget(ui->reconnectRetryDelay,  SCROLL_CHANGED, ADV_CHANGED);
	HookWidget(ui->reconnectMaxRetries,  SCROLL_CHANGED, ADV_CHANGED);
	HookWidget(ui->processPriority,      COMBO_CHANGED,  ADV_CHANGED);
	HookWidget(ui->confirmOnExit,        CHECK_CHANGED,  ADV_CHANGED);
	HookWidget(ui->bindToIP,             COMBO_CHANGED,  ADV_CHANGED);
	HookWidget(ui->ipFamily,             COMBO_CHANGED,  ADV_CHANGED);
	HookWidget(ui->enableNewSocketLoop,  CHECK_CHANGED,  ADV_CHANGED);
	HookWidget(ui->enableLowLatencyMode, CHECK_CHANGED,  ADV_CHANGED);
	HookWidget(ui->hotkeyFocusType,      COMBO_CHANGED,  ADV_CHANGED);
	HookWidget(ui->autoRemux,            CHECK_CHANGED,  ADV_CHANGED);
	HookWidget(ui->dynBitrate,           CHECK_CHANGED,  ADV_CHANGED);
	/* clang-format on */

#define ADD_HOTKEY_FOCUS_TYPE(s) ui->hotkeyFocusType->addItem(QTStr("Basic.Settings.Advanced.Hotkeys." s), s)

	ADD_HOTKEY_FOCUS_TYPE("NeverDisableHotkeys");
	ADD_HOTKEY_FOCUS_TYPE("DisableHotkeysInFocus");
	ADD_HOTKEY_FOCUS_TYPE("DisableHotkeysOutOfFocus");

#undef ADD_HOTKEY_FOCUS_TYPE

	ui->simpleOutputVBitrate->setSingleStep(50);
	ui->simpleOutputVBitrate->setSuffix(" Kbps");
	ui->advOutFFVBitrate->setSingleStep(50);
	ui->advOutFFVBitrate->setSuffix(" Kbps");
	ui->advOutFFABitrate->setSuffix(" Kbps");

#if !defined(_WIN32) && !defined(ENABLE_SPARKLE_UPDATER)
	delete ui->updateSettingsGroupBox;
	ui->updateSettingsGroupBox = nullptr;
	ui->updateChannelLabel = nullptr;
	ui->updateChannelBox = nullptr;
	ui->enableAutoUpdates = nullptr;
#else
	// Hide update section if disabled
	if (App()->IsUpdaterDisabled())
		ui->updateSettingsGroupBox->hide();
#endif

	// Remove the Advanced Audio section if monitoring is not supported, as the monitoring device selection is the only item in the group box.
	if (!obs_audio_monitoring_available()) {
		delete ui->monitoringDeviceLabel;
		ui->monitoringDeviceLabel = nullptr;
		delete ui->monitoringDevice;
		ui->monitoringDevice = nullptr;
	}

#ifdef _WIN32
	if (!SetDisplayAffinitySupported()) {
		delete ui->hideOBSFromCapture;
		ui->hideOBSFromCapture = nullptr;
	}

	static struct ProcessPriority {
		const char *name;
		const char *val;
	} processPriorities[] = {
		{"Basic.Settings.Advanced.General.ProcessPriority.High", "High"},
		{"Basic.Settings.Advanced.General.ProcessPriority.AboveNormal", "AboveNormal"},
		{"Basic.Settings.Advanced.General.ProcessPriority.Normal", "Normal"},
		{"Basic.Settings.Advanced.General.ProcessPriority.BelowNormal", "BelowNormal"},
		{"Basic.Settings.Advanced.General.ProcessPriority.Idle", "Idle"},
	};

	for (ProcessPriority pri : processPriorities)
		ui->processPriority->addItem(QTStr(pri.name), pri.val);

#else
	delete ui->rendererLabel;
	delete ui->renderer;
	delete ui->adapterLabel;
	delete ui->adapter;
	delete ui->processPriorityLabel;
	delete ui->processPriority;
	delete ui->enableNewSocketLoop;
	delete ui->enableLowLatencyMode;
	delete ui->hideOBSFromCapture;
#ifdef __linux__
	delete ui->browserHWAccel;
	delete ui->sourcesGroup;
#endif
	delete ui->disableAudioDucking;

	ui->rendererLabel = nullptr;
	ui->renderer = nullptr;
	ui->adapterLabel = nullptr;
	ui->adapter = nullptr;
	ui->processPriorityLabel = nullptr;
	ui->processPriority = nullptr;
	ui->enableNewSocketLoop = nullptr;
	ui->enableLowLatencyMode = nullptr;
	ui->hideOBSFromCapture = nullptr;
#ifdef __linux__
	ui->browserHWAccel = nullptr;
	ui->sourcesGroup = nullptr;
#endif
	ui->disableAudioDucking = nullptr;
#endif

#ifndef __APPLE__
	delete ui->disableOSXVSync;
	delete ui->resetOSXVSync;
	ui->disableOSXVSync = nullptr;
	ui->resetOSXVSync = nullptr;
#endif

	connect(ui->streamDelaySec, &QSpinBox::valueChanged, this, &OBSBasicSettings::UpdateStreamDelayEstimate);
	connect(ui->outputMode, &QComboBox::currentIndexChanged, this, &OBSBasicSettings::UpdateStreamDelayEstimate);
	connect(ui->simpleOutputVBitrate, &QSpinBox::valueChanged, this, &OBSBasicSettings::UpdateStreamDelayEstimate);
	connect(ui->simpleOutputABitrate, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::UpdateStreamDelayEstimate);
	connect(ui->advOutTrack1Bitrate, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::UpdateStreamDelayEstimate);
	connect(ui->advOutTrack2Bitrate, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::UpdateStreamDelayEstimate);
	connect(ui->advOutTrack3Bitrate, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::UpdateStreamDelayEstimate);
	connect(ui->advOutTrack4Bitrate, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::UpdateStreamDelayEstimate);
	connect(ui->advOutTrack5Bitrate, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::UpdateStreamDelayEstimate);
	connect(ui->advOutTrack6Bitrate, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::UpdateStreamDelayEstimate);

	//Apply button disabled until change.
	EnableApplyButton(false);

	installEventFilter(new SettingsEventFilter());

	LoadColorRanges();
	LoadColorSpaces();
	LoadColorFormats();
	LoadFormats();

	auto ReloadAudioSources = [](void *data, calldata_t *param) {
		auto settings = static_cast<OBSBasicSettings *>(data);
		auto source = static_cast<obs_source_t *>(calldata_ptr(param, "source"));

		if (!source)
			return;

		if (!(obs_source_get_output_flags(source) & OBS_SOURCE_AUDIO))
			return;

		QMetaObject::invokeMethod(settings, "ReloadAudioSources", Qt::QueuedConnection);
	};
	sourceCreated.Connect(obs_get_signal_handler(), "source_create", ReloadAudioSources, this);
	channelChanged.Connect(obs_get_signal_handler(), "channel_change", ReloadAudioSources, this);

	hotkeyConflictIcon = QIcon::fromTheme("obs", QIcon(":/res/images/warning.svg"));

	auto ReloadHotkeys = [](void *data, calldata_t *) {
		auto settings = static_cast<OBSBasicSettings *>(data);
		QMetaObject::invokeMethod(settings, "ReloadHotkeys");
	};
	hotkeyRegistered.Connect(obs_get_signal_handler(), "hotkey_register", ReloadHotkeys, this);

	auto ReloadHotkeysIgnore = [](void *data, calldata_t *param) {
		auto settings = static_cast<OBSBasicSettings *>(data);
		auto key = static_cast<obs_hotkey_t *>(calldata_ptr(param, "key"));
		QMetaObject::invokeMethod(settings, "ReloadHotkeys", Q_ARG(obs_hotkey_id, obs_hotkey_get_id(key)));
	};
	hotkeyUnregistered.Connect(obs_get_signal_handler(), "hotkey_unregister", ReloadHotkeysIgnore, this);

	FillSimpleRecordingValues();
	if (obs_audio_monitoring_available())
		FillAudioMonitoringDevices();

	connect(ui->channelSetup, &QComboBox::currentIndexChanged, this, &OBSBasicSettings::SurroundWarning);
	connect(ui->channelSetup, &QComboBox::currentIndexChanged, this, &OBSBasicSettings::SpeakerLayoutChanged);
	connect(ui->lowLatencyBuffering, &QCheckBox::clicked, this, &OBSBasicSettings::LowLatencyBufferingChanged);
	connect(ui->simpleOutRecQuality, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::SimpleRecordingQualityChanged);
	connect(ui->simpleOutRecQuality, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::SimpleRecordingQualityLosslessWarning);
	connect(ui->simpleOutRecFormat, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::SimpleRecordingEncoderChanged);
	connect(ui->simpleOutStrEncoder, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::SimpleStreamingEncoderChanged);
	connect(ui->simpleOutStrEncoder, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::SimpleRecordingEncoderChanged);
	connect(ui->simpleOutRecEncoder, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::SimpleRecordingEncoderChanged);
	connect(ui->simpleOutRecAEncoder, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::SimpleRecordingEncoderChanged);
	connect(ui->simpleOutputVBitrate, &QSpinBox::valueChanged, this,
		&OBSBasicSettings::SimpleRecordingEncoderChanged);
	connect(ui->simpleOutputABitrate, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::SimpleRecordingEncoderChanged);
	connect(ui->simpleOutAdvanced, &QCheckBox::toggled, this, &OBSBasicSettings::SimpleRecordingEncoderChanged);
	connect(ui->ignoreRecommended, &QCheckBox::toggled, this, &OBSBasicSettings::SimpleRecordingEncoderChanged);
	connect(ui->simpleReplayBuf, &QGroupBox::toggled, this, &OBSBasicSettings::SimpleReplayBufferChanged);
	connect(ui->simpleOutputVBitrate, &QSpinBox::valueChanged, this, &OBSBasicSettings::SimpleReplayBufferChanged);
	connect(ui->simpleOutputABitrate, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::SimpleReplayBufferChanged);
	connect(ui->simpleRBSecMax, &QSpinBox::valueChanged, this, &OBSBasicSettings::SimpleReplayBufferChanged);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
	connect(ui->advOutSplitFile, &QCheckBox::checkStateChanged, this, &OBSBasicSettings::AdvOutSplitFileChanged);
#else
	connect(ui->advOutSplitFile, &QCheckBox::stateChanged, this, &OBSBasicSettings::AdvOutSplitFileChanged);
#endif
	connect(ui->advOutSplitFileType, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::AdvOutSplitFileChanged);
	connect(ui->advReplayBuf, &QCheckBox::toggled, this, &OBSBasicSettings::AdvReplayBufferChanged);
	connect(ui->advOutRecTrack1, &QCheckBox::toggled, this, &OBSBasicSettings::AdvReplayBufferChanged);
	connect(ui->advOutRecTrack2, &QCheckBox::toggled, this, &OBSBasicSettings::AdvReplayBufferChanged);
	connect(ui->advOutRecTrack3, &QCheckBox::toggled, this, &OBSBasicSettings::AdvReplayBufferChanged);
	connect(ui->advOutRecTrack4, &QCheckBox::toggled, this, &OBSBasicSettings::AdvReplayBufferChanged);
	connect(ui->advOutRecTrack5, &QCheckBox::toggled, this, &OBSBasicSettings::AdvReplayBufferChanged);
	connect(ui->advOutRecTrack6, &QCheckBox::toggled, this, &OBSBasicSettings::AdvReplayBufferChanged);
	connect(ui->advOutTrack1Bitrate, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::AdvReplayBufferChanged);
	connect(ui->advOutTrack2Bitrate, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::AdvReplayBufferChanged);
	connect(ui->advOutTrack3Bitrate, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::AdvReplayBufferChanged);
	connect(ui->advOutTrack4Bitrate, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::AdvReplayBufferChanged);
	connect(ui->advOutTrack5Bitrate, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::AdvReplayBufferChanged);
	connect(ui->advOutTrack6Bitrate, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::AdvReplayBufferChanged);
	connect(ui->advOutRecType, &QComboBox::currentIndexChanged, this, &OBSBasicSettings::AdvReplayBufferChanged);
	connect(ui->advOutRecEncoder, &QComboBox::currentIndexChanged, this, &OBSBasicSettings::AdvReplayBufferChanged);
	connect(ui->advRBSecMax, &QSpinBox::valueChanged, this, &OBSBasicSettings::AdvReplayBufferChanged);

	// GPU scaling filters
	auto addScaleFilter = [&](const char *string, int value) -> void {
		ui->advOutRescaleFilter->addItem(QTStr(string), value);
		ui->advOutRecRescaleFilter->addItem(QTStr(string), value);
	};

	addScaleFilter("Basic.Settings.Output.Adv.Rescale.Disabled", OBS_SCALE_DISABLE);
	addScaleFilter("Basic.Settings.Video.DownscaleFilter.Bilinear", OBS_SCALE_BILINEAR);
	addScaleFilter("Basic.Settings.Video.DownscaleFilter.Area", OBS_SCALE_AREA);
	addScaleFilter("Basic.Settings.Video.DownscaleFilter.Bicubic", OBS_SCALE_BICUBIC);
	addScaleFilter("Basic.Settings.Video.DownscaleFilter.Lanczos", OBS_SCALE_LANCZOS);

	auto connectScaleFilter = [&](QComboBox *filter, QComboBox *res) -> void {
		connect(filter, &QComboBox::currentIndexChanged, this,
			[this, res, filter](int) { res->setEnabled(filter->currentData() != OBS_SCALE_DISABLE); });
	};

	connectScaleFilter(ui->advOutRescaleFilter, ui->advOutRescale);
	connectScaleFilter(ui->advOutRecRescaleFilter, ui->advOutRecRescale);

	// Get Bind to IP Addresses
	obs_properties_t *ppts = obs_get_output_properties("rtmp_output");
	obs_property_t *p = obs_properties_get(ppts, "bind_ip");

	size_t count = obs_property_list_item_count(p);
	for (size_t i = 0; i < count; i++) {
		const char *name = obs_property_list_item_name(p, i);
		const char *val = obs_property_list_item_string(p, i);

		ui->bindToIP->addItem(QT_UTF8(name), val);
	}

	// Add IP Family options
	p = obs_properties_get(ppts, "ip_family");

	count = obs_property_list_item_count(p);
	for (size_t i = 0; i < count; i++) {
		const char *name = obs_property_list_item_name(p, i);
		const char *val = obs_property_list_item_string(p, i);

		ui->ipFamily->addItem(QT_UTF8(name), val);
	}

	obs_properties_destroy(ppts);

	ui->multitrackVideoNoticeBox->setVisible(false);

	InitStreamPage();
	InitAppearancePage();
	LoadSettings(false);

	ui->advOutTrack1->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track1"));
	ui->advOutTrack2->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track2"));
	ui->advOutTrack3->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track3"));
	ui->advOutTrack4->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track4"));
	ui->advOutTrack5->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track5"));
	ui->advOutTrack6->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track6"));

	ui->advOutRecTrack1->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track1"));
	ui->advOutRecTrack2->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track2"));
	ui->advOutRecTrack3->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track3"));
	ui->advOutRecTrack4->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track4"));
	ui->advOutRecTrack5->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track5"));
	ui->advOutRecTrack6->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track6"));

	ui->advOutFFTrack1->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track1"));
	ui->advOutFFTrack2->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track2"));
	ui->advOutFFTrack3->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track3"));
	ui->advOutFFTrack4->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track4"));
	ui->advOutFFTrack5->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track5"));
	ui->advOutFFTrack6->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track6"));

	ui->snappingEnabled->setAccessibleName(QTStr("Basic.Settings.General.Snapping"));
	ui->systemTrayEnabled->setAccessibleName(QTStr("Basic.Settings.General.SysTray"));
	ui->label_31->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Recording.RecType"));
	ui->streamDelayEnable->setAccessibleName(QTStr("Basic.Settings.Advanced.StreamDelay"));
	ui->reconnectEnable->setAccessibleName(QTStr("Basic.Settings.Output.Reconnect"));

	// Add warning checks to advanced output recording section controls
	connect(ui->advOutRecTrack1, &QCheckBox::clicked, this, &OBSBasicSettings::AdvOutRecCheckWarnings);
	connect(ui->advOutRecTrack2, &QCheckBox::clicked, this, &OBSBasicSettings::AdvOutRecCheckWarnings);
	connect(ui->advOutRecTrack3, &QCheckBox::clicked, this, &OBSBasicSettings::AdvOutRecCheckWarnings);
	connect(ui->advOutRecTrack4, &QCheckBox::clicked, this, &OBSBasicSettings::AdvOutRecCheckWarnings);
	connect(ui->advOutRecTrack5, &QCheckBox::clicked, this, &OBSBasicSettings::AdvOutRecCheckWarnings);
	connect(ui->advOutRecTrack6, &QCheckBox::clicked, this, &OBSBasicSettings::AdvOutRecCheckWarnings);
	connect(ui->advOutRecFormat, &QComboBox::currentIndexChanged, this, &OBSBasicSettings::AdvOutRecCheckWarnings);
	connect(ui->advOutRecEncoder, &QComboBox::currentIndexChanged, this, &OBSBasicSettings::AdvOutRecCheckWarnings);

	// Check codec compatibility when format (container) changes
	connect(ui->advOutRecFormat, &QComboBox::currentIndexChanged, this, &OBSBasicSettings::AdvOutRecCheckCodecs);

	// Set placeholder used when selection was reset due to incompatibilities
	ui->advOutAEncoder->setPlaceholderText(QTStr("CodecCompat.CodecPlaceholder"));
	ui->advOutRecEncoder->setPlaceholderText(QTStr("CodecCompat.CodecPlaceholder"));
	ui->advOutRecAEncoder->setPlaceholderText(QTStr("CodecCompat.CodecPlaceholder"));
	ui->simpleOutRecEncoder->setPlaceholderText(QTStr("CodecCompat.CodecPlaceholder"));
	ui->simpleOutRecAEncoder->setPlaceholderText(QTStr("CodecCompat.CodecPlaceholder"));
	ui->simpleOutRecFormat->setPlaceholderText(QTStr("CodecCompat.ContainerPlaceholder"));

	SimpleRecordingQualityChanged();
	AdvOutSplitFileChanged();
	AdvOutRecCheckCodecs();
	AdvOutRecCheckWarnings();

	UpdateAutomaticReplayBufferCheckboxes();

	App()->DisableHotkeys();

	channelIndex = ui->channelSetup->currentIndex();
	sampleRateIndex = ui->sampleRate->currentIndex();
	llBufferingEnabled = ui->lowLatencyBuffering->isChecked();

	QRegularExpression rx("\\d{1,5}x\\d{1,5}");
	QValidator *validator = new QRegularExpressionValidator(rx, this);
	ui->baseResolution->lineEdit()->setValidator(validator);
	ui->outputResolution->lineEdit()->setValidator(validator);
	ui->advOutRescale->lineEdit()->setValidator(validator);
	ui->advOutRecRescale->lineEdit()->setValidator(validator);
	ui->advOutFFRescale->lineEdit()->setValidator(validator);

	connect(ui->useStreamKeyAdv, &QCheckBox::clicked, this, &OBSBasicSettings::UseStreamKeyAdvClicked);

	connect(ui->simpleOutStrAEncoder, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::SimpleStreamAudioEncoderChanged);
	connect(ui->advOutAEncoder, &QComboBox::currentIndexChanged, this, &OBSBasicSettings::AdvAudioEncodersChanged);
	connect(ui->advOutRecAEncoder, &QComboBox::currentIndexChanged, this,
		&OBSBasicSettings::AdvAudioEncodersChanged);

	UpdateAudioWarnings();
	UpdateAdvNetworkGroup();

	ui->audioMsg->setVisible(false);
	ui->advancedMsg->setVisible(false);
	ui->advancedMsg2->setVisible(false);
}

OBSBasicSettings::~OBSBasicSettings()
{
	delete ui->filenameFormatting->completer();
	main->EnableOutputs(true);

	App()->UpdateHotkeyFocusSetting();

	EnableThreadedMessageBoxes(false);
}

void OBSBasicSettings::SaveCombo(QComboBox *widget, const char *section, const char *value)
{
	if (WidgetChanged(widget))
		config_set_string(main->Config(), section, value, QT_TO_UTF8(widget->currentText()));
}

void OBSBasicSettings::SaveComboData(QComboBox *widget, const char *section, const char *value)
{
	if (WidgetChanged(widget)) {
		QString str = GetComboData(widget);
		config_set_string(main->Config(), section, value, QT_TO_UTF8(str));
	}
}

void OBSBasicSettings::SaveCheckBox(QAbstractButton *widget, const char *section, const char *value, bool invert)
{
	if (WidgetChanged(widget)) {
		bool checked = widget->isChecked();
		if (invert)
			checked = !checked;

		config_set_bool(main->Config(), section, value, checked);
	}
}

void OBSBasicSettings::SaveEdit(QLineEdit *widget, const char *section, const char *value)
{
	if (WidgetChanged(widget))
		config_set_string(main->Config(), section, value, QT_TO_UTF8(widget->text()));
}

void OBSBasicSettings::SaveSpinBox(QSpinBox *widget, const char *section, const char *value)
{
	if (WidgetChanged(widget))
		config_set_int(main->Config(), section, value, widget->value());
}

void OBSBasicSettings::SaveText(QPlainTextEdit *widget, const char *section, const char *value)
{
	if (!WidgetChanged(widget))
		return;

	auto utf8 = widget->toPlainText().toUtf8();

	OBSDataAutoRelease safe_text = obs_data_create();
	obs_data_set_string(safe_text, "text", utf8.constData());

	config_set_string(main->Config(), section, value, obs_data_get_json(safe_text));
}

std::string DeserializeConfigText(const char *value)
{
	OBSDataAutoRelease data = obs_data_create_from_json(value);
	return obs_data_get_string(data, "text");
}

void OBSBasicSettings::SaveGroupBox(QGroupBox *widget, const char *section, const char *value)
{
	if (WidgetChanged(widget))
		config_set_bool(main->Config(), section, value, widget->isChecked());
}

#define CS_PARTIAL_STR QTStr("Basic.Settings.Advanced.Video.ColorRange.Partial")
#define CS_FULL_STR QTStr("Basic.Settings.Advanced.Video.ColorRange.Full")

void OBSBasicSettings::LoadColorRanges()
{
	ui->colorRange->addItem(CS_PARTIAL_STR, "Partial");
	ui->colorRange->addItem(CS_FULL_STR, "Full");
}

#define CS_SRGB_STR QTStr("Basic.Settings.Advanced.Video.ColorSpace.sRGB")
#define CS_709_STR QTStr("Basic.Settings.Advanced.Video.ColorSpace.709")
#define CS_601_STR QTStr("Basic.Settings.Advanced.Video.ColorSpace.601")
#define CS_2100PQ_STR QTStr("Basic.Settings.Advanced.Video.ColorSpace.2100PQ")
#define CS_2100HLG_STR QTStr("Basic.Settings.Advanced.Video.ColorSpace.2100HLG")

void OBSBasicSettings::LoadColorSpaces()
{
	ui->colorSpace->addItem(CS_SRGB_STR, "sRGB");
	ui->colorSpace->addItem(CS_709_STR, "709");
	ui->colorSpace->addItem(CS_601_STR, "601");
	ui->colorSpace->addItem(CS_2100PQ_STR, "2100PQ");
	ui->colorSpace->addItem(CS_2100HLG_STR, "2100HLG");
}

#define CF_NV12_STR QTStr("Basic.Settings.Advanced.Video.ColorFormat.NV12")
#define CF_I420_STR QTStr("Basic.Settings.Advanced.Video.ColorFormat.I420")
#define CF_I444_STR QTStr("Basic.Settings.Advanced.Video.ColorFormat.I444")
#define CF_P010_STR QTStr("Basic.Settings.Advanced.Video.ColorFormat.P010")
#define CF_I010_STR QTStr("Basic.Settings.Advanced.Video.ColorFormat.I010")
#define CF_P216_STR QTStr("Basic.Settings.Advanced.Video.ColorFormat.P216")
#define CF_P416_STR QTStr("Basic.Settings.Advanced.Video.ColorFormat.P416")
#define CF_BGRA_STR QTStr("Basic.Settings.Advanced.Video.ColorFormat.BGRA")

void OBSBasicSettings::LoadColorFormats()
{
	ui->colorFormat->addItem(CF_NV12_STR, "NV12");
	ui->colorFormat->addItem(CF_I420_STR, "I420");
	ui->colorFormat->addItem(CF_I444_STR, "I444");
	ui->colorFormat->addItem(CF_P010_STR, "P010");
	ui->colorFormat->addItem(CF_I010_STR, "I010");
	ui->colorFormat->addItem(CF_P216_STR, "P216");
	ui->colorFormat->addItem(CF_P416_STR, "P416");
	ui->colorFormat->addItem(CF_BGRA_STR, "RGB"); // Avoid config break
}

#define AV_FORMAT_DEFAULT_STR QTStr("Basic.Settings.Output.Adv.FFmpeg.FormatDefault")
#define AUDIO_STR QTStr("Basic.Settings.Output.Adv.FFmpeg.FormatAudio")
#define VIDEO_STR QTStr("Basic.Settings.Output.Adv.FFmpeg.FormatVideo")

void OBSBasicSettings::LoadFormats()
{
#define FORMAT_STR(str) QTStr("Basic.Settings.Output.Format." str)
	ui->advOutFFFormat->blockSignals(true);

	formats = GetSupportedFormats();

	for (auto &format : formats) {
		bool audio = format.HasAudio();
		bool video = format.HasVideo();

		if (audio || video) {
			QString itemText(format.name);
			if (audio ^ video)
				itemText += QStringLiteral(" (%1)").arg(audio ? AUDIO_STR : VIDEO_STR);

			ui->advOutFFFormat->addItem(itemText, QVariant::fromValue(format));
		}
	}

	ui->advOutFFFormat->model()->sort(0);

	ui->advOutFFFormat->insertItem(0, AV_FORMAT_DEFAULT_STR);

	ui->advOutFFFormat->blockSignals(false);

	ui->simpleOutRecFormat->addItem(FORMAT_STR("FLV"), "flv");
	ui->simpleOutRecFormat->addItem(FORMAT_STR("MKV"), "mkv");
	ui->simpleOutRecFormat->addItem(FORMAT_STR("MP4"), "mp4");
	ui->simpleOutRecFormat->addItem(FORMAT_STR("MOV"), "mov");
	ui->simpleOutRecFormat->addItem(FORMAT_STR("hMP4"), "hybrid_mp4");
	ui->simpleOutRecFormat->addItem(FORMAT_STR("fMP4"), "fragmented_mp4");
	ui->simpleOutRecFormat->addItem(FORMAT_STR("fMOV"), "fragmented_mov");
	ui->simpleOutRecFormat->addItem(FORMAT_STR("TS"), "mpegts");

	ui->advOutRecFormat->addItem(FORMAT_STR("FLV"), "flv");
	ui->advOutRecFormat->addItem(FORMAT_STR("MKV"), "mkv");
	ui->advOutRecFormat->addItem(FORMAT_STR("MP4"), "mp4");
	ui->advOutRecFormat->addItem(FORMAT_STR("MOV"), "mov");
	ui->advOutRecFormat->addItem(FORMAT_STR("hMP4"), "hybrid_mp4");
	ui->advOutRecFormat->addItem(FORMAT_STR("fMP4"), "fragmented_mp4");
	ui->advOutRecFormat->addItem(FORMAT_STR("fMOV"), "fragmented_mov");
	ui->advOutRecFormat->addItem(FORMAT_STR("TS"), "mpegts");
	ui->advOutRecFormat->addItem(FORMAT_STR("HLS"), "hls");

#undef FORMAT_STR
}

static void AddCodec(QComboBox *combo, const FFmpegCodec &codec)
{
	QString itemText;
	if (codec.long_name)
		itemText = QStringLiteral("%1 - %2").arg(codec.name, codec.long_name);
	else
		itemText = codec.name;

	combo->addItem(itemText, QVariant::fromValue(codec));
}

#define AV_ENCODER_DEFAULT_STR QTStr("Basic.Settings.Output.Adv.FFmpeg.AVEncoderDefault")

static void AddDefaultCodec(QComboBox *combo, const FFmpegFormat &format, FFmpegCodecType codecType)
{
	FFmpegCodec codec = format.GetDefaultEncoder(codecType);

	int existingIdx = FindEncoder(combo, codec.name, codec.id);
	if (existingIdx >= 0)
		combo->removeItem(existingIdx);

	QString itemText;
	if (codec.long_name) {
		itemText = QStringLiteral("%1 - %2 (%3)").arg(codec.name, codec.long_name, AV_ENCODER_DEFAULT_STR);
	} else {
		itemText = QStringLiteral("%1 (%2)").arg(codec.name, AV_ENCODER_DEFAULT_STR);
	}

	combo->addItem(itemText, QVariant::fromValue(codec));
}

#define AV_ENCODER_DISABLE_STR QTStr("Basic.Settings.Output.Adv.FFmpeg.AVEncoderDisable")

void OBSBasicSettings::ReloadCodecs(const FFmpegFormat &format)
{
	ui->advOutFFAEncoder->blockSignals(true);
	ui->advOutFFVEncoder->blockSignals(true);
	ui->advOutFFAEncoder->clear();
	ui->advOutFFVEncoder->clear();

	bool ignore_compatibility = ui->advOutFFIgnoreCompat->isChecked();
	vector<FFmpegCodec> supportedCodecs = GetFormatCodecs(format, ignore_compatibility);

	for (auto &codec : supportedCodecs) {
		switch (codec.type) {
		case FFmpegCodecType::AUDIO:
			AddCodec(ui->advOutFFAEncoder, codec);
			break;
		case FFmpegCodecType::VIDEO:
			AddCodec(ui->advOutFFVEncoder, codec);
			break;
		default:
			break;
		}
	}

	if (format.HasAudio())
		AddDefaultCodec(ui->advOutFFAEncoder, format, FFmpegCodecType::AUDIO);
	if (format.HasVideo())
		AddDefaultCodec(ui->advOutFFVEncoder, format, FFmpegCodecType::VIDEO);

	ui->advOutFFAEncoder->model()->sort(0);
	ui->advOutFFVEncoder->model()->sort(0);

	QVariant disable = QVariant::fromValue(FFmpegCodec());

	ui->advOutFFAEncoder->insertItem(0, AV_ENCODER_DISABLE_STR, disable);
	ui->advOutFFVEncoder->insertItem(0, AV_ENCODER_DISABLE_STR, disable);

	ui->advOutFFAEncoder->blockSignals(false);
	ui->advOutFFVEncoder->blockSignals(false);
}

void OBSBasicSettings::LoadLanguageList()
{
	const char *currentLang = App()->GetLocale();

	ui->language->clear();

	for (const auto &locale : GetLocaleNames()) {
		int idx = ui->language->count();

		ui->language->addItem(QT_UTF8(locale.second.c_str()), QT_UTF8(locale.first.c_str()));

		if (locale.first == currentLang)
			ui->language->setCurrentIndex(idx);
	}

	ui->language->model()->sort(0);
}

#if defined(_WIN32) || defined(ENABLE_SPARKLE_UPDATER)
void TranslateBranchInfo(const QString &name, QString &displayName, QString &description)
{
	QString translatedName = QTStr("Basic.Settings.General.ChannelName." + name.toUtf8());
	QString translatedDesc = QTStr("Basic.Settings.General.ChannelDescription." + name.toUtf8());

	if (!translatedName.startsWith("Basic.Settings."))
		displayName = translatedName;
	if (!translatedDesc.startsWith("Basic.Settings."))
		description = translatedDesc;
}
#endif

void OBSBasicSettings::LoadBranchesList()
{
#if defined(_WIN32) || defined(ENABLE_SPARKLE_UPDATER)
	bool configBranchRemoved = true;
	QString configBranch = config_get_string(App()->GetAppConfig(), "General", "UpdateBranch");

	for (const UpdateBranch &branch : App()->GetBranches()) {
		if (branch.name == configBranch)
			configBranchRemoved = false;
		if (!branch.is_visible && branch.name != configBranch)
			continue;

		QString displayName = branch.display_name;
		QString description = branch.description;

		TranslateBranchInfo(branch.name, displayName, description);
		QString itemDesc = displayName + " - " + description;

		if (!branch.is_enabled) {
			itemDesc.prepend(" ");
			itemDesc.prepend(QTStr("Basic.Settings.General.UpdateChannelDisabled"));
		} else if (branch.name == "stable") {
			itemDesc.append(" ");
			itemDesc.append(QTStr("Basic.Settings.General.UpdateChannelDefault"));
		}

		ui->updateChannelBox->addItem(itemDesc, branch.name);

		// Disable item if branch is disabled
		if (!branch.is_enabled) {
			QStandardItemModel *model = dynamic_cast<QStandardItemModel *>(ui->updateChannelBox->model());
			QStandardItem *item = model->item(ui->updateChannelBox->count() - 1);
			item->setFlags(Qt::NoItemFlags);
		}
	}

	// Fall back to default if not yet set or user-selected branch has been removed
	if (configBranch.isEmpty() || configBranchRemoved)
		configBranch = "stable";

	int idx = ui->updateChannelBox->findData(configBranch);
	ui->updateChannelBox->setCurrentIndex(idx);
#endif
}

void OBSBasicSettings::LoadGeneralSettings()
{
	loading = true;

	LoadLanguageList();

#if defined(_WIN32) || defined(ENABLE_SPARKLE_UPDATER)
	bool enableAutoUpdates = config_get_bool(App()->GetAppConfig(), "General", "EnableAutoUpdates");
	ui->enableAutoUpdates->setChecked(enableAutoUpdates);

	LoadBranchesList();
#endif
	bool openStatsOnStartup = config_get_bool(main->Config(), "General", "OpenStatsOnStartup");
	ui->openStatsOnStartup->setChecked(openStatsOnStartup);

#if defined(_WIN32)
	if (ui->hideOBSFromCapture) {
		bool hideWindowFromCapture =
			config_get_bool(App()->GetUserConfig(), "BasicWindow", "HideOBSWindowsFromCapture");
		ui->hideOBSFromCapture->setChecked(hideWindowFromCapture);

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
		connect(ui->hideOBSFromCapture, &QCheckBox::checkStateChanged, this,
			&OBSBasicSettings::HideOBSWindowWarning);
#else
		connect(ui->hideOBSFromCapture, &QCheckBox::stateChanged, this,
			&OBSBasicSettings::HideOBSWindowWarning);
#endif
	}
#endif

	bool recordWhenStreaming = config_get_bool(App()->GetUserConfig(), "BasicWindow", "RecordWhenStreaming");
	ui->recordWhenStreaming->setChecked(recordWhenStreaming);

	bool keepRecordStreamStops =
		config_get_bool(App()->GetUserConfig(), "BasicWindow", "KeepRecordingWhenStreamStops");
	ui->keepRecordStreamStops->setChecked(keepRecordStreamStops);

	bool replayWhileStreaming =
		config_get_bool(App()->GetUserConfig(), "BasicWindow", "ReplayBufferWhileStreaming");
	ui->replayWhileStreaming->setChecked(replayWhileStreaming);

	bool keepReplayStreamStops =
		config_get_bool(App()->GetUserConfig(), "BasicWindow", "KeepReplayBufferStreamStops");
	ui->keepReplayStreamStops->setChecked(keepReplayStreamStops);

	bool systemTrayEnabled = config_get_bool(App()->GetUserConfig(), "BasicWindow", "SysTrayEnabled");
	ui->systemTrayEnabled->setChecked(systemTrayEnabled);

	bool systemTrayWhenStarted = config_get_bool(App()->GetUserConfig(), "BasicWindow", "SysTrayWhenStarted");
	ui->systemTrayWhenStarted->setChecked(systemTrayWhenStarted);

	bool systemTrayAlways = config_get_bool(App()->GetUserConfig(), "BasicWindow", "SysTrayMinimizeToTray");
	ui->systemTrayAlways->setChecked(systemTrayAlways);

	bool saveProjectors = config_get_bool(App()->GetUserConfig(), "BasicWindow", "SaveProjectors");
	ui->saveProjectors->setChecked(saveProjectors);

	bool closeProjectors = config_get_bool(App()->GetUserConfig(), "BasicWindow", "CloseExistingProjectors");
	ui->closeProjectors->setChecked(closeProjectors);

	bool snappingEnabled = config_get_bool(App()->GetUserConfig(), "BasicWindow", "SnappingEnabled");
	ui->snappingEnabled->setChecked(snappingEnabled);

	bool screenSnapping = config_get_bool(App()->GetUserConfig(), "BasicWindow", "ScreenSnapping");
	ui->screenSnapping->setChecked(screenSnapping);

	bool centerSnapping = config_get_bool(App()->GetUserConfig(), "BasicWindow", "CenterSnapping");
	ui->centerSnapping->setChecked(centerSnapping);

	bool sourceSnapping = config_get_bool(App()->GetUserConfig(), "BasicWindow", "SourceSnapping");
	ui->sourceSnapping->setChecked(sourceSnapping);

	double snapDistance = config_get_double(App()->GetUserConfig(), "BasicWindow", "SnapDistance");
	ui->snapDistance->setValue(snapDistance);

	bool warnBeforeStreamStart = config_get_bool(App()->GetUserConfig(), "BasicWindow", "WarnBeforeStartingStream");
	ui->warnBeforeStreamStart->setChecked(warnBeforeStreamStart);

	bool spacingHelpersEnabled = config_get_bool(App()->GetUserConfig(), "BasicWindow", "SpacingHelpersEnabled");
	ui->previewSpacingHelpers->setChecked(spacingHelpersEnabled);

	bool warnBeforeStreamStop = config_get_bool(App()->GetUserConfig(), "BasicWindow", "WarnBeforeStoppingStream");
	ui->warnBeforeStreamStop->setChecked(warnBeforeStreamStop);

	bool warnBeforeRecordStop = config_get_bool(App()->GetUserConfig(), "BasicWindow", "WarnBeforeStoppingRecord");
	ui->warnBeforeRecordStop->setChecked(warnBeforeRecordStop);

	bool hideProjectorCursor = config_get_bool(App()->GetUserConfig(), "BasicWindow", "HideProjectorCursor");
	ui->hideProjectorCursor->setChecked(hideProjectorCursor);

	bool projectorAlwaysOnTop = config_get_bool(App()->GetUserConfig(), "BasicWindow", "ProjectorAlwaysOnTop");
	ui->projectorAlwaysOnTop->setChecked(projectorAlwaysOnTop);

	bool overflowHide = config_get_bool(App()->GetUserConfig(), "BasicWindow", "OverflowHidden");
	ui->overflowHide->setChecked(overflowHide);

	bool overflowAlwaysVisible = config_get_bool(App()->GetUserConfig(), "BasicWindow", "OverflowAlwaysVisible");
	ui->overflowAlwaysVisible->setChecked(overflowAlwaysVisible);

	bool overflowSelectionHide = config_get_bool(App()->GetUserConfig(), "BasicWindow", "OverflowSelectionHidden");
	ui->overflowSelectionHide->setChecked(overflowSelectionHide);

	bool safeAreas = config_get_bool(App()->GetUserConfig(), "BasicWindow", "ShowSafeAreas");
	ui->previewSafeAreas->setChecked(safeAreas);

	bool automaticSearch = config_get_bool(App()->GetUserConfig(), "General", "AutomaticCollectionSearch");
	ui->automaticSearch->setChecked(automaticSearch);

	bool doubleClickSwitch = config_get_bool(App()->GetUserConfig(), "BasicWindow", "TransitionOnDoubleClick");
	ui->doubleClickSwitch->setChecked(doubleClickSwitch);

	bool studioPortraitLayout = config_get_bool(App()->GetUserConfig(), "BasicWindow", "StudioPortraitLayout");
	ui->studioPortraitLayout->setChecked(studioPortraitLayout);

	bool prevProgLabels = config_get_bool(App()->GetUserConfig(), "BasicWindow", "StudioModeLabels");
	ui->prevProgLabelToggle->setChecked(prevProgLabels);

	bool multiviewMouseSwitch = config_get_bool(App()->GetUserConfig(), "BasicWindow", "MultiviewMouseSwitch");
	ui->multiviewMouseSwitch->setChecked(multiviewMouseSwitch);

	bool multiviewDrawNames = config_get_bool(App()->GetUserConfig(), "BasicWindow", "MultiviewDrawNames");
	ui->multiviewDrawNames->setChecked(multiviewDrawNames);

	bool multiviewDrawAreas = config_get_bool(App()->GetUserConfig(), "BasicWindow", "MultiviewDrawAreas");
	ui->multiviewDrawAreas->setChecked(multiviewDrawAreas);

	ui->multiviewLayout->addItem(QTStr("Basic.Settings.General.MultiviewLayout.Horizontal.Top"),
				     static_cast<int>(MultiviewLayout::HORIZONTAL_TOP_8_SCENES));
	ui->multiviewLayout->addItem(QTStr("Basic.Settings.General.MultiviewLayout.Horizontal.Bottom"),
				     static_cast<int>(MultiviewLayout::HORIZONTAL_BOTTOM_8_SCENES));
	ui->multiviewLayout->addItem(QTStr("Basic.Settings.General.MultiviewLayout.Vertical.Left"),
				     static_cast<int>(MultiviewLayout::VERTICAL_LEFT_8_SCENES));
	ui->multiviewLayout->addItem(QTStr("Basic.Settings.General.MultiviewLayout.Vertical.Right"),
				     static_cast<int>(MultiviewLayout::VERTICAL_RIGHT_8_SCENES));
	ui->multiviewLayout->addItem(QTStr("Basic.Settings.General.MultiviewLayout.Horizontal.18Scene.Top"),
				     static_cast<int>(MultiviewLayout::HORIZONTAL_TOP_18_SCENES));
	ui->multiviewLayout->addItem(QTStr("Basic.Settings.General.MultiviewLayout.Horizontal.Extended.Top"),
				     static_cast<int>(MultiviewLayout::HORIZONTAL_TOP_24_SCENES));
	ui->multiviewLayout->addItem(QTStr("Basic.Settings.General.MultiviewLayout.4Scene"),
				     static_cast<int>(MultiviewLayout::SCENES_ONLY_4_SCENES));
	ui->multiviewLayout->addItem(QTStr("Basic.Settings.General.MultiviewLayout.9Scene"),
				     static_cast<int>(MultiviewLayout::SCENES_ONLY_9_SCENES));
	ui->multiviewLayout->addItem(QTStr("Basic.Settings.General.MultiviewLayout.16Scene"),
				     static_cast<int>(MultiviewLayout::SCENES_ONLY_16_SCENES));
	ui->multiviewLayout->addItem(QTStr("Basic.Settings.General.MultiviewLayout.25Scene"),
				     static_cast<int>(MultiviewLayout::SCENES_ONLY_25_SCENES));

	ui->multiviewLayout->setCurrentIndex(ui->multiviewLayout->findData(
		QVariant::fromValue(config_get_int(App()->GetUserConfig(), "BasicWindow", "MultiviewLayout"))));

	prevLangIndex = ui->language->currentIndex();

	if (obs_video_active())
		ui->language->setEnabled(false);

	loading = false;
}

void OBSBasicSettings::LoadRendererList()
{
#ifdef _WIN32
	const char *renderer = config_get_string(App()->GetAppConfig(), "Video", "Renderer");

	ui->renderer->addItem(QT_UTF8("Direct3D 11"));
	if (opt_allow_opengl || strcmp(renderer, "OpenGL") == 0)
		ui->renderer->addItem(QT_UTF8("OpenGL"));

	int idx = ui->renderer->findText(QT_UTF8(renderer));
	if (idx == -1)
		idx = 0;

	// the video adapter selection is not currently implemented, hide for now
	// to avoid user confusion. was previously protected by
	// if (strcmp(renderer, "OpenGL") == 0)
	delete ui->adapter;
	delete ui->adapterLabel;
	ui->adapter = nullptr;
	ui->adapterLabel = nullptr;

	ui->renderer->setCurrentIndex(idx);
#endif
}

static string ResString(uint32_t cx, uint32_t cy)
{
	stringstream res;
	res << cx << "x" << cy;
	return res.str();
}

/* some nice default output resolution vals */
static const double vals[] = {1.0, 1.25, (1.0 / 0.75), 1.5, (1.0 / 0.6), 1.75, 2.0, 2.25, 2.5, 2.75, 3.0};

static const size_t numVals = sizeof(vals) / sizeof(double);

void OBSBasicSettings::ResetDownscales(uint32_t cx, uint32_t cy, bool ignoreAllSignals)
{
	QString advRescale;
	QString advRecRescale;
	QString advFFRescale;
	QString oldOutputRes;
	string bestScale;
	int bestPixelDiff = 0x7FFFFFFF;
	uint32_t out_cx = outputCX;
	uint32_t out_cy = outputCY;

	advRescale = ui->advOutRescale->lineEdit()->text();
	advRecRescale = ui->advOutRecRescale->lineEdit()->text();
	advFFRescale = ui->advOutFFRescale->lineEdit()->text();

	bool lockedOutputRes = !ui->outputResolution->isEditable();

	if (!lockedOutputRes) {
		ui->outputResolution->blockSignals(true);
		ui->outputResolution->clear();
	}
	if (ignoreAllSignals) {
		ui->advOutRescale->blockSignals(true);
		ui->advOutRecRescale->blockSignals(true);
		ui->advOutFFRescale->blockSignals(true);
	}
	ui->advOutRescale->clear();
	ui->advOutRecRescale->clear();
	ui->advOutFFRescale->clear();

	if (!out_cx || !out_cy) {
		out_cx = cx;
		out_cy = cy;
		oldOutputRes = ui->baseResolution->lineEdit()->text();
	} else {
		oldOutputRes = QString::number(out_cx) + "x" + QString::number(out_cy);
	}

	for (size_t idx = 0; idx < numVals; idx++) {
		uint32_t downscaleCX = uint32_t(double(cx) / vals[idx]);
		uint32_t downscaleCY = uint32_t(double(cy) / vals[idx]);
		uint32_t outDownscaleCX = uint32_t(double(out_cx) / vals[idx]);
		uint32_t outDownscaleCY = uint32_t(double(out_cy) / vals[idx]);

		downscaleCX &= 0xFFFFFFFC;
		downscaleCY &= 0xFFFFFFFE;
		outDownscaleCX &= 0xFFFFFFFE;
		outDownscaleCY &= 0xFFFFFFFE;

		string res = ResString(downscaleCX, downscaleCY);
		string outRes = ResString(outDownscaleCX, outDownscaleCY);
		if (!lockedOutputRes)
			ui->outputResolution->addItem(res.c_str());
		ui->advOutRescale->addItem(outRes.c_str());
		ui->advOutRecRescale->addItem(outRes.c_str());
		ui->advOutFFRescale->addItem(outRes.c_str());

		/* always try to find the closest output resolution to the
		 * previously set output resolution */
		int newPixelCount = int(downscaleCX * downscaleCY);
		int oldPixelCount = int(out_cx * out_cy);
		int diff = abs(newPixelCount - oldPixelCount);

		if (diff < bestPixelDiff) {
			bestScale = res;
			bestPixelDiff = diff;
		}
	}

	string res = ResString(cx, cy);

	if (!lockedOutputRes) {
		float baseAspect = float(cx) / float(cy);
		float outputAspect = float(out_cx) / float(out_cy);
		bool closeAspect = close_float(baseAspect, outputAspect, 0.01f);

		if (closeAspect) {
			ui->outputResolution->lineEdit()->setText(oldOutputRes);
			on_outputResolution_editTextChanged(oldOutputRes);
		} else {
			ui->outputResolution->lineEdit()->setText(bestScale.c_str());
			on_outputResolution_editTextChanged(bestScale.c_str());
		}

		ui->outputResolution->blockSignals(false);

		if (!closeAspect) {
			ui->outputResolution->setProperty("changed", QVariant(true));
			videoChanged = true;
		}
	}

	if (advRescale.isEmpty())
		advRescale = res.c_str();
	if (advRecRescale.isEmpty())
		advRecRescale = res.c_str();
	if (advFFRescale.isEmpty())
		advFFRescale = res.c_str();

	ui->advOutRescale->lineEdit()->setText(advRescale);
	ui->advOutRecRescale->lineEdit()->setText(advRecRescale);
	ui->advOutFFRescale->lineEdit()->setText(advFFRescale);

	if (ignoreAllSignals) {
		ui->advOutRescale->blockSignals(false);
		ui->advOutRecRescale->blockSignals(false);
		ui->advOutFFRescale->blockSignals(false);
	}
}

void OBSBasicSettings::LoadDownscaleFilters()
{
	QString downscaleFilter = ui->downscaleFilter->currentData().toString();
	if (downscaleFilter.isEmpty())
		downscaleFilter = config_get_string(main->Config(), "Video", "ScaleType");

	ui->downscaleFilter->clear();
	if (ui->baseResolution->currentText() == ui->outputResolution->currentText()) {
		ui->downscaleFilter->setEnabled(false);
		ui->downscaleFilter->addItem(QTStr("Basic.Settings.Video.DownscaleFilter.Unavailable"),
					     downscaleFilter);
	} else {
		ui->downscaleFilter->setEnabled(true);
		ui->downscaleFilter->addItem(QTStr("Basic.Settings.Video.DownscaleFilter.Bilinear"),
					     QT_UTF8("bilinear"));
		ui->downscaleFilter->addItem(QTStr("Basic.Settings.Video.DownscaleFilter.Area"), QT_UTF8("area"));
		ui->downscaleFilter->addItem(QTStr("Basic.Settings.Video.DownscaleFilter.Bicubic"), QT_UTF8("bicubic"));
		ui->downscaleFilter->addItem(QTStr("Basic.Settings.Video.DownscaleFilter.Lanczos"), QT_UTF8("lanczos"));

		if (downscaleFilter == "bilinear")
			ui->downscaleFilter->setCurrentIndex(0);
		else if (downscaleFilter == "lanczos")
			ui->downscaleFilter->setCurrentIndex(3);
		else if (downscaleFilter == "area")
			ui->downscaleFilter->setCurrentIndex(1);
		else
			ui->downscaleFilter->setCurrentIndex(2);
	}
}

void OBSBasicSettings::LoadResolutionLists()
{
	uint32_t cx = config_get_uint(main->Config(), "Video", "BaseCX");
	uint32_t cy = config_get_uint(main->Config(), "Video", "BaseCY");
	uint32_t out_cx = config_get_uint(main->Config(), "Video", "OutputCX");
	uint32_t out_cy = config_get_uint(main->Config(), "Video", "OutputCY");

	ui->baseResolution->clear();

	auto addRes = [this](int cx, int cy) {
		QString res = ResString(cx, cy).c_str();
		if (ui->baseResolution->findText(res) == -1)
			ui->baseResolution->addItem(res);
	};

	for (QScreen *screen : QGuiApplication::screens()) {
		QSize as = screen->size();
		uint32_t as_width = as.width();
		uint32_t as_height = as.height();

		// Calculate physical screen resolution based on the virtual screen resolution
		// They might differ if scaling is enabled, e.g. for HiDPI screens
		as_width = round(as_width * screen->devicePixelRatio());
		as_height = round(as_height * screen->devicePixelRatio());

		addRes(as_width, as_height);
	}

	addRes(1920, 1080);
	addRes(1280, 720);

	string outputResString = ResString(out_cx, out_cy);

	ui->baseResolution->lineEdit()->setText(ResString(cx, cy).c_str());

	RecalcOutputResPixels(outputResString.c_str());
	ResetDownscales(cx, cy);

	ui->outputResolution->lineEdit()->setText(outputResString.c_str());

	std::tuple<int, int> aspect = aspect_ratio(cx, cy);

	ui->baseAspect->setText(
		QTStr("AspectRatio").arg(QString::number(std::get<0>(aspect)), QString::number(std::get<1>(aspect))));
}

static inline void LoadFPSCommon(OBSBasic *main, Ui::OBSBasicSettings *ui)
{
	const char *val = config_get_string(main->Config(), "Video", "FPSCommon");

	int idx = ui->fpsCommon->findText(val);
	if (idx == -1)
		idx = 4;
	ui->fpsCommon->setCurrentIndex(idx);
}

static inline void LoadFPSInteger(OBSBasic *main, Ui::OBSBasicSettings *ui)
{
	uint32_t val = config_get_uint(main->Config(), "Video", "FPSInt");
	ui->fpsInteger->setValue(val);
}

static inline void LoadFPSFraction(OBSBasic *main, Ui::OBSBasicSettings *ui)
{
	uint32_t num = config_get_uint(main->Config(), "Video", "FPSNum");
	uint32_t den = config_get_uint(main->Config(), "Video", "FPSDen");

	ui->fpsNumerator->setValue(num);
	ui->fpsDenominator->setValue(den);
}

void OBSBasicSettings::LoadFPSData()
{
	LoadFPSCommon(main, ui.get());
	LoadFPSInteger(main, ui.get());
	LoadFPSFraction(main, ui.get());

	uint32_t fpsType = config_get_uint(main->Config(), "Video", "FPSType");
	if (fpsType > 2)
		fpsType = 0;

	ui->fpsType->setCurrentIndex(fpsType);
	ui->fpsTypes->setCurrentIndex(fpsType);
}

void OBSBasicSettings::LoadVideoSettings()
{
	loading = true;

	if (obs_video_active()) {
		ui->videoPage->setEnabled(false);
		ui->videoMsg->setText(QTStr("Basic.Settings.Video.CurrentlyActive"));
	}

	LoadResolutionLists();
	LoadFPSData();
	LoadDownscaleFilters();

	loading = false;
}

static inline bool IsSurround(const char *speakers)
{
	static const char *surroundLayouts[] = {"2.1", "4.0", "4.1", "5.1", "7.1", nullptr};

	if (!speakers || !*speakers)
		return false;

	const char **curLayout = surroundLayouts;
	for (; *curLayout; ++curLayout) {
		if (strcmp(*curLayout, speakers) == 0) {
			return true;
		}
	}

	return false;
}

void OBSBasicSettings::LoadSimpleOutputSettings()
{
	const char *path = config_get_string(main->Config(), "SimpleOutput", "FilePath");
	bool noSpace = config_get_bool(main->Config(), "SimpleOutput", "FileNameWithoutSpace");
	const char *format = config_get_string(main->Config(), "SimpleOutput", "RecFormat2");
	int videoBitrate = config_get_uint(main->Config(), "SimpleOutput", "VBitrate");
	const char *streamEnc = config_get_string(main->Config(), "SimpleOutput", "StreamEncoder");
	const char *streamAudioEnc = config_get_string(main->Config(), "SimpleOutput", "StreamAudioEncoder");
	int audioBitrate = config_get_uint(main->Config(), "SimpleOutput", "ABitrate");
	bool advanced = config_get_bool(main->Config(), "SimpleOutput", "UseAdvanced");
	const char *preset = config_get_string(main->Config(), "SimpleOutput", "Preset");
	const char *qsvPreset = config_get_string(main->Config(), "SimpleOutput", "QSVPreset");
	const char *nvPreset = config_get_string(main->Config(), "SimpleOutput", "NVENCPreset2");
	const char *amdPreset = config_get_string(main->Config(), "SimpleOutput", "AMDPreset");
	const char *amdAV1Preset = config_get_string(main->Config(), "SimpleOutput", "AMDAV1Preset");
	const char *custom = config_get_string(main->Config(), "SimpleOutput", "x264Settings");
	const char *recQual = config_get_string(main->Config(), "SimpleOutput", "RecQuality");
	const char *recEnc = config_get_string(main->Config(), "SimpleOutput", "RecEncoder");
	const char *recAudioEnc = config_get_string(main->Config(), "SimpleOutput", "RecAudioEncoder");
	const char *muxCustom = config_get_string(main->Config(), "SimpleOutput", "MuxerCustom");
	bool replayBuf = config_get_bool(main->Config(), "SimpleOutput", "RecRB");
	int rbTime = config_get_int(main->Config(), "SimpleOutput", "RecRBTime");
	int rbSize = config_get_int(main->Config(), "SimpleOutput", "RecRBSize");
	int tracks = config_get_int(main->Config(), "SimpleOutput", "RecTracks");

	ui->simpleOutRecTrack1->setChecked(tracks & (1 << 0));
	ui->simpleOutRecTrack2->setChecked(tracks & (1 << 1));
	ui->simpleOutRecTrack3->setChecked(tracks & (1 << 2));
	ui->simpleOutRecTrack4->setChecked(tracks & (1 << 3));
	ui->simpleOutRecTrack5->setChecked(tracks & (1 << 4));
	ui->simpleOutRecTrack6->setChecked(tracks & (1 << 5));

	curPreset = preset;
	curQSVPreset = qsvPreset;
	curNVENCPreset = nvPreset;
	curAMDPreset = amdPreset;
	curAMDAV1Preset = amdAV1Preset;

	bool isOpus = strcmp(streamAudioEnc, "opus") == 0;
	audioBitrate = isOpus ? FindClosestAvailableSimpleOpusBitrate(audioBitrate)
			      : FindClosestAvailableSimpleAACBitrate(audioBitrate);

	ui->simpleOutputPath->setText(path);
	ui->simpleNoSpace->setChecked(noSpace);
	ui->simpleOutputVBitrate->setValue(videoBitrate);

	int idx = ui->simpleOutRecFormat->findData(format);
	ui->simpleOutRecFormat->setCurrentIndex(idx);

	PopulateSimpleBitrates(ui->simpleOutputABitrate, isOpus);

	const char *speakers = config_get_string(main->Config(), "Audio", "ChannelSetup");

	// restrict list of bitrates when multichannel is OFF
	if (!IsSurround(speakers))
		RestrictResetBitrates({ui->simpleOutputABitrate}, 320);

	SetComboByName(ui->simpleOutputABitrate, std::to_string(audioBitrate).c_str());

	ui->simpleOutAdvanced->setChecked(advanced);
	ui->simpleOutCustom->setText(custom);

	idx = ui->simpleOutRecQuality->findData(QString(recQual));
	if (idx == -1)
		idx = 0;
	ui->simpleOutRecQuality->setCurrentIndex(idx);

	idx = ui->simpleOutStrEncoder->findData(QString(streamEnc));
	if (idx == -1)
		idx = 0;
	ui->simpleOutStrEncoder->setCurrentIndex(idx);

	idx = ui->simpleOutStrAEncoder->findData(QString(streamAudioEnc));
	if (idx == -1)
		idx = 0;
	ui->simpleOutStrAEncoder->setCurrentIndex(idx);

	idx = ui->simpleOutRecEncoder->findData(QString(recEnc));
	ui->simpleOutRecEncoder->setCurrentIndex(idx);

	idx = ui->simpleOutRecAEncoder->findData(QString(recAudioEnc));
	ui->simpleOutRecAEncoder->setCurrentIndex(idx);

	ui->simpleOutMuxCustom->setText(muxCustom);

	ui->simpleReplayBuf->setChecked(replayBuf);
	ui->simpleRBSecMax->setValue(rbTime);
	ui->simpleRBMegsMax->setValue(rbSize);

	SimpleStreamingEncoderChanged();
}

static inline QString makeFormatToolTip()
{
	static const char *format_list[][2] = {
		{"CCYY", "FilenameFormatting.TT.CCYY"}, {"YY", "FilenameFormatting.TT.YY"},
		{"MM", "FilenameFormatting.TT.MM"},     {"DD", "FilenameFormatting.TT.DD"},
		{"hh", "FilenameFormatting.TT.hh"},     {"mm", "FilenameFormatting.TT.mm"},
		{"ss", "FilenameFormatting.TT.ss"},     {"%", "FilenameFormatting.TT.Percent"},
		{"a", "FilenameFormatting.TT.a"},       {"A", "FilenameFormatting.TT.A"},
		{"b", "FilenameFormatting.TT.b"},       {"B", "FilenameFormatting.TT.B"},
		{"d", "FilenameFormatting.TT.d"},       {"H", "FilenameFormatting.TT.H"},
		{"I", "FilenameFormatting.TT.I"},       {"m", "FilenameFormatting.TT.m"},
		{"M", "FilenameFormatting.TT.M"},       {"p", "FilenameFormatting.TT.p"},
		{"s", "FilenameFormatting.TT.s"},       {"S", "FilenameFormatting.TT.S"},
		{"y", "FilenameFormatting.TT.y"},       {"Y", "FilenameFormatting.TT.Y"},
		{"z", "FilenameFormatting.TT.z"},       {"Z", "FilenameFormatting.TT.Z"},
		{"FPS", "FilenameFormatting.TT.FPS"},   {"CRES", "FilenameFormatting.TT.CRES"},
		{"ORES", "FilenameFormatting.TT.ORES"}, {"VF", "FilenameFormatting.TT.VF"},
	};

	QString html = "<table>";

	for (auto f : format_list) {
		html += "<tr><th align='left'>%";
		html += f[0];
		html += "</th><td>";
		html += QTStr(f[1]);
		html += "</td></tr>";
	}

	html += "</table>";
	return html;
}

void OBSBasicSettings::LoadAdvOutputStreamingSettings()
{
	const char *rescaleRes = config_get_string(main->Config(), "AdvOut", "RescaleRes");
	int rescaleFilter = config_get_int(main->Config(), "AdvOut", "RescaleFilter");
	int trackIndex = config_get_int(main->Config(), "AdvOut", "TrackIndex");
	int audioMixes = config_get_int(main->Config(), "AdvOut", "StreamMultiTrackAudioMixes");
	ui->advOutRescale->setEnabled(rescaleFilter != OBS_SCALE_DISABLE);
	ui->advOutRescale->setCurrentText(rescaleRes);

	int idx = ui->advOutRescaleFilter->findData(rescaleFilter);
	if (idx != -1)
		ui->advOutRescaleFilter->setCurrentIndex(idx);

	QStringList specList = QTStr("FilenameFormatting.completer").split(QRegularExpression("\n"));
	QCompleter *specCompleter = new QCompleter(specList);
	specCompleter->setCaseSensitivity(Qt::CaseSensitive);
	specCompleter->setFilterMode(Qt::MatchContains);
	ui->filenameFormatting->setCompleter(specCompleter);
	ui->filenameFormatting->setToolTip(makeFormatToolTip());

	switch (trackIndex) {
	case 1:
		ui->advOutTrack1->setChecked(true);
		break;
	case 2:
		ui->advOutTrack2->setChecked(true);
		break;
	case 3:
		ui->advOutTrack3->setChecked(true);
		break;
	case 4:
		ui->advOutTrack4->setChecked(true);
		break;
	case 5:
		ui->advOutTrack5->setChecked(true);
		break;
	case 6:
		ui->advOutTrack6->setChecked(true);
		break;
	}
	ui->advOutMultiTrack1->setChecked(audioMixes & (1 << 0));
	ui->advOutMultiTrack2->setChecked(audioMixes & (1 << 1));
	ui->advOutMultiTrack3->setChecked(audioMixes & (1 << 2));
	ui->advOutMultiTrack4->setChecked(audioMixes & (1 << 3));
	ui->advOutMultiTrack5->setChecked(audioMixes & (1 << 4));
	ui->advOutMultiTrack6->setChecked(audioMixes & (1 << 5));

	obs_service_t *service_obj = main->GetService();
	const char *protocol = nullptr;
	protocol = obs_service_get_protocol(service_obj);
	SwapMultiTrack(protocol);
}

OBSPropertiesView *OBSBasicSettings::CreateEncoderPropertyView(const char *encoder, const char *path, bool changed)
{
	OBSDataAutoRelease settings = obs_encoder_defaults(encoder);
	OBSPropertiesView *view;

	if (path) {
		const OBSBasic *basic = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
		const OBSProfile &currentProfile = basic->GetCurrentProfile();

		const std::filesystem::path jsonFilePath = currentProfile.path / std::filesystem::u8path(path);

		if (!jsonFilePath.empty()) {
			obs_data_t *data = obs_data_create_from_json_file_safe(jsonFilePath.u8string().c_str(), "bak");
			obs_data_apply(settings, data);
			obs_data_release(data);
		}
	}

	view = new OBSPropertiesView(settings.Get(), encoder, (PropertiesReloadCallback)obs_get_encoder_properties,
				     170);
	view->setFrameShape(QFrame::NoFrame);
	view->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	view->setProperty("changed", QVariant(changed));
	view->setScrolling(false);
	QObject::connect(view, &OBSPropertiesView::Changed, this, &OBSBasicSettings::OutputsChanged);

	return view;
}

void OBSBasicSettings::LoadAdvOutputStreamingEncoderProperties()
{
	const char *type = config_get_string(main->Config(), "AdvOut", "Encoder");

	delete streamEncoderProps;
	streamEncoderProps = CreateEncoderPropertyView(type, "streamEncoder.json");
	streamEncoderProps->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	ui->advOutEncoderLayout->addWidget(streamEncoderProps);

	connect(streamEncoderProps, &OBSPropertiesView::Changed, this, &OBSBasicSettings::UpdateStreamDelayEstimate);
	connect(streamEncoderProps, &OBSPropertiesView::Changed, this, &OBSBasicSettings::AdvReplayBufferChanged);

	curAdvStreamEncoder = type;

	if (!SetComboByValue(ui->advOutEncoder, type)) {
		uint32_t caps = obs_get_encoder_caps(type);
		if ((caps & ENCODER_HIDE_FLAGS) != 0) {
			QString encName = QT_UTF8(obs_encoder_get_display_name(type));
			if (caps & OBS_ENCODER_CAP_DEPRECATED)
				encName += " (" + QTStr("Deprecated") + ")";

			ui->advOutEncoder->insertItem(0, encName, QT_UTF8(type));
			SetComboByValue(ui->advOutEncoder, type);
		}
	}

	UpdateStreamDelayEstimate();
}

void OBSBasicSettings::LoadAdvOutputRecordingSettings()
{
	const char *type = config_get_string(main->Config(), "AdvOut", "RecType");
	const char *format = config_get_string(main->Config(), "AdvOut", "RecFormat2");
	const char *path = config_get_string(main->Config(), "AdvOut", "RecFilePath");
	bool noSpace = config_get_bool(main->Config(), "AdvOut", "RecFileNameWithoutSpace");
	const char *rescaleRes = config_get_string(main->Config(), "AdvOut", "RecRescaleRes");
	int rescaleFilter = config_get_int(main->Config(), "AdvOut", "RecRescaleFilter");
	const char *muxCustom = config_get_string(main->Config(), "AdvOut", "RecMuxerCustom");
	int tracks = config_get_int(main->Config(), "AdvOut", "RecTracks");
	int flvTrack = config_get_int(main->Config(), "AdvOut", "FLVTrack");
	bool splitFile = config_get_bool(main->Config(), "AdvOut", "RecSplitFile");
	const char *splitFileType = config_get_string(main->Config(), "AdvOut", "RecSplitFileType");
	int splitFileTime = config_get_int(main->Config(), "AdvOut", "RecSplitFileTime");
	int splitFileSize = config_get_int(main->Config(), "AdvOut", "RecSplitFileSize");

	int typeIndex = (astrcmpi(type, "FFmpeg") == 0) ? 1 : 0;
	ui->advOutRecType->setCurrentIndex(typeIndex);
	ui->advOutRecPath->setText(path);
	ui->advOutNoSpace->setChecked(noSpace);
	ui->advOutRecRescale->setCurrentText(rescaleRes);
	int idx = ui->advOutRecRescaleFilter->findData(rescaleFilter);
	if (idx != -1)
		ui->advOutRecRescaleFilter->setCurrentIndex(idx);
	ui->advOutMuxCustom->setText(muxCustom);

	idx = ui->advOutRecFormat->findData(format);
	ui->advOutRecFormat->setCurrentIndex(idx);

	ui->advOutRecTrack1->setChecked(tracks & (1 << 0));
	ui->advOutRecTrack2->setChecked(tracks & (1 << 1));
	ui->advOutRecTrack3->setChecked(tracks & (1 << 2));
	ui->advOutRecTrack4->setChecked(tracks & (1 << 3));
	ui->advOutRecTrack5->setChecked(tracks & (1 << 4));
	ui->advOutRecTrack6->setChecked(tracks & (1 << 5));

	if (astrcmpi(splitFileType, "Size") == 0)
		idx = 1;
	else if (astrcmpi(splitFileType, "Manual") == 0)
		idx = 2;
	else
		idx = 0;
	ui->advOutSplitFile->setChecked(splitFile);
	ui->advOutSplitFileType->setCurrentIndex(idx);
	ui->advOutSplitFileTime->setValue(splitFileTime);
	ui->advOutSplitFileSize->setValue(splitFileSize);

	switch (flvTrack) {
	case 1:
		ui->flvTrack1->setChecked(true);
		break;
	case 2:
		ui->flvTrack2->setChecked(true);
		break;
	case 3:
		ui->flvTrack3->setChecked(true);
		break;
	case 4:
		ui->flvTrack4->setChecked(true);
		break;
	case 5:
		ui->flvTrack5->setChecked(true);
		break;
	case 6:
		ui->flvTrack6->setChecked(true);
		break;
	default:
		ui->flvTrack1->setChecked(true);
		break;
	}
}

void OBSBasicSettings::LoadAdvOutputRecordingEncoderProperties()
{
	const char *type = config_get_string(main->Config(), "AdvOut", "RecEncoder");

	delete recordEncoderProps;
	recordEncoderProps = nullptr;

	if (astrcmpi(type, "none") != 0) {
		recordEncoderProps = CreateEncoderPropertyView(type, "recordEncoder.json");
		recordEncoderProps->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
		ui->advOutRecEncoderProps->layout()->addWidget(recordEncoderProps);
		connect(recordEncoderProps, &OBSPropertiesView::Changed, this,
			&OBSBasicSettings::AdvReplayBufferChanged);
	}

	curAdvRecordEncoder = type;

	if (!SetComboByValue(ui->advOutRecEncoder, type)) {
		uint32_t caps = obs_get_encoder_caps(type);
		if ((caps & ENCODER_HIDE_FLAGS) != 0) {
			QString encName = QT_UTF8(obs_encoder_get_display_name(type));
			if (caps & OBS_ENCODER_CAP_DEPRECATED)
				encName += " (" + QTStr("Deprecated") + ")";

			ui->advOutRecEncoder->insertItem(1, encName, QT_UTF8(type));
			SetComboByValue(ui->advOutRecEncoder, type);
		} else {
			ui->advOutRecEncoder->setCurrentIndex(-1);
		}
	}
}

static void SelectFormat(QComboBox *combo, const char *name, const char *mimeType)
{
	FFmpegFormat format{name, mimeType};

	for (int i = 0; i < combo->count(); i++) {
		QVariant v = combo->itemData(i);
		if (!v.isNull()) {
			if (format == v.value<FFmpegFormat>()) {
				combo->setCurrentIndex(i);
				return;
			}
		}
	}

	combo->setCurrentIndex(0);
}

static void SelectEncoder(QComboBox *combo, const char *name, int id)
{
	int idx = FindEncoder(combo, name, id);
	if (idx >= 0)
		combo->setCurrentIndex(idx);
}

void OBSBasicSettings::LoadAdvOutputFFmpegSettings()
{
	bool saveFile = config_get_bool(main->Config(), "AdvOut", "FFOutputToFile");
	const char *path = config_get_string(main->Config(), "AdvOut", "FFFilePath");
	bool noSpace = config_get_bool(main->Config(), "AdvOut", "FFFileNameWithoutSpace");
	const char *url = config_get_string(main->Config(), "AdvOut", "FFURL");
	const char *format = config_get_string(main->Config(), "AdvOut", "FFFormat");
	const char *mimeType = config_get_string(main->Config(), "AdvOut", "FFFormatMimeType");
	const char *muxCustom = config_get_string(main->Config(), "AdvOut", "FFMCustom");
	int videoBitrate = config_get_int(main->Config(), "AdvOut", "FFVBitrate");
	int gopSize = config_get_int(main->Config(), "AdvOut", "FFVGOPSize");
	bool rescale = config_get_bool(main->Config(), "AdvOut", "FFRescale");
	bool codecCompat = config_get_bool(main->Config(), "AdvOut", "FFIgnoreCompat");
	const char *rescaleRes = config_get_string(main->Config(), "AdvOut", "FFRescaleRes");
	const char *vEncoder = config_get_string(main->Config(), "AdvOut", "FFVEncoder");
	int vEncoderId = config_get_int(main->Config(), "AdvOut", "FFVEncoderId");
	const char *vEncCustom = config_get_string(main->Config(), "AdvOut", "FFVCustom");
	int audioBitrate = config_get_int(main->Config(), "AdvOut", "FFABitrate");
	int audioMixes = config_get_int(main->Config(), "AdvOut", "FFAudioMixes");
	const char *aEncoder = config_get_string(main->Config(), "AdvOut", "FFAEncoder");
	int aEncoderId = config_get_int(main->Config(), "AdvOut", "FFAEncoderId");
	const char *aEncCustom = config_get_string(main->Config(), "AdvOut", "FFACustom");

	ui->advOutFFType->setCurrentIndex(saveFile ? 0 : 1);
	ui->advOutFFRecPath->setText(QT_UTF8(path));
	ui->advOutFFNoSpace->setChecked(noSpace);
	ui->advOutFFURL->setText(QT_UTF8(url));
	SelectFormat(ui->advOutFFFormat, format, mimeType);
	ui->advOutFFMCfg->setText(muxCustom);
	ui->advOutFFVBitrate->setValue(videoBitrate);
	ui->advOutFFVGOPSize->setValue(gopSize);
	ui->advOutFFUseRescale->setChecked(rescale);
	ui->advOutFFIgnoreCompat->setChecked(codecCompat);
	ui->advOutFFRescale->setEnabled(rescale);
	ui->advOutFFRescale->setCurrentText(rescaleRes);
	SelectEncoder(ui->advOutFFVEncoder, vEncoder, vEncoderId);
	ui->advOutFFVCfg->setText(vEncCustom);
	ui->advOutFFABitrate->setValue(audioBitrate);
	SelectEncoder(ui->advOutFFAEncoder, aEncoder, aEncoderId);
	ui->advOutFFACfg->setText(aEncCustom);

	ui->advOutFFTrack1->setChecked(audioMixes & (1 << 0));
	ui->advOutFFTrack2->setChecked(audioMixes & (1 << 1));
	ui->advOutFFTrack3->setChecked(audioMixes & (1 << 2));
	ui->advOutFFTrack4->setChecked(audioMixes & (1 << 3));
	ui->advOutFFTrack5->setChecked(audioMixes & (1 << 4));
	ui->advOutFFTrack6->setChecked(audioMixes & (1 << 5));
}

void OBSBasicSettings::LoadAdvOutputAudioSettings()
{
	int track1Bitrate = config_get_uint(main->Config(), "AdvOut", "Track1Bitrate");
	int track2Bitrate = config_get_uint(main->Config(), "AdvOut", "Track2Bitrate");
	int track3Bitrate = config_get_uint(main->Config(), "AdvOut", "Track3Bitrate");
	int track4Bitrate = config_get_uint(main->Config(), "AdvOut", "Track4Bitrate");
	int track5Bitrate = config_get_uint(main->Config(), "AdvOut", "Track5Bitrate");
	int track6Bitrate = config_get_uint(main->Config(), "AdvOut", "Track6Bitrate");
	const char *name1 = config_get_string(main->Config(), "AdvOut", "Track1Name");
	const char *name2 = config_get_string(main->Config(), "AdvOut", "Track2Name");
	const char *name3 = config_get_string(main->Config(), "AdvOut", "Track3Name");
	const char *name4 = config_get_string(main->Config(), "AdvOut", "Track4Name");
	const char *name5 = config_get_string(main->Config(), "AdvOut", "Track5Name");
	const char *name6 = config_get_string(main->Config(), "AdvOut", "Track6Name");

	const char *encoder_id = config_get_string(main->Config(), "AdvOut", "AudioEncoder");
	const char *rec_encoder_id = config_get_string(main->Config(), "AdvOut", "RecAudioEncoder");

	PopulateAdvancedBitrates({ui->advOutTrack1Bitrate, ui->advOutTrack2Bitrate, ui->advOutTrack3Bitrate,
				  ui->advOutTrack4Bitrate, ui->advOutTrack5Bitrate, ui->advOutTrack6Bitrate},
				 encoder_id, strcmp(rec_encoder_id, "none") != 0 ? rec_encoder_id : encoder_id);

	track1Bitrate = FindClosestAvailableAudioBitrate(ui->advOutTrack1Bitrate, track1Bitrate);
	track2Bitrate = FindClosestAvailableAudioBitrate(ui->advOutTrack2Bitrate, track2Bitrate);
	track3Bitrate = FindClosestAvailableAudioBitrate(ui->advOutTrack3Bitrate, track3Bitrate);
	track4Bitrate = FindClosestAvailableAudioBitrate(ui->advOutTrack4Bitrate, track4Bitrate);
	track5Bitrate = FindClosestAvailableAudioBitrate(ui->advOutTrack5Bitrate, track5Bitrate);
	track6Bitrate = FindClosestAvailableAudioBitrate(ui->advOutTrack6Bitrate, track6Bitrate);

	// restrict list of bitrates when multichannel is OFF
	const char *speakers = config_get_string(main->Config(), "Audio", "ChannelSetup");

	// restrict list of bitrates when multichannel is OFF
	if (!IsSurround(speakers)) {
		RestrictResetBitrates({ui->advOutTrack1Bitrate, ui->advOutTrack2Bitrate, ui->advOutTrack3Bitrate,
				       ui->advOutTrack4Bitrate, ui->advOutTrack5Bitrate, ui->advOutTrack6Bitrate},
				      320);
	}

	SetComboByName(ui->advOutTrack1Bitrate, std::to_string(track1Bitrate).c_str());
	SetComboByName(ui->advOutTrack2Bitrate, std::to_string(track2Bitrate).c_str());
	SetComboByName(ui->advOutTrack3Bitrate, std::to_string(track3Bitrate).c_str());
	SetComboByName(ui->advOutTrack4Bitrate, std::to_string(track4Bitrate).c_str());
	SetComboByName(ui->advOutTrack5Bitrate, std::to_string(track5Bitrate).c_str());
	SetComboByName(ui->advOutTrack6Bitrate, std::to_string(track6Bitrate).c_str());

	ui->advOutTrack1Name->setText(name1);
	ui->advOutTrack2Name->setText(name2);
	ui->advOutTrack3Name->setText(name3);
	ui->advOutTrack4Name->setText(name4);
	ui->advOutTrack5Name->setText(name5);
	ui->advOutTrack6Name->setText(name6);
}

void OBSBasicSettings::LoadOutputSettings()
{
	loading = true;

	ResetEncoders();

	const char *mode = config_get_string(main->Config(), "Output", "Mode");

	int modeIdx = astrcmpi(mode, "Advanced") == 0 ? 1 : 0;
	ui->outputMode->setCurrentIndex(modeIdx);

	LoadSimpleOutputSettings();
	LoadAdvOutputStreamingSettings();
	LoadAdvOutputStreamingEncoderProperties();

	const char *type = config_get_string(main->Config(), "AdvOut", "AudioEncoder");
	if (!SetComboByValue(ui->advOutAEncoder, type))
		ui->advOutAEncoder->setCurrentIndex(-1);

	LoadAdvOutputRecordingSettings();
	LoadAdvOutputRecordingEncoderProperties();
	type = config_get_string(main->Config(), "AdvOut", "RecAudioEncoder");
	if (!SetComboByValue(ui->advOutRecAEncoder, type))
		ui->advOutRecAEncoder->setCurrentIndex(-1);
	LoadAdvOutputFFmpegSettings();
	LoadAdvOutputAudioSettings();

	if (obs_video_active()) {
		ui->outputMode->setEnabled(false);
		ui->outputModeLabel->setEnabled(false);
		ui->simpleOutStrEncoderLabel->setEnabled(false);
		ui->simpleOutStrEncoder->setEnabled(false);
		ui->simpleOutStrAEncoderLabel->setEnabled(false);
		ui->simpleOutStrAEncoder->setEnabled(false);
		ui->simpleRecordingGroupBox->setEnabled(false);
		ui->simpleReplayBuf->setEnabled(false);
		ui->advOutTopContainer->setEnabled(false);
		ui->advOutRecTopContainer->setEnabled(false);
		ui->advOutRecTypeContainer->setEnabled(false);
		ui->advOutputAudioTracksTab->setEnabled(false);
		ui->advNetworkGroupBox->setEnabled(false);
	}

	loading = false;
}

void OBSBasicSettings::SetAdvOutputFFmpegEnablement(FFmpegCodecType encoderType, bool enabled, bool enableEncoder)
{
	bool rescale = config_get_bool(main->Config(), "AdvOut", "FFRescale");

	switch (encoderType) {
	case FFmpegCodecType::VIDEO:
		ui->advOutFFVBitrate->setEnabled(enabled);
		ui->advOutFFVGOPSize->setEnabled(enabled);
		ui->advOutFFUseRescale->setEnabled(enabled);
		ui->advOutFFRescale->setEnabled(enabled && rescale);
		ui->advOutFFVEncoder->setEnabled(enabled || enableEncoder);
		ui->advOutFFVCfg->setEnabled(enabled);
		break;
	case FFmpegCodecType::AUDIO:
		ui->advOutFFABitrate->setEnabled(enabled);
		ui->advOutFFAEncoder->setEnabled(enabled || enableEncoder);
		ui->advOutFFACfg->setEnabled(enabled);
		ui->advOutFFTrack1->setEnabled(enabled);
		ui->advOutFFTrack2->setEnabled(enabled);
		ui->advOutFFTrack3->setEnabled(enabled);
		ui->advOutFFTrack4->setEnabled(enabled);
		ui->advOutFFTrack5->setEnabled(enabled);
		ui->advOutFFTrack6->setEnabled(enabled);
	default:
		break;
	}
}

static inline void LoadListValue(QComboBox *widget, const char *text, const char *val)
{
	widget->addItem(QT_UTF8(text), QT_UTF8(val));
}

void OBSBasicSettings::LoadListValues(QComboBox *widget, obs_property_t *prop, int index)
{
	size_t count = obs_property_list_item_count(prop);

	OBSSourceAutoRelease source = obs_get_output_source(index);
	const char *deviceId = nullptr;
	OBSDataAutoRelease settings = nullptr;

	if (source) {
		settings = obs_source_get_settings(source);
		if (settings)
			deviceId = obs_data_get_string(settings, "device_id");
	}

	widget->addItem(QTStr("Basic.Settings.Audio.Disabled"), "disabled");

	for (size_t i = 0; i < count; i++) {
		const char *name = obs_property_list_item_name(prop, i);
		const char *val = obs_property_list_item_string(prop, i);
		LoadListValue(widget, name, val);
	}

	if (deviceId) {
		QVariant var(QT_UTF8(deviceId));
		int idx = widget->findData(var);
		if (idx != -1) {
			widget->setCurrentIndex(idx);
		} else {
			widget->insertItem(0,
					   QTStr("Basic.Settings.Audio."
						 "UnknownAudioDevice"),
					   var);
			widget->setCurrentIndex(0);
			HighlightGroupBoxLabel(ui->audioDevicesGroupBox, widget, "errorLabel");
		}
	}
}

void OBSBasicSettings::LoadAudioDevices()
{
	const char *input_id = App()->InputAudioSource();
	const char *output_id = App()->OutputAudioSource();

	obs_properties_t *input_props = obs_get_source_properties(input_id);
	obs_properties_t *output_props = obs_get_source_properties(output_id);

	if (input_props) {
		obs_property_t *inputs = obs_properties_get(input_props, "device_id");
		LoadListValues(ui->auxAudioDevice1, inputs, 3);
		LoadListValues(ui->auxAudioDevice2, inputs, 4);
		LoadListValues(ui->auxAudioDevice3, inputs, 5);
		LoadListValues(ui->auxAudioDevice4, inputs, 6);
		obs_properties_destroy(input_props);
	}

	if (output_props) {
		obs_property_t *outputs = obs_properties_get(output_props, "device_id");
		LoadListValues(ui->desktopAudioDevice1, outputs, 1);
		LoadListValues(ui->desktopAudioDevice2, outputs, 2);
		obs_properties_destroy(output_props);
	}

	if (obs_video_active()) {
		ui->sampleRate->setEnabled(false);
		ui->channelSetup->setEnabled(false);
	}
}

#define NBSP "\xC2\xA0"

void OBSBasicSettings::LoadAudioSources()
{
	if (ui->audioSourceLayout->rowCount() > 0) {
		QLayoutItem *forDeletion = ui->audioSourceLayout->takeAt(0);
		forDeletion->widget()->deleteLater();
		delete forDeletion;
	}
	auto layout = new QFormLayout();
	layout->setVerticalSpacing(15);
	layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

	audioSourceSignals.clear();
	audioSources.clear();

	auto widget = new QWidget();
	widget->setLayout(layout);
	ui->audioSourceLayout->addRow(widget);

	const char *enablePtm = Str("Basic.Settings.Audio.EnablePushToMute");
	const char *ptmDelay = Str("Basic.Settings.Audio.PushToMuteDelay");
	const char *enablePtt = Str("Basic.Settings.Audio.EnablePushToTalk");
	const char *pttDelay = Str("Basic.Settings.Audio.PushToTalkDelay");
	auto AddSource = [&](obs_source_t *source) {
		if (!(obs_source_get_output_flags(source) & OBS_SOURCE_AUDIO))
			return true;

		auto form = new QFormLayout();
		form->setVerticalSpacing(0);
		form->setHorizontalSpacing(5);
		form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

		auto ptmCB = new SilentUpdateCheckBox();
		ptmCB->setText(enablePtm);
		ptmCB->setChecked(obs_source_push_to_mute_enabled(source));
		form->addRow(ptmCB);

		auto ptmSB = new SilentUpdateSpinBox();
		ptmSB->setSuffix(NBSP "ms");
		ptmSB->setRange(0, INT_MAX);
		ptmSB->setValue(obs_source_get_push_to_mute_delay(source));
		form->addRow(ptmDelay, ptmSB);

		auto pttCB = new SilentUpdateCheckBox();
		pttCB->setText(enablePtt);
		pttCB->setChecked(obs_source_push_to_talk_enabled(source));
		form->addRow(pttCB);

		auto pttSB = new SilentUpdateSpinBox();
		pttSB->setSuffix(NBSP "ms");
		pttSB->setRange(0, INT_MAX);
		pttSB->setValue(obs_source_get_push_to_talk_delay(source));
		form->addRow(pttDelay, pttSB);

		HookWidget(ptmCB, CHECK_CHANGED, AUDIO_CHANGED);
		HookWidget(ptmSB, SCROLL_CHANGED, AUDIO_CHANGED);
		HookWidget(pttCB, CHECK_CHANGED, AUDIO_CHANGED);
		HookWidget(pttSB, SCROLL_CHANGED, AUDIO_CHANGED);

		audioSourceSignals.reserve(audioSourceSignals.size() + 4);

		auto handler = obs_source_get_signal_handler(source);
		audioSourceSignals.emplace_back(
			handler, "push_to_mute_changed",
			[](void *data, calldata_t *param) {
				QMetaObject::invokeMethod(static_cast<QObject *>(data), "setCheckedSilently",
							  Q_ARG(bool, calldata_bool(param, "enabled")));
			},
			ptmCB);
		audioSourceSignals.emplace_back(
			handler, "push_to_mute_delay",
			[](void *data, calldata_t *param) {
				QMetaObject::invokeMethod(static_cast<QObject *>(data), "setValueSilently",
							  Q_ARG(int, calldata_int(param, "delay")));
			},
			ptmSB);
		audioSourceSignals.emplace_back(
			handler, "push_to_talk_changed",
			[](void *data, calldata_t *param) {
				QMetaObject::invokeMethod(static_cast<QObject *>(data), "setCheckedSilently",
							  Q_ARG(bool, calldata_bool(param, "enabled")));
			},
			pttCB);
		audioSourceSignals.emplace_back(
			handler, "push_to_talk_delay",
			[](void *data, calldata_t *param) {
				QMetaObject::invokeMethod(static_cast<QObject *>(data), "setValueSilently",
							  Q_ARG(int, calldata_int(param, "delay")));
			},
			pttSB);

		audioSources.emplace_back(OBSGetWeakRef(source), ptmCB, ptmSB, pttCB, pttSB);

		auto label = new OBSSourceLabel(source);
		TruncateLabel(label, label->text());
		label->setMinimumSize(QSize(170, 0));
		label->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
		connect(label, &OBSSourceLabel::Removed,
			[=]() { QMetaObject::invokeMethod(this, "ReloadAudioSources"); });
		connect(label, &OBSSourceLabel::Destroyed,
			[=]() { QMetaObject::invokeMethod(this, "ReloadAudioSources"); });

		layout->addRow(label, form);
		return true;
	};

	using AddSource_t = decltype(AddSource);
	obs_enum_sources(
		[](void *data, obs_source_t *source) {
			auto &AddSource = *static_cast<AddSource_t *>(data);
			if (!obs_source_removed(source))
				AddSource(source);
			return true;
		},
		static_cast<void *>(&AddSource));

	if (layout->rowCount() == 0)
		ui->audioHotkeysGroupBox->hide();
	else
		ui->audioHotkeysGroupBox->show();
}

void OBSBasicSettings::LoadAudioSettings()
{
	uint32_t sampleRate = config_get_uint(main->Config(), "Audio", "SampleRate");
	const char *speakers = config_get_string(main->Config(), "Audio", "ChannelSetup");
	double meterDecayRate = config_get_double(main->Config(), "Audio", "MeterDecayRate");
	uint32_t peakMeterTypeIdx = config_get_uint(main->Config(), "Audio", "PeakMeterType");
	bool enableLLAudioBuffering = config_get_bool(App()->GetUserConfig(), "Audio", "LowLatencyAudioBuffering");

	loading = true;

	const char *str;
	if (sampleRate == 48000)
		str = "48 kHz";
	else
		str = "44.1 kHz";

	int sampleRateIdx = ui->sampleRate->findText(str);
	if (sampleRateIdx != -1)
		ui->sampleRate->setCurrentIndex(sampleRateIdx);

	if (strcmp(speakers, "Mono") == 0)
		ui->channelSetup->setCurrentIndex(0);
	else if (strcmp(speakers, "2.1") == 0)
		ui->channelSetup->setCurrentIndex(2);
	else if (strcmp(speakers, "4.0") == 0)
		ui->channelSetup->setCurrentIndex(3);
	else if (strcmp(speakers, "4.1") == 0)
		ui->channelSetup->setCurrentIndex(4);
	else if (strcmp(speakers, "5.1") == 0)
		ui->channelSetup->setCurrentIndex(5);
	else if (strcmp(speakers, "7.1") == 0)
		ui->channelSetup->setCurrentIndex(6);
	else
		ui->channelSetup->setCurrentIndex(1);

	if (meterDecayRate == VOLUME_METER_DECAY_MEDIUM)
		ui->meterDecayRate->setCurrentIndex(1);
	else if (meterDecayRate == VOLUME_METER_DECAY_SLOW)
		ui->meterDecayRate->setCurrentIndex(2);
	else
		ui->meterDecayRate->setCurrentIndex(0);

	ui->peakMeterType->setCurrentIndex(peakMeterTypeIdx);
	ui->lowLatencyBuffering->setChecked(enableLLAudioBuffering);

	LoadAudioDevices();
	LoadAudioSources();

	loading = false;
}

void OBSBasicSettings::UpdateColorFormatSpaceWarning()
{
	const QString format = ui->colorFormat->currentData().toString();
	switch (ui->colorSpace->currentIndex()) {
	case 3: /* Rec.2100 (PQ) */
	case 4: /* Rec.2100 (HLG) */
		if ((format == "P010") || (format == "P216") || (format == "P416")) {
			ui->advancedMsg2->clear();
			ui->advancedMsg2->setVisible(false);
		} else if (format == "I010") {
			ui->advancedMsg2->setText(QTStr("Basic.Settings.Advanced.FormatWarning"));
			ui->advancedMsg2->setVisible(true);
		} else {
			ui->advancedMsg2->setText(QTStr("Basic.Settings.Advanced.FormatWarning2100"));
			ui->advancedMsg2->setVisible(true);
		}
		break;
	default:
		if (format == "NV12") {
			ui->advancedMsg2->clear();
			ui->advancedMsg2->setVisible(false);
		} else if ((format == "I010") || (format == "P010") || (format == "P216") || (format == "P416")) {
			ui->advancedMsg2->setText(QTStr("Basic.Settings.Advanced.FormatWarningPreciseSdr"));
			ui->advancedMsg2->setVisible(true);
		} else {
			ui->advancedMsg2->setText(QTStr("Basic.Settings.Advanced.FormatWarning"));
			ui->advancedMsg2->setVisible(true);
		}
	}
}

void OBSBasicSettings::LoadAdvancedSettings()
{
	const char *videoColorFormat = config_get_string(main->Config(), "Video", "ColorFormat");
	const char *videoColorSpace = config_get_string(main->Config(), "Video", "ColorSpace");
	const char *videoColorRange = config_get_string(main->Config(), "Video", "ColorRange");
	uint32_t sdrWhiteLevel = (uint32_t)config_get_uint(main->Config(), "Video", "SdrWhiteLevel");
	uint32_t hdrNominalPeakLevel = (uint32_t)config_get_uint(main->Config(), "Video", "HdrNominalPeakLevel");

	QString monDevName;
	QString monDevId;
	if (obs_audio_monitoring_available()) {
		monDevName = config_get_string(main->Config(), "Audio", "MonitoringDeviceName");
		monDevId = config_get_string(main->Config(), "Audio", "MonitoringDeviceId");
	}
	bool enableDelay = config_get_bool(main->Config(), "Output", "DelayEnable");
	int delaySec = config_get_int(main->Config(), "Output", "DelaySec");
	bool preserveDelay = config_get_bool(main->Config(), "Output", "DelayPreserve");
	bool reconnect = config_get_bool(main->Config(), "Output", "Reconnect");
	int retryDelay = config_get_int(main->Config(), "Output", "RetryDelay");
	int maxRetries = config_get_int(main->Config(), "Output", "MaxRetries");
	const char *filename = config_get_string(main->Config(), "Output", "FilenameFormatting");
	bool overwriteIfExists = config_get_bool(main->Config(), "Output", "OverwriteIfExists");
	const char *bindIP = config_get_string(main->Config(), "Output", "BindIP");
	const char *rbPrefix = config_get_string(main->Config(), "SimpleOutput", "RecRBPrefix");
	const char *rbSuffix = config_get_string(main->Config(), "SimpleOutput", "RecRBSuffix");
	bool replayBuf = config_get_bool(main->Config(), "AdvOut", "RecRB");
	int rbTime = config_get_int(main->Config(), "AdvOut", "RecRBTime");
	int rbSize = config_get_int(main->Config(), "AdvOut", "RecRBSize");
	bool autoRemux = config_get_bool(main->Config(), "Video", "AutoRemux");
	const char *hotkeyFocusType = config_get_string(App()->GetUserConfig(), "General", "HotkeyFocusType");
	bool dynBitrate = config_get_bool(main->Config(), "Output", "DynamicBitrate");
	const char *ipFamily = config_get_string(main->Config(), "Output", "IPFamily");
	bool confirmOnExit = config_get_bool(App()->GetUserConfig(), "General", "ConfirmOnExit");

	loading = true;

	LoadRendererList();

	if (obs_audio_monitoring_available() && !SetComboByValue(ui->monitoringDevice, monDevId.toUtf8()))
		SetInvalidValue(ui->monitoringDevice, monDevName.toUtf8(), monDevId.toUtf8());

	ui->confirmOnExit->setChecked(confirmOnExit);

	ui->filenameFormatting->setText(filename);
	ui->overwriteIfExists->setChecked(overwriteIfExists);
	ui->simpleRBPrefix->setText(rbPrefix);
	ui->simpleRBSuffix->setText(rbSuffix);

	ui->advReplayBuf->setChecked(replayBuf);
	ui->advRBSecMax->setValue(rbTime);
	ui->advRBMegsMax->setValue(rbSize);

	ui->reconnectEnable->setChecked(reconnect);
	ui->reconnectRetryDelay->setValue(retryDelay);
	ui->reconnectMaxRetries->setValue(maxRetries);

	ui->streamDelaySec->setValue(delaySec);
	ui->streamDelayPreserve->setChecked(preserveDelay);
	ui->streamDelayEnable->setChecked(enableDelay);
	ui->autoRemux->setChecked(autoRemux);
	ui->dynBitrate->setChecked(dynBitrate);

	SetComboByValue(ui->colorFormat, videoColorFormat);
	SetComboByValue(ui->colorSpace, videoColorSpace);
	SetComboByValue(ui->colorRange, videoColorRange);
	ui->sdrWhiteLevel->setValue(sdrWhiteLevel);
	ui->hdrNominalPeakLevel->setValue(hdrNominalPeakLevel);

	SetComboByValue(ui->ipFamily, ipFamily);
	if (!SetComboByValue(ui->bindToIP, bindIP))
		SetInvalidValue(ui->bindToIP, bindIP, bindIP);

	if (obs_video_active()) {
		ui->advancedVideoContainer->setEnabled(false);
	}

#ifdef __APPLE__
	bool disableOSXVSync = config_get_bool(App()->GetAppConfig(), "Video", "DisableOSXVSync");
	bool resetOSXVSync = config_get_bool(App()->GetAppConfig(), "Video", "ResetOSXVSyncOnExit");
	ui->disableOSXVSync->setChecked(disableOSXVSync);
	ui->resetOSXVSync->setChecked(resetOSXVSync);
	ui->resetOSXVSync->setEnabled(disableOSXVSync);
#elif _WIN32
	bool disableAudioDucking = config_get_bool(App()->GetAppConfig(), "Audio", "DisableAudioDucking");
	ui->disableAudioDucking->setChecked(disableAudioDucking);

	const char *processPriority = config_get_string(App()->GetAppConfig(), "General", "ProcessPriority");
	bool enableNewSocketLoop = config_get_bool(main->Config(), "Output", "NewSocketLoopEnable");
	bool enableLowLatencyMode = config_get_bool(main->Config(), "Output", "LowLatencyEnable");

	int idx = ui->processPriority->findData(processPriority);
	if (idx == -1)
		idx = ui->processPriority->findData("Normal");
	ui->processPriority->setCurrentIndex(idx);

	ui->enableNewSocketLoop->setChecked(enableNewSocketLoop);
	ui->enableLowLatencyMode->setChecked(enableLowLatencyMode);
	ui->enableLowLatencyMode->setToolTip(QTStr("Basic.Settings.Advanced.Network.TCPPacing.Tooltip"));
#endif
#if defined(_WIN32) || defined(__APPLE__)
	bool browserHWAccel = config_get_bool(App()->GetAppConfig(), "General", "BrowserHWAccel");
	ui->browserHWAccel->setChecked(browserHWAccel);
	prevBrowserAccel = ui->browserHWAccel->isChecked();
#endif

	SetComboByValue(ui->hotkeyFocusType, hotkeyFocusType);

	loading = false;
}

template<typename Func>
static inline void LayoutHotkey(OBSBasicSettings *settings, obs_hotkey_id id, obs_hotkey_t *key, Func &&fun,
				const map<obs_hotkey_id, vector<obs_key_combination_t>> &keys)
{
	auto *label = new OBSHotkeyLabel;
	QString text = QT_UTF8(obs_hotkey_get_description(key));

	label->setProperty("fullName", text);
	TruncateLabel(label, text);

	OBSHotkeyWidget *hw = nullptr;

	auto combos = keys.find(id);
	if (combos == std::end(keys))
		hw = new OBSHotkeyWidget(settings, id, obs_hotkey_get_name(key), settings);
	else
		hw = new OBSHotkeyWidget(settings, id, obs_hotkey_get_name(key), settings, combos->second);

	hw->label = label;
	hw->setAccessibleName(text);
	label->widget = hw;

	fun(key, label, hw);
}

template<typename Func, typename T> static QLabel *makeLabel(T &t, Func &&getName)
{
	QLabel *label = new QLabel(getName(t));
	label->setStyleSheet("font-weight: bold;");
	return label;
}

template<typename Func> static QLabel *makeLabel(const OBSSource &source, Func &&)
{
	OBSSourceLabel *label = new OBSSourceLabel(source);
	label->setStyleSheet("font-weight: bold;");
	QString name = QT_UTF8(obs_source_get_name(source));
	TruncateLabel(label, name);

	return label;
}

template<typename Func, typename T>
static inline void AddHotkeys(QFormLayout &layout, Func &&getName,
			      std::vector<std::tuple<T, QPointer<QLabel>, QPointer<QWidget>>> &hotkeys)
{
	if (hotkeys.empty())
		return;

	layout.setItem(layout.rowCount(), QFormLayout::SpanningRole, new QSpacerItem(0, 10));

	using tuple_type = std::tuple<T, QPointer<QLabel>, QPointer<QWidget>>;

	stable_sort(begin(hotkeys), end(hotkeys), [&](const tuple_type &a, const tuple_type &b) {
		const auto &o_a = get<0>(a);
		const auto &o_b = get<0>(b);
		return o_a != o_b && string(getName(o_a)) < getName(o_b);
	});

	string prevName;
	for (const auto &hotkey : hotkeys) {
		const auto &o = get<0>(hotkey);
		const char *name = getName(o);
		if (prevName != name) {
			prevName = name;
			layout.setItem(layout.rowCount(), QFormLayout::SpanningRole, new QSpacerItem(0, 10));
			layout.addRow(makeLabel(o, getName));
		}

		auto hlabel = get<1>(hotkey);
		auto widget = get<2>(hotkey);
		layout.addRow(hlabel, widget);
	}
}

void OBSBasicSettings::LoadHotkeySettings(obs_hotkey_id ignoreKey)
{
	hotkeys.clear();
	if (ui->hotkeyFormLayout->rowCount() > 0) {
		QLayoutItem *forDeletion = ui->hotkeyFormLayout->takeAt(0);
		forDeletion->widget()->deleteLater();
		delete forDeletion;
	}
	ui->hotkeyFilterSearch->blockSignals(true);
	ui->hotkeyFilterInput->blockSignals(true);
	ui->hotkeyFilterSearch->setText("");
	ui->hotkeyFilterInput->ResetKey();
	ui->hotkeyFilterSearch->blockSignals(false);
	ui->hotkeyFilterInput->blockSignals(false);

	using keys_t = map<obs_hotkey_id, vector<obs_key_combination_t>>;
	keys_t keys;
	obs_enum_hotkey_bindings(
		[](void *data, size_t, obs_hotkey_binding_t *binding) {
			auto &keys = *static_cast<keys_t *>(data);

			keys[obs_hotkey_binding_get_hotkey_id(binding)].emplace_back(
				obs_hotkey_binding_get_key_combination(binding));

			return true;
		},
		&keys);

	QFormLayout *hotkeysLayout = new QFormLayout();
	hotkeysLayout->setVerticalSpacing(0);
	hotkeysLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	hotkeysLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
	auto hotkeyChildWidget = new QWidget();
	hotkeyChildWidget->setLayout(hotkeysLayout);
	ui->hotkeyFormLayout->addRow(hotkeyChildWidget);

	using namespace std;
	using encoders_elem_t = tuple<OBSEncoder, QPointer<QLabel>, QPointer<QWidget>>;
	using outputs_elem_t = tuple<OBSOutput, QPointer<QLabel>, QPointer<QWidget>>;
	using services_elem_t = tuple<OBSService, QPointer<QLabel>, QPointer<QWidget>>;
	using sources_elem_t = tuple<OBSSource, QPointer<QLabel>, QPointer<QWidget>>;
	vector<encoders_elem_t> encoders;
	vector<outputs_elem_t> outputs;
	vector<services_elem_t> services;
	vector<sources_elem_t> scenes;
	vector<sources_elem_t> sources;

	vector<obs_hotkey_id> pairIds;
	map<obs_hotkey_id, pair<obs_hotkey_id, OBSHotkeyLabel *>> pairLabels;

	using std::move;

	auto HandleEncoder = [&](void *registerer, OBSHotkeyLabel *label, OBSHotkeyWidget *hw) {
		auto weak_encoder = static_cast<obs_weak_encoder_t *>(registerer);
		auto encoder = OBSGetStrongRef(weak_encoder);

		if (!encoder)
			return true;

		encoders.emplace_back(std::move(encoder), label, hw);
		return false;
	};

	auto HandleOutput = [&](void *registerer, OBSHotkeyLabel *label, OBSHotkeyWidget *hw) {
		auto weak_output = static_cast<obs_weak_output_t *>(registerer);
		auto output = OBSGetStrongRef(weak_output);

		if (!output)
			return true;

		outputs.emplace_back(std::move(output), label, hw);
		return false;
	};

	auto HandleService = [&](void *registerer, OBSHotkeyLabel *label, OBSHotkeyWidget *hw) {
		auto weak_service = static_cast<obs_weak_service_t *>(registerer);
		auto service = OBSGetStrongRef(weak_service);

		if (!service)
			return true;

		services.emplace_back(std::move(service), label, hw);
		return false;
	};

	auto HandleSource = [&](void *registerer, OBSHotkeyLabel *label, OBSHotkeyWidget *hw) {
		auto weak_source = static_cast<obs_weak_source_t *>(registerer);
		auto source = OBSGetStrongRef(weak_source);

		if (!source)
			return true;

		if (obs_scene_from_source(source))
			scenes.emplace_back(source, label, hw);
		else if (obs_source_get_name(source) != NULL)
			sources.emplace_back(source, label, hw);

		return false;
	};

	auto RegisterHotkey = [&](obs_hotkey_t *key, OBSHotkeyLabel *label, OBSHotkeyWidget *hw) {
		auto registerer_type = obs_hotkey_get_registerer_type(key);
		void *registerer = obs_hotkey_get_registerer(key);

		obs_hotkey_id partner = obs_hotkey_get_pair_partner_id(key);
		if (partner != OBS_INVALID_HOTKEY_ID) {
			pairLabels.emplace(obs_hotkey_get_id(key), make_pair(partner, label));
			pairIds.push_back(obs_hotkey_get_id(key));
		}

		using std::move;

		switch (registerer_type) {
		case OBS_HOTKEY_REGISTERER_FRONTEND:
			hotkeysLayout->addRow(label, hw);
			break;

		case OBS_HOTKEY_REGISTERER_ENCODER:
			if (HandleEncoder(registerer, label, hw))
				return;
			break;

		case OBS_HOTKEY_REGISTERER_OUTPUT:
			if (HandleOutput(registerer, label, hw))
				return;
			break;

		case OBS_HOTKEY_REGISTERER_SERVICE:
			if (HandleService(registerer, label, hw))
				return;
			break;

		case OBS_HOTKEY_REGISTERER_SOURCE:
			if (HandleSource(registerer, label, hw))
				return;
			break;
		}

		hotkeys.emplace_back(registerer_type == OBS_HOTKEY_REGISTERER_FRONTEND, hw);
		connect(hw, &OBSHotkeyWidget::KeyChanged, this, [=]() {
			HotkeysChanged();
			ScanDuplicateHotkeys(hotkeysLayout);
		});
		connect(hw, &OBSHotkeyWidget::SearchKey, [=](obs_key_combination_t combo) {
			ui->hotkeyFilterSearch->setText("");
			ui->hotkeyFilterInput->HandleNewKey(combo);
			ui->hotkeyFilterInput->KeyChanged(combo);
		});
	};

	auto data = make_tuple(RegisterHotkey, std::move(keys), ignoreKey, this);
	using data_t = decltype(data);
	obs_enum_hotkeys(
		[](void *data, obs_hotkey_id id, obs_hotkey_t *key) {
			data_t &d = *static_cast<data_t *>(data);
			if (id != get<2>(d))
				LayoutHotkey(get<3>(d), id, key, get<0>(d), get<1>(d));
			return true;
		},
		&data);

	for (auto keyId : pairIds) {
		auto data1 = pairLabels.find(keyId);
		if (data1 == end(pairLabels))
			continue;

		auto &label1 = data1->second.second;
		if (label1->pairPartner)
			continue;

		auto data2 = pairLabels.find(data1->second.first);
		if (data2 == end(pairLabels))
			continue;

		auto &label2 = data2->second.second;
		if (label2->pairPartner)
			continue;

		QString tt = QTStr("Basic.Settings.Hotkeys.Pair");
		auto name1 = label1->text();
		auto name2 = label2->text();

		auto Update = [&](OBSHotkeyLabel *label, const QString &name, OBSHotkeyLabel *other,
				  const QString &otherName) {
			QString string = other->property("fullName").value<QString>();

			if (string.isEmpty() || string.isNull())
				string = otherName;

			label->setToolTip(tt.arg(string));
			label->setText(name + " *");
			label->pairPartner = other;
		};
		Update(label1, name1, label2, name2);
		Update(label2, name2, label1, name1);
	}

	AddHotkeys(*hotkeysLayout, obs_output_get_name, outputs);
	AddHotkeys(*hotkeysLayout, obs_source_get_name, scenes);
	AddHotkeys(*hotkeysLayout, obs_source_get_name, sources);
	AddHotkeys(*hotkeysLayout, obs_encoder_get_name, encoders);
	AddHotkeys(*hotkeysLayout, obs_service_get_name, services);

	ScanDuplicateHotkeys(hotkeysLayout);

	/* After this function returns the UI can still be unresponsive for a bit.
	 * So by deferring the call to unsetCursor() to the Qt event loop it will
	 * take until it has actually finished processing the created widgets
	 * before the cursor is reset. */
	QTimer::singleShot(1, this, &OBSBasicSettings::unsetCursor);
	hotkeysLoaded = true;
}

void OBSBasicSettings::LoadSettings(bool changedOnly)
{
	if (!changedOnly || generalChanged)
		LoadGeneralSettings();
	if (!changedOnly || stream1Changed)
		LoadStream1Settings();
	if (!changedOnly || outputsChanged)
		LoadOutputSettings();
	if (!changedOnly || audioChanged)
		LoadAudioSettings();
	if (!changedOnly || videoChanged)
		LoadVideoSettings();
	if (!changedOnly || a11yChanged)
		LoadA11ySettings();
	if (!changedOnly || appearanceChanged)
		LoadAppearanceSettings();
	if (!changedOnly || advancedChanged)
		LoadAdvancedSettings();
}

void OBSBasicSettings::SaveGeneralSettings()
{
	int languageIndex = ui->language->currentIndex();
	QVariant langData = ui->language->itemData(languageIndex);
	string language = langData.toString().toStdString();

	if (WidgetChanged(ui->language))
		config_set_string(App()->GetUserConfig(), "General", "Language", language.c_str());

#if defined(_WIN32) || defined(ENABLE_SPARKLE_UPDATER)
	if (WidgetChanged(ui->enableAutoUpdates))
		config_set_bool(App()->GetAppConfig(), "General", "EnableAutoUpdates",
				ui->enableAutoUpdates->isChecked());
	int branchIdx = ui->updateChannelBox->currentIndex();
	QString branchName = ui->updateChannelBox->itemData(branchIdx).toString();

	if (WidgetChanged(ui->updateChannelBox)) {
		config_set_string(App()->GetAppConfig(), "General", "UpdateBranch", QT_TO_UTF8(branchName));
		forceUpdateCheck = true;
	}
#endif
#ifdef _WIN32
	if (ui->hideOBSFromCapture && WidgetChanged(ui->hideOBSFromCapture)) {
		bool hide_window = ui->hideOBSFromCapture->isChecked();
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "HideOBSWindowsFromCapture", hide_window);

		QWindowList windows = QGuiApplication::allWindows();
		for (auto window : windows) {
			if (window->isVisible()) {
				main->SetDisplayAffinity(window);
			}
		}

		blog(LOG_INFO, "Hide OBS windows from screen capture: %s", hide_window ? "true" : "false");
	}
#endif
	if (WidgetChanged(ui->openStatsOnStartup))
		config_set_bool(main->Config(), "General", "OpenStatsOnStartup", ui->openStatsOnStartup->isChecked());
	if (WidgetChanged(ui->snappingEnabled))
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "SnappingEnabled",
				ui->snappingEnabled->isChecked());
	if (WidgetChanged(ui->screenSnapping))
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "ScreenSnapping",
				ui->screenSnapping->isChecked());
	if (WidgetChanged(ui->centerSnapping))
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "CenterSnapping",
				ui->centerSnapping->isChecked());
	if (WidgetChanged(ui->sourceSnapping))
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "SourceSnapping",
				ui->sourceSnapping->isChecked());
	if (WidgetChanged(ui->snapDistance))
		config_set_double(App()->GetUserConfig(), "BasicWindow", "SnapDistance", ui->snapDistance->value());
	if (WidgetChanged(ui->overflowAlwaysVisible) || WidgetChanged(ui->overflowHide) ||
	    WidgetChanged(ui->overflowSelectionHide)) {
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "OverflowAlwaysVisible",
				ui->overflowAlwaysVisible->isChecked());
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "OverflowHidden", ui->overflowHide->isChecked());
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "OverflowSelectionHidden",
				ui->overflowSelectionHide->isChecked());
		main->UpdatePreviewOverflowSettings();
	}
	if (WidgetChanged(ui->previewSafeAreas)) {
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "ShowSafeAreas",
				ui->previewSafeAreas->isChecked());
		main->UpdatePreviewSafeAreas();
	}

	if (WidgetChanged(ui->previewSpacingHelpers)) {
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "SpacingHelpersEnabled",
				ui->previewSpacingHelpers->isChecked());
		main->UpdatePreviewSpacingHelpers();
	}

	if (WidgetChanged(ui->doubleClickSwitch))
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "TransitionOnDoubleClick",
				ui->doubleClickSwitch->isChecked());
	if (WidgetChanged(ui->automaticSearch))
		config_set_bool(App()->GetUserConfig(), "General", "AutomaticCollectionSearch",
				ui->automaticSearch->isChecked());

	config_set_bool(App()->GetUserConfig(), "BasicWindow", "WarnBeforeStartingStream",
			ui->warnBeforeStreamStart->isChecked());
	config_set_bool(App()->GetUserConfig(), "BasicWindow", "WarnBeforeStoppingStream",
			ui->warnBeforeStreamStop->isChecked());
	config_set_bool(App()->GetUserConfig(), "BasicWindow", "WarnBeforeStoppingRecord",
			ui->warnBeforeRecordStop->isChecked());

	if (WidgetChanged(ui->hideProjectorCursor)) {
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "HideProjectorCursor",
				ui->hideProjectorCursor->isChecked());
		main->UpdateProjectorHideCursor();
	}

	if (WidgetChanged(ui->projectorAlwaysOnTop)) {
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "ProjectorAlwaysOnTop",
				ui->projectorAlwaysOnTop->isChecked());
#if defined(_WIN32) || defined(__APPLE__)
		main->UpdateProjectorAlwaysOnTop(ui->projectorAlwaysOnTop->isChecked());
#else
		main->ResetProjectors();
#endif
	}

	if (WidgetChanged(ui->recordWhenStreaming))
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "RecordWhenStreaming",
				ui->recordWhenStreaming->isChecked());
	if (WidgetChanged(ui->keepRecordStreamStops))
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "KeepRecordingWhenStreamStops",
				ui->keepRecordStreamStops->isChecked());

	if (WidgetChanged(ui->replayWhileStreaming))
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "ReplayBufferWhileStreaming",
				ui->replayWhileStreaming->isChecked());
	if (WidgetChanged(ui->keepReplayStreamStops))
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "KeepReplayBufferStreamStops",
				ui->keepReplayStreamStops->isChecked());

	if (WidgetChanged(ui->systemTrayEnabled)) {
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "SysTrayEnabled",
				ui->systemTrayEnabled->isChecked());

		main->SystemTray(false);
	}

	if (WidgetChanged(ui->systemTrayWhenStarted))
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "SysTrayWhenStarted",
				ui->systemTrayWhenStarted->isChecked());

	if (WidgetChanged(ui->systemTrayAlways))
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "SysTrayMinimizeToTray",
				ui->systemTrayAlways->isChecked());

	if (WidgetChanged(ui->saveProjectors))
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "SaveProjectors",
				ui->saveProjectors->isChecked());

	if (WidgetChanged(ui->closeProjectors))
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "CloseExistingProjectors",
				ui->closeProjectors->isChecked());

	if (WidgetChanged(ui->studioPortraitLayout)) {
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "StudioPortraitLayout",
				ui->studioPortraitLayout->isChecked());

		main->ResetUI();
	}

	if (WidgetChanged(ui->prevProgLabelToggle)) {
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "StudioModeLabels",
				ui->prevProgLabelToggle->isChecked());

		main->ResetUI();
	}

	bool multiviewChanged = false;
	if (WidgetChanged(ui->multiviewMouseSwitch)) {
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "MultiviewMouseSwitch",
				ui->multiviewMouseSwitch->isChecked());
		multiviewChanged = true;
	}

	if (WidgetChanged(ui->multiviewDrawNames)) {
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "MultiviewDrawNames",
				ui->multiviewDrawNames->isChecked());
		multiviewChanged = true;
	}

	if (WidgetChanged(ui->multiviewDrawAreas)) {
		config_set_bool(App()->GetUserConfig(), "BasicWindow", "MultiviewDrawAreas",
				ui->multiviewDrawAreas->isChecked());
		multiviewChanged = true;
	}

	if (WidgetChanged(ui->multiviewLayout)) {
		config_set_int(App()->GetUserConfig(), "BasicWindow", "MultiviewLayout",
			       ui->multiviewLayout->currentData().toInt());
		multiviewChanged = true;
	}

	if (multiviewChanged)
		OBSProjector::UpdateMultiviewProjectors();
}

void OBSBasicSettings::SaveVideoSettings()
{
	QString baseResolution = ui->baseResolution->currentText();
	QString outputResolution = ui->outputResolution->currentText();
	int fpsType = ui->fpsType->currentIndex();
	uint32_t cx = 0, cy = 0;

	/* ------------------- */

	if (WidgetChanged(ui->baseResolution) && ConvertResText(QT_TO_UTF8(baseResolution), cx, cy)) {
		config_set_uint(main->Config(), "Video", "BaseCX", cx);
		config_set_uint(main->Config(), "Video", "BaseCY", cy);
	}

	if (WidgetChanged(ui->outputResolution) && ConvertResText(QT_TO_UTF8(outputResolution), cx, cy)) {
		config_set_uint(main->Config(), "Video", "OutputCX", cx);
		config_set_uint(main->Config(), "Video", "OutputCY", cy);
	}

	if (WidgetChanged(ui->fpsType))
		config_set_uint(main->Config(), "Video", "FPSType", fpsType);

	SaveCombo(ui->fpsCommon, "Video", "FPSCommon");
	SaveSpinBox(ui->fpsInteger, "Video", "FPSInt");
	SaveSpinBox(ui->fpsNumerator, "Video", "FPSNum");
	SaveSpinBox(ui->fpsDenominator, "Video", "FPSDen");
	SaveComboData(ui->downscaleFilter, "Video", "ScaleType");
}

void OBSBasicSettings::SaveAdvancedSettings()
{
	QString lastMonitoringDevice = config_get_string(main->Config(), "Audio", "MonitoringDeviceId");

#ifdef _WIN32
	if (WidgetChanged(ui->renderer))
		config_set_string(App()->GetAppConfig(), "Video", "Renderer", QT_TO_UTF8(ui->renderer->currentText()));

	std::string priority = QT_TO_UTF8(ui->processPriority->currentData().toString());
	config_set_string(App()->GetAppConfig(), "General", "ProcessPriority", priority.c_str());
	if (main->Active())
		SetProcessPriority(priority.c_str());

	SaveCheckBox(ui->enableNewSocketLoop, "Output", "NewSocketLoopEnable");
	SaveCheckBox(ui->enableLowLatencyMode, "Output", "LowLatencyEnable");
#endif
#if defined(_WIN32) || defined(__APPLE__)
	bool browserHWAccel = ui->browserHWAccel->isChecked();
	config_set_bool(App()->GetAppConfig(), "General", "BrowserHWAccel", browserHWAccel);
#endif

	if (WidgetChanged(ui->hotkeyFocusType)) {
		QString str = GetComboData(ui->hotkeyFocusType);
		config_set_string(App()->GetUserConfig(), "General", "HotkeyFocusType", QT_TO_UTF8(str));
	}

#ifdef __APPLE__
	if (WidgetChanged(ui->disableOSXVSync)) {
		bool disable = ui->disableOSXVSync->isChecked();
		config_set_bool(App()->GetAppConfig(), "Video", "DisableOSXVSync", disable);
		EnableOSXVSync(!disable);
	}
	if (WidgetChanged(ui->resetOSXVSync))
		config_set_bool(App()->GetAppConfig(), "Video", "ResetOSXVSyncOnExit", ui->resetOSXVSync->isChecked());
#endif

	SaveComboData(ui->colorFormat, "Video", "ColorFormat");
	SaveComboData(ui->colorSpace, "Video", "ColorSpace");
	SaveComboData(ui->colorRange, "Video", "ColorRange");
	SaveSpinBox(ui->sdrWhiteLevel, "Video", "SdrWhiteLevel");
	SaveSpinBox(ui->hdrNominalPeakLevel, "Video", "HdrNominalPeakLevel");
	if (obs_audio_monitoring_available()) {
		SaveCombo(ui->monitoringDevice, "Audio", "MonitoringDeviceName");
		SaveComboData(ui->monitoringDevice, "Audio", "MonitoringDeviceId");
	}

#ifdef _WIN32
	if (WidgetChanged(ui->disableAudioDucking)) {
		bool disable = ui->disableAudioDucking->isChecked();
		config_set_bool(App()->GetAppConfig(), "Audio", "DisableAudioDucking", disable);
		DisableAudioDucking(disable);
	}
#endif

	if (WidgetChanged(ui->confirmOnExit))
		config_set_bool(App()->GetUserConfig(), "General", "ConfirmOnExit", ui->confirmOnExit->isChecked());

	SaveEdit(ui->filenameFormatting, "Output", "FilenameFormatting");
	SaveEdit(ui->simpleRBPrefix, "SimpleOutput", "RecRBPrefix");
	SaveEdit(ui->simpleRBSuffix, "SimpleOutput", "RecRBSuffix");
	SaveCheckBox(ui->overwriteIfExists, "Output", "OverwriteIfExists");
	SaveCheckBox(ui->streamDelayEnable, "Output", "DelayEnable");
	SaveSpinBox(ui->streamDelaySec, "Output", "DelaySec");
	SaveCheckBox(ui->streamDelayPreserve, "Output", "DelayPreserve");
	SaveCheckBox(ui->reconnectEnable, "Output", "Reconnect");
	SaveSpinBox(ui->reconnectRetryDelay, "Output", "RetryDelay");
	SaveSpinBox(ui->reconnectMaxRetries, "Output", "MaxRetries");
	SaveComboData(ui->bindToIP, "Output", "BindIP");
	SaveComboData(ui->ipFamily, "Output", "IPFamily");
	SaveCheckBox(ui->autoRemux, "Video", "AutoRemux");
	SaveCheckBox(ui->dynBitrate, "Output", "DynamicBitrate");

	if (obs_audio_monitoring_available()) {
		QString newDevice = ui->monitoringDevice->currentData().toString();

		if (lastMonitoringDevice != newDevice) {
			obs_set_audio_monitoring_device(QT_TO_UTF8(ui->monitoringDevice->currentText()),
							QT_TO_UTF8(newDevice));

			blog(LOG_INFO, "Audio monitoring device:\n\tname: %s\n\tid: %s",
			     QT_TO_UTF8(ui->monitoringDevice->currentText()), QT_TO_UTF8(newDevice));
		}
	}
}

static inline const char *OutputModeFromIdx(int idx)
{
	if (idx == 1)
		return "Advanced";
	else
		return "Simple";
}

static inline const char *RecTypeFromIdx(int idx)
{
	if (idx == 1)
		return "FFmpeg";
	else
		return "Standard";
}

static inline const char *SplitFileTypeFromIdx(int idx)
{
	if (idx == 1)
		return "Size";
	else if (idx == 2)
		return "Manual";
	else
		return "Time";
}

static void WriteJsonData(OBSPropertiesView *view, const char *path)
{
	if (!view || !WidgetChanged(view))
		return;

	const OBSBasic *basic = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	const OBSProfile &currentProfile = basic->GetCurrentProfile();

	const std::filesystem::path jsonFilePath = currentProfile.path / std::filesystem::u8path(path);

	if (!jsonFilePath.empty()) {
		obs_data_t *settings = view->GetSettings();
		if (settings) {
			obs_data_save_json_safe(settings, jsonFilePath.u8string().c_str(), "tmp", "bak");
		}
	}
}

static void SaveTrackIndex(config_t *config, const char *section, const char *name, QAbstractButton *check1,
			   QAbstractButton *check2, QAbstractButton *check3, QAbstractButton *check4,
			   QAbstractButton *check5, QAbstractButton *check6)
{
	if (check1->isChecked())
		config_set_int(config, section, name, 1);
	else if (check2->isChecked())
		config_set_int(config, section, name, 2);
	else if (check3->isChecked())
		config_set_int(config, section, name, 3);
	else if (check4->isChecked())
		config_set_int(config, section, name, 4);
	else if (check5->isChecked())
		config_set_int(config, section, name, 5);
	else if (check6->isChecked())
		config_set_int(config, section, name, 6);
}

void OBSBasicSettings::SaveFormat(QComboBox *combo)
{
	QVariant v = combo->currentData();
	if (!v.isNull()) {
		auto format = v.value<FFmpegFormat>();
		config_set_string(main->Config(), "AdvOut", "FFFormat", format.name);
		config_set_string(main->Config(), "AdvOut", "FFFormatMimeType", format.mime_type);

		const char *ext = format.extensions;
		string extStr = ext ? ext : "";

		char *comma = strchr(&extStr[0], ',');
		if (comma)
			*comma = 0;

		config_set_string(main->Config(), "AdvOut", "FFExtension", extStr.c_str());
	} else {
		config_set_string(main->Config(), "AdvOut", "FFFormat", nullptr);
		config_set_string(main->Config(), "AdvOut", "FFFormatMimeType", nullptr);

		config_remove_value(main->Config(), "AdvOut", "FFExtension");
	}
}

void OBSBasicSettings::SaveEncoder(QComboBox *combo, const char *section, const char *value)
{
	QVariant v = combo->currentData();
	FFmpegCodec cd{};
	if (!v.isNull())
		cd = v.value<FFmpegCodec>();

	config_set_int(main->Config(), section, QT_TO_UTF8(QStringLiteral("%1Id").arg(value)), cd.id);
	if (cd.id != 0)
		config_set_string(main->Config(), section, value, cd.name);
	else
		config_set_string(main->Config(), section, value, nullptr);
}

void OBSBasicSettings::SaveOutputSettings()
{
	config_set_string(main->Config(), "Output", "Mode", OutputModeFromIdx(ui->outputMode->currentIndex()));

	QString encoder = ui->simpleOutStrEncoder->currentData().toString();
	const char *presetType;

	if (encoder == SIMPLE_ENCODER_QSV)
		presetType = "QSVPreset";
	else if (encoder == SIMPLE_ENCODER_QSV_AV1)
		presetType = "QSVPreset";
	else if (encoder == SIMPLE_ENCODER_NVENC)
		presetType = "NVENCPreset2";
	else if (encoder == SIMPLE_ENCODER_NVENC_AV1)
		presetType = "NVENCPreset2";
#ifdef ENABLE_HEVC
	else if (encoder == SIMPLE_ENCODER_AMD_HEVC)
		presetType = "AMDPreset";
	else if (encoder == SIMPLE_ENCODER_NVENC_HEVC)
		presetType = "NVENCPreset2";
#endif
	else if (encoder == SIMPLE_ENCODER_AMD)
		presetType = "AMDPreset";
	else if (encoder == SIMPLE_ENCODER_AMD_AV1)
		presetType = "AMDAV1Preset";
	else if (encoder == SIMPLE_ENCODER_APPLE_H264
#ifdef ENABLE_HEVC
		 || encoder == SIMPLE_ENCODER_APPLE_HEVC
#endif
	)
		/* The Apple encoders don't have presets like the other encoders
         do. This only exists to make sure that the x264 preset doesn't
         get overwritten with empty data. */
		presetType = "ApplePreset";
	else
		presetType = "Preset";

	SaveSpinBox(ui->simpleOutputVBitrate, "SimpleOutput", "VBitrate");
	SaveComboData(ui->simpleOutStrEncoder, "SimpleOutput", "StreamEncoder");
	SaveComboData(ui->simpleOutStrAEncoder, "SimpleOutput", "StreamAudioEncoder");
	SaveCombo(ui->simpleOutputABitrate, "SimpleOutput", "ABitrate");
	SaveEdit(ui->simpleOutputPath, "SimpleOutput", "FilePath");
	SaveCheckBox(ui->simpleNoSpace, "SimpleOutput", "FileNameWithoutSpace");
	SaveComboData(ui->simpleOutRecFormat, "SimpleOutput", "RecFormat2");
	SaveCheckBox(ui->simpleOutAdvanced, "SimpleOutput", "UseAdvanced");
	SaveComboData(ui->simpleOutPreset, "SimpleOutput", presetType);
	SaveEdit(ui->simpleOutCustom, "SimpleOutput", "x264Settings");
	SaveComboData(ui->simpleOutRecQuality, "SimpleOutput", "RecQuality");
	SaveComboData(ui->simpleOutRecEncoder, "SimpleOutput", "RecEncoder");
	SaveComboData(ui->simpleOutRecAEncoder, "SimpleOutput", "RecAudioEncoder");
	SaveEdit(ui->simpleOutMuxCustom, "SimpleOutput", "MuxerCustom");
	SaveGroupBox(ui->simpleReplayBuf, "SimpleOutput", "RecRB");
	SaveSpinBox(ui->simpleRBSecMax, "SimpleOutput", "RecRBTime");
	SaveSpinBox(ui->simpleRBMegsMax, "SimpleOutput", "RecRBSize");
	config_set_int(main->Config(), "SimpleOutput", "RecTracks", SimpleOutGetSelectedAudioTracks());

	curAdvStreamEncoder = GetComboData(ui->advOutEncoder);

	SaveComboData(ui->advOutEncoder, "AdvOut", "Encoder");
	SaveComboData(ui->advOutAEncoder, "AdvOut", "AudioEncoder");
	SaveCombo(ui->advOutRescale, "AdvOut", "RescaleRes");
	SaveComboData(ui->advOutRescaleFilter, "AdvOut", "RescaleFilter");
	SaveTrackIndex(main->Config(), "AdvOut", "TrackIndex", ui->advOutTrack1, ui->advOutTrack2, ui->advOutTrack3,
		       ui->advOutTrack4, ui->advOutTrack5, ui->advOutTrack6);
	config_set_int(main->Config(), "AdvOut", "StreamMultiTrackAudioMixes", AdvOutGetStreamingSelectedAudioTracks());
	config_set_string(main->Config(), "AdvOut", "RecType", RecTypeFromIdx(ui->advOutRecType->currentIndex()));

	curAdvRecordEncoder = GetComboData(ui->advOutRecEncoder);

	SaveEdit(ui->advOutRecPath, "AdvOut", "RecFilePath");
	SaveCheckBox(ui->advOutNoSpace, "AdvOut", "RecFileNameWithoutSpace");
	SaveComboData(ui->advOutRecFormat, "AdvOut", "RecFormat2");
	SaveComboData(ui->advOutRecEncoder, "AdvOut", "RecEncoder");
	SaveComboData(ui->advOutRecAEncoder, "AdvOut", "RecAudioEncoder");
	SaveCombo(ui->advOutRecRescale, "AdvOut", "RecRescaleRes");
	SaveComboData(ui->advOutRecRescaleFilter, "AdvOut", "RecRescaleFilter");
	SaveEdit(ui->advOutMuxCustom, "AdvOut", "RecMuxerCustom");
	SaveCheckBox(ui->advOutSplitFile, "AdvOut", "RecSplitFile");
	config_set_string(main->Config(), "AdvOut", "RecSplitFileType",
			  SplitFileTypeFromIdx(ui->advOutSplitFileType->currentIndex()));
	SaveSpinBox(ui->advOutSplitFileTime, "AdvOut", "RecSplitFileTime");
	SaveSpinBox(ui->advOutSplitFileSize, "AdvOut", "RecSplitFileSize");

	config_set_int(main->Config(), "AdvOut", "RecTracks", AdvOutGetSelectedAudioTracks());

	config_set_int(main->Config(), "AdvOut", "FLVTrack", CurrentFLVTrack());

	config_set_bool(main->Config(), "AdvOut", "FFOutputToFile",
			ui->advOutFFType->currentIndex() == 0 ? true : false);
	SaveEdit(ui->advOutFFRecPath, "AdvOut", "FFFilePath");
	SaveCheckBox(ui->advOutFFNoSpace, "AdvOut", "FFFileNameWithoutSpace");
	SaveEdit(ui->advOutFFURL, "AdvOut", "FFURL");
	SaveFormat(ui->advOutFFFormat);
	SaveEdit(ui->advOutFFMCfg, "AdvOut", "FFMCustom");
	SaveSpinBox(ui->advOutFFVBitrate, "AdvOut", "FFVBitrate");
	SaveSpinBox(ui->advOutFFVGOPSize, "AdvOut", "FFVGOPSize");
	SaveCheckBox(ui->advOutFFUseRescale, "AdvOut", "FFRescale");
	SaveCheckBox(ui->advOutFFIgnoreCompat, "AdvOut", "FFIgnoreCompat");
	SaveCombo(ui->advOutFFRescale, "AdvOut", "FFRescaleRes");
	SaveEncoder(ui->advOutFFVEncoder, "AdvOut", "FFVEncoder");
	SaveEdit(ui->advOutFFVCfg, "AdvOut", "FFVCustom");
	SaveSpinBox(ui->advOutFFABitrate, "AdvOut", "FFABitrate");
	SaveEncoder(ui->advOutFFAEncoder, "AdvOut", "FFAEncoder");
	SaveEdit(ui->advOutFFACfg, "AdvOut", "FFACustom");
	config_set_int(main->Config(), "AdvOut", "FFAudioMixes",
		       (ui->advOutFFTrack1->isChecked() ? (1 << 0) : 0) |
			       (ui->advOutFFTrack2->isChecked() ? (1 << 1) : 0) |
			       (ui->advOutFFTrack3->isChecked() ? (1 << 2) : 0) |
			       (ui->advOutFFTrack4->isChecked() ? (1 << 3) : 0) |
			       (ui->advOutFFTrack5->isChecked() ? (1 << 4) : 0) |
			       (ui->advOutFFTrack6->isChecked() ? (1 << 5) : 0));
	SaveCombo(ui->advOutTrack1Bitrate, "AdvOut", "Track1Bitrate");
	SaveCombo(ui->advOutTrack2Bitrate, "AdvOut", "Track2Bitrate");
	SaveCombo(ui->advOutTrack3Bitrate, "AdvOut", "Track3Bitrate");
	SaveCombo(ui->advOutTrack4Bitrate, "AdvOut", "Track4Bitrate");
	SaveCombo(ui->advOutTrack5Bitrate, "AdvOut", "Track5Bitrate");
	SaveCombo(ui->advOutTrack6Bitrate, "AdvOut", "Track6Bitrate");
	SaveEdit(ui->advOutTrack1Name, "AdvOut", "Track1Name");
	SaveEdit(ui->advOutTrack2Name, "AdvOut", "Track2Name");
	SaveEdit(ui->advOutTrack3Name, "AdvOut", "Track3Name");
	SaveEdit(ui->advOutTrack4Name, "AdvOut", "Track4Name");
	SaveEdit(ui->advOutTrack5Name, "AdvOut", "Track5Name");
	SaveEdit(ui->advOutTrack6Name, "AdvOut", "Track6Name");

	if (vodTrackCheckbox) {
		SaveCheckBox(simpleVodTrack, "SimpleOutput", "VodTrackEnabled");
		SaveCheckBox(vodTrackCheckbox, "AdvOut", "VodTrackEnabled");
		SaveTrackIndex(main->Config(), "AdvOut", "VodTrackIndex", vodTrack[0], vodTrack[1], vodTrack[2],
			       vodTrack[3], vodTrack[4], vodTrack[5]);
	}

	SaveCheckBox(ui->advReplayBuf, "AdvOut", "RecRB");
	SaveSpinBox(ui->advRBSecMax, "AdvOut", "RecRBTime");
	SaveSpinBox(ui->advRBMegsMax, "AdvOut", "RecRBSize");

	WriteJsonData(streamEncoderProps, "streamEncoder.json");
	WriteJsonData(recordEncoderProps, "recordEncoder.json");
	main->ResetOutputs();
}

void OBSBasicSettings::SaveAudioSettings()
{
	QString sampleRateStr = ui->sampleRate->currentText();
	int channelSetupIdx = ui->channelSetup->currentIndex();

	const char *channelSetup;
	switch (channelSetupIdx) {
	case 0:
		channelSetup = "Mono";
		break;
	case 1:
		channelSetup = "Stereo";
		break;
	case 2:
		channelSetup = "2.1";
		break;
	case 3:
		channelSetup = "4.0";
		break;
	case 4:
		channelSetup = "4.1";
		break;
	case 5:
		channelSetup = "5.1";
		break;
	case 6:
		channelSetup = "7.1";
		break;

	default:
		channelSetup = "Stereo";
		break;
	}

	int sampleRate = 44100;
	if (sampleRateStr == "48 kHz")
		sampleRate = 48000;

	if (WidgetChanged(ui->sampleRate))
		config_set_uint(main->Config(), "Audio", "SampleRate", sampleRate);

	if (WidgetChanged(ui->channelSetup))
		config_set_string(main->Config(), "Audio", "ChannelSetup", channelSetup);

	if (WidgetChanged(ui->meterDecayRate)) {
		double meterDecayRate;
		switch (ui->meterDecayRate->currentIndex()) {
		case 0:
			meterDecayRate = VOLUME_METER_DECAY_FAST;
			break;
		case 1:
			meterDecayRate = VOLUME_METER_DECAY_MEDIUM;
			break;
		case 2:
			meterDecayRate = VOLUME_METER_DECAY_SLOW;
			break;
		default:
			meterDecayRate = VOLUME_METER_DECAY_FAST;
			break;
		}
		config_set_double(main->Config(), "Audio", "MeterDecayRate", meterDecayRate);

		main->UpdateVolumeControlsDecayRate();
	}

	if (WidgetChanged(ui->peakMeterType)) {
		uint32_t peakMeterTypeIdx = ui->peakMeterType->currentIndex();
		config_set_uint(main->Config(), "Audio", "PeakMeterType", peakMeterTypeIdx);

		main->UpdateVolumeControlsPeakMeterType();
	}

	if (WidgetChanged(ui->lowLatencyBuffering)) {
		bool enableLLAudioBuffering = ui->lowLatencyBuffering->isChecked();
		config_set_bool(App()->GetUserConfig(), "Audio", "LowLatencyAudioBuffering", enableLLAudioBuffering);
	}

	for (auto &audioSource : audioSources) {
		auto source = OBSGetStrongRef(get<0>(audioSource));
		if (!source)
			continue;

		auto &ptmCB = get<1>(audioSource);
		auto &ptmSB = get<2>(audioSource);
		auto &pttCB = get<3>(audioSource);
		auto &pttSB = get<4>(audioSource);

		obs_source_enable_push_to_mute(source, ptmCB->isChecked());
		obs_source_set_push_to_mute_delay(source, ptmSB->value());

		obs_source_enable_push_to_talk(source, pttCB->isChecked());
		obs_source_set_push_to_talk_delay(source, pttSB->value());
	}

	auto UpdateAudioDevice = [this](bool input, QComboBox *combo, const char *name, int index) {
		main->ResetAudioDevice(input ? App()->InputAudioSource() : App()->OutputAudioSource(),
				       QT_TO_UTF8(GetComboData(combo)), Str(name), index);
	};

	UpdateAudioDevice(false, ui->desktopAudioDevice1, "Basic.DesktopDevice1", 1);
	UpdateAudioDevice(false, ui->desktopAudioDevice2, "Basic.DesktopDevice2", 2);
	UpdateAudioDevice(true, ui->auxAudioDevice1, "Basic.AuxDevice1", 3);
	UpdateAudioDevice(true, ui->auxAudioDevice2, "Basic.AuxDevice2", 4);
	UpdateAudioDevice(true, ui->auxAudioDevice3, "Basic.AuxDevice3", 5);
	UpdateAudioDevice(true, ui->auxAudioDevice4, "Basic.AuxDevice4", 6);
	main->SaveProject();
}

void OBSBasicSettings::SaveHotkeySettings()
{
	const auto &config = main->Config();

	using namespace std;

	std::vector<obs_key_combination> combinations;
	for (auto &hotkey : hotkeys) {
		auto &hw = *hotkey.second;
		if (!hw.Changed())
			continue;

		hw.Save(combinations);

		if (!hotkey.first)
			continue;

		OBSDataArrayAutoRelease array = obs_hotkey_save(hw.id);
		OBSDataAutoRelease data = obs_data_create();
		obs_data_set_array(data, "bindings", array);
		const char *json = obs_data_get_json(data);
		config_set_string(config, "Hotkeys", hw.name.c_str(), json);
	}

	if (!main->outputHandler || !main->outputHandler->replayBuffer)
		return;

	const char *id = obs_obj_get_id(main->outputHandler->replayBuffer);
	if (strcmp(id, "replay_buffer") == 0) {
		OBSDataAutoRelease hotkeys = obs_hotkeys_save_output(main->outputHandler->replayBuffer);
		config_set_string(config, "Hotkeys", "ReplayBuffer", obs_data_get_json(hotkeys));
	}
}

#define MINOR_SEPARATOR "------------------------------------------------"

static void AddChangedVal(std::string &changed, const char *str)
{
	if (changed.size())
		changed += ", ";
	changed += str;
}

void OBSBasicSettings::SaveSettings()
{
	if (generalChanged)
		SaveGeneralSettings();
	if (stream1Changed)
		SaveStream1Settings();
	if (outputsChanged)
		SaveOutputSettings();
	if (audioChanged)
		SaveAudioSettings();
	if (videoChanged)
		SaveVideoSettings();
	if (hotkeysChanged)
		SaveHotkeySettings();
	if (a11yChanged)
		SaveA11ySettings();
	if (advancedChanged)
		SaveAdvancedSettings();
	if (appearanceChanged)
		SaveAppearanceSettings();
	if (videoChanged || advancedChanged)
		main->ResetVideo();

	config_save_safe(main->Config(), "tmp", nullptr);
	config_save_safe(App()->GetUserConfig(), "tmp", nullptr);
	main->SaveProject();

	if (Changed()) {
		std::string changed;
		if (generalChanged)
			AddChangedVal(changed, "general");
		if (stream1Changed)
			AddChangedVal(changed, "stream 1");
		if (outputsChanged)
			AddChangedVal(changed, "outputs");
		if (audioChanged)
			AddChangedVal(changed, "audio");
		if (videoChanged)
			AddChangedVal(changed, "video");
		if (hotkeysChanged)
			AddChangedVal(changed, "hotkeys");
		if (a11yChanged)
			AddChangedVal(changed, "a11y");
		if (appearanceChanged)
			AddChangedVal(changed, "appearance");
		if (advancedChanged)
			AddChangedVal(changed, "advanced");

		blog(LOG_INFO, "Settings changed (%s)", changed.c_str());
		blog(LOG_INFO, MINOR_SEPARATOR);
	}

	bool langChanged = (ui->language->currentIndex() != prevLangIndex);
	bool audioRestart =
		(ui->channelSetup->currentIndex() != channelIndex || ui->sampleRate->currentIndex() != sampleRateIndex);
	bool browserHWAccelChanged = (ui->browserHWAccel && ui->browserHWAccel->isChecked() != prevBrowserAccel);

	if (langChanged || audioRestart || browserHWAccelChanged)
		restart = true;
	else
		restart = false;
}

bool OBSBasicSettings::QueryChanges()
{
	QMessageBox::StandardButton button;

	button = OBSMessageBox::question(this, QTStr("Basic.Settings.ConfirmTitle"), QTStr("Basic.Settings.Confirm"),
					 QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

	if (button == QMessageBox::Cancel) {
		return false;
	} else if (button == QMessageBox::Yes) {
		if (!QueryAllowedToClose())
			return false;

		SaveSettings();
	} else {
		if (savedTheme != App()->GetTheme())
			App()->SetTheme(savedTheme->id);

		LoadSettings(true);
		restart = false;
	}

	ClearChanged();
	return true;
}

bool OBSBasicSettings::QueryAllowedToClose()
{
	bool simple = (ui->outputMode->currentIndex() == 0);

	bool invalidEncoder = false;
	bool invalidFormat = false;
	bool invalidTracks = false;
	if (simple) {
		if (ui->simpleOutRecEncoder->currentIndex() == -1 || ui->simpleOutStrEncoder->currentIndex() == -1 ||
		    ui->simpleOutRecAEncoder->currentIndex() == -1 || ui->simpleOutStrAEncoder->currentIndex() == -1)
			invalidEncoder = true;

		if (ui->simpleOutRecFormat->currentIndex() == -1)
			invalidFormat = true;

		QString qual = ui->simpleOutRecQuality->currentData().toString();
		QString format = ui->simpleOutRecFormat->currentData().toString();
		if (SimpleOutGetSelectedAudioTracks() == 0 && qual != "Stream" && format != "flv")
			invalidTracks = true;
	} else {
		if (ui->advOutRecEncoder->currentIndex() == -1 || ui->advOutEncoder->currentIndex() == -1 ||
		    ui->advOutRecAEncoder->currentIndex() == -1 || ui->advOutAEncoder->currentIndex() == -1)
			invalidEncoder = true;

		QString format = ui->advOutRecFormat->currentData().toString();
		if (AdvOutGetSelectedAudioTracks() == 0 && format != "flv")
			invalidTracks = true;
		if (AdvOutGetStreamingSelectedAudioTracks() == 0)
			invalidTracks = true;
	}

	if (invalidEncoder) {
		OBSMessageBox::warning(this, QTStr("CodecCompat.CodecMissingOnExit.Title"),
				       QTStr("CodecCompat.CodecMissingOnExit.Text"));
		return false;
	} else if (invalidFormat) {
		OBSMessageBox::warning(this, QTStr("CodecCompat.ContainerMissingOnExit.Title"),
				       QTStr("CodecCompat.ContainerMissingOnExit.Text"));
		return false;
	} else if (invalidTracks) {
		OBSMessageBox::warning(this, QTStr("OutputWarnings.NoTracksSelectedOnExit.Title"),
				       QTStr("OutputWarnings.NoTracksSelectedOnExit.Text"));
		return false;
	}

	return true;
}

void OBSBasicSettings::closeEvent(QCloseEvent *event)
{
	if (!AskIfCanCloseSettings())
		event->ignore();
}

void OBSBasicSettings::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);

	/* Reduce the height of the widget area if too tall compared to the screen
	 * size (e.g., 720p) with potential window decoration (e.g., titlebar). */
	const int titleBarHeight = QApplication::style()->pixelMetric(QStyle::PM_TitleBarHeight);
	const int maxHeight = round(screen()->availableGeometry().height() - titleBarHeight);
	if (size().height() >= maxHeight)
		resize(size().width(), maxHeight);
}

void OBSBasicSettings::reject()
{
	if (AskIfCanCloseSettings())
		close();
}

void OBSBasicSettings::on_listWidget_itemSelectionChanged()
{
	int row = ui->listWidget->currentRow();

	if (loading || row == pageIndex)
		return;

	if (!hotkeysLoaded && row == Pages::HOTKEYS) {
		setCursor(Qt::BusyCursor);
		/* Look, I know this /feels/ wrong, but the specific issue we're dealing with
		 * here means that the UI locks up immediately even when using "invokeMethod".
		 * So the only way for the user to see the loading message on the page is to
		 * give the Qt event loop a tiny bit of time to switch to the hotkey page,
		 * and only then start loading. This could maybe be done by subclassing QWidget
		 * for the hotkey page and then using showEvent() but I *really* don't want
		 * to deal with that right now. I've got better things to do with my life
		 * than to work around this god damn stupid issue for something we'll remove
		 * soon enough anyway. So this solution it is. */
		QTimer::singleShot(1, this, [&]() { LoadHotkeySettings(); });
	}

	pageIndex = row;
}

void OBSBasicSettings::UpdateYouTubeAppDockSettings()
{
#if defined(BROWSER_AVAILABLE) && defined(YOUTUBE_ENABLED)
	if (cef_js_avail) {
		std::string service = ui->service->currentText().toStdString();
		if (IsYouTubeService(service)) {
			if (!main->GetYouTubeAppDock()) {
				main->NewYouTubeAppDock();
			}
			main->GetYouTubeAppDock()->SettingsUpdated(!IsYouTubeService(service) || stream1Changed);
		} else {
			if (main->GetYouTubeAppDock()) {
				main->GetYouTubeAppDock()->AccountDisconnected();
			}
			main->DeleteYouTubeAppDock();
		}
	}
#endif
}

void OBSBasicSettings::on_buttonBox_clicked(QAbstractButton *button)
{
	QDialogButtonBox::ButtonRole val = ui->buttonBox->buttonRole(button);

	if (val == QDialogButtonBox::ApplyRole || val == QDialogButtonBox::AcceptRole) {
		if (!QueryAllowedToClose())
			return;

		SaveSettings();

		UpdateYouTubeAppDockSettings();
		ClearChanged();
	}

	if (val == QDialogButtonBox::AcceptRole || val == QDialogButtonBox::RejectRole) {
		if (val == QDialogButtonBox::RejectRole) {
			if (savedTheme != App()->GetTheme())
				App()->SetTheme(savedTheme->id);
		}
		ClearChanged();
		close();
	}
}

void OBSBasicSettings::on_simpleOutputBrowse_clicked()
{
	QString dir =
		SelectDirectory(this, QTStr("Basic.Settings.Output.SelectDirectory"), ui->simpleOutputPath->text());
	if (dir.isEmpty())
		return;

	ui->simpleOutputPath->setText(dir);
}

void OBSBasicSettings::on_advOutRecPathBrowse_clicked()
{
	QString dir = SelectDirectory(this, QTStr("Basic.Settings.Output.SelectDirectory"), ui->advOutRecPath->text());
	if (dir.isEmpty())
		return;

	ui->advOutRecPath->setText(dir);
}

void OBSBasicSettings::on_advOutFFPathBrowse_clicked()
{
	QString dir = SelectDirectory(this, QTStr("Basic.Settings.Output.SelectDirectory"), ui->advOutRecPath->text());
	if (dir.isEmpty())
		return;

	ui->advOutFFRecPath->setText(dir);
}

void OBSBasicSettings::on_advOutEncoder_currentIndexChanged()
{
	QString encoder = GetComboData(ui->advOutEncoder);
	if (!loading) {
		bool loadSettings = encoder == curAdvStreamEncoder;

		delete streamEncoderProps;
		streamEncoderProps = CreateEncoderPropertyView(QT_TO_UTF8(encoder),
							       loadSettings ? "streamEncoder.json" : nullptr, true);
		streamEncoderProps->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
		ui->advOutEncoderLayout->addWidget(streamEncoderProps);
	}

	ui->advOutUseRescale->setVisible(true);
	ui->advOutRescale->setVisible(true);
}

void OBSBasicSettings::on_advOutRecEncoder_currentIndexChanged(int idx)
{
	if (!loading) {
		delete recordEncoderProps;
		recordEncoderProps = nullptr;
	}

	if (idx <= 0) {
		ui->advOutRecUseRescale->setVisible(false);
		ui->advOutRecRescaleContainer->setVisible(false);
		ui->advOutRecEncoderProps->setVisible(false);
		return;
	}

	QString encoder = GetComboData(ui->advOutRecEncoder);
	bool loadSettings = encoder == curAdvRecordEncoder;

	if (!loading) {
		recordEncoderProps = CreateEncoderPropertyView(QT_TO_UTF8(encoder),
							       loadSettings ? "recordEncoder.json" : nullptr, true);
		recordEncoderProps->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
		ui->advOutRecEncoderProps->layout()->addWidget(recordEncoderProps);
		connect(recordEncoderProps, &OBSPropertiesView::Changed, this,
			&OBSBasicSettings::AdvReplayBufferChanged);
	}

	ui->advOutRecUseRescale->setVisible(true);
	ui->advOutRecRescaleContainer->setVisible(true);
	ui->advOutRecEncoderProps->setVisible(true);
}

void OBSBasicSettings::on_advOutFFIgnoreCompat_stateChanged(int)
{
	/* Little hack to reload codecs when checked */
	on_advOutFFFormat_currentIndexChanged(ui->advOutFFFormat->currentIndex());
}

#define DEFAULT_CONTAINER_STR QTStr("Basic.Settings.Output.Adv.FFmpeg.FormatDescDef")

void OBSBasicSettings::on_advOutFFFormat_currentIndexChanged(int idx)
{
	const QVariant itemDataVariant = ui->advOutFFFormat->itemData(idx);

	if (!itemDataVariant.isNull()) {
		auto format = itemDataVariant.value<FFmpegFormat>();
		SetAdvOutputFFmpegEnablement(FFmpegCodecType::AUDIO, format.HasAudio(), false);
		SetAdvOutputFFmpegEnablement(FFmpegCodecType::VIDEO, format.HasVideo(), false);
		ReloadCodecs(format);

		ui->advOutFFFormatDesc->setText(format.long_name);

		FFmpegCodec defaultAudioCodecDesc = format.GetDefaultEncoder(FFmpegCodecType::AUDIO);
		FFmpegCodec defaultVideoCodecDesc = format.GetDefaultEncoder(FFmpegCodecType::VIDEO);
		SelectEncoder(ui->advOutFFAEncoder, defaultAudioCodecDesc.name, defaultAudioCodecDesc.id);
		SelectEncoder(ui->advOutFFVEncoder, defaultVideoCodecDesc.name, defaultVideoCodecDesc.id);
	} else {
		ui->advOutFFAEncoder->blockSignals(true);
		ui->advOutFFVEncoder->blockSignals(true);
		ui->advOutFFAEncoder->clear();
		ui->advOutFFVEncoder->clear();

		ui->advOutFFFormatDesc->setText(DEFAULT_CONTAINER_STR);
	}
}

void OBSBasicSettings::on_advOutFFAEncoder_currentIndexChanged(int idx)
{
	const QVariant itemDataVariant = ui->advOutFFAEncoder->itemData(idx);
	if (!itemDataVariant.isNull()) {
		auto desc = itemDataVariant.value<FFmpegCodec>();
		SetAdvOutputFFmpegEnablement(FFmpegCodecType::AUDIO, desc.id != 0 || desc.name != nullptr, true);
	}
}

void OBSBasicSettings::on_advOutFFVEncoder_currentIndexChanged(int idx)
{
	const QVariant itemDataVariant = ui->advOutFFVEncoder->itemData(idx);
	if (!itemDataVariant.isNull()) {
		auto desc = itemDataVariant.value<FFmpegCodec>();
		SetAdvOutputFFmpegEnablement(FFmpegCodecType::VIDEO, desc.id != 0 || desc.name != nullptr, true);
	}
}

void OBSBasicSettings::on_advOutFFType_currentIndexChanged(int idx)
{
	ui->advOutFFNoSpace->setHidden(idx != 0);
}

void OBSBasicSettings::on_colorFormat_currentIndexChanged(int)
{
	UpdateColorFormatSpaceWarning();
}

void OBSBasicSettings::on_colorSpace_currentIndexChanged(int)
{
	UpdateColorFormatSpaceWarning();
}

#define INVALID_RES_STR "Basic.Settings.Video.InvalidResolution"

static bool ValidResolutions(Ui::OBSBasicSettings *ui)
{
	QString baseRes = ui->baseResolution->lineEdit()->text();
	uint32_t cx, cy;

	if (!ConvertResText(QT_TO_UTF8(baseRes), cx, cy)) {
		ui->videoMsg->setText(QTStr(INVALID_RES_STR));
		return false;
	}

	bool lockedOutRes = !ui->outputResolution->isEditable();
	if (!lockedOutRes) {
		QString outRes = ui->outputResolution->lineEdit()->text();
		if (!ConvertResText(QT_TO_UTF8(outRes), cx, cy)) {
			ui->videoMsg->setText(QTStr(INVALID_RES_STR));
			return false;
		}
	}

	ui->videoMsg->setText("");
	return true;
}

void OBSBasicSettings::RecalcOutputResPixels(const char *resText)
{
	uint32_t newCX;
	uint32_t newCY;

	if (ConvertResText(resText, newCX, newCY) && newCX && newCY) {
		outputCX = newCX;
		outputCY = newCY;

		std::tuple<int, int> aspect = aspect_ratio(outputCX, outputCY);

		ui->scaledAspect->setText(
			QTStr("AspectRatio")
				.arg(QString::number(std::get<0>(aspect)), QString::number(std::get<1>(aspect))));
	}
}

bool OBSBasicSettings::AskIfCanCloseSettings()
{
	bool canCloseSettings = false;

	if (!Changed() || QueryChanges())
		canCloseSettings = true;

	if (forceAuthReload) {
		main->auth->Save();
		main->auth->Load();
		forceAuthReload = false;
	}

	if (forceUpdateCheck) {
		main->CheckForUpdates(false);
		forceUpdateCheck = false;
	}

	return canCloseSettings;
}

void OBSBasicSettings::on_filenameFormatting_textEdited(const QString &text)
{
	QString safeStr = text;

#ifdef __APPLE__
	safeStr.replace(QRegularExpression("[:]"), "");
#elif defined(_WIN32)
	safeStr.replace(QRegularExpression("[<>:\"\\|\\?\\*]"), "");
#else
	// TODO: Add filtering for other platforms
#endif

	if (text != safeStr)
		ui->filenameFormatting->setText(safeStr);
}

void OBSBasicSettings::on_outputResolution_editTextChanged(const QString &text)
{
	if (!loading) {
		RecalcOutputResPixels(QT_TO_UTF8(text));
		LoadDownscaleFilters();
	}
}

void OBSBasicSettings::on_baseResolution_editTextChanged(const QString &text)
{
	if (!loading && ValidResolutions(ui.get())) {
		QString baseResolution = text;
		uint32_t cx, cy;

		ConvertResText(QT_TO_UTF8(baseResolution), cx, cy);

		std::tuple<int, int> aspect = aspect_ratio(cx, cy);

		ui->baseAspect->setText(
			QTStr("AspectRatio")
				.arg(QString::number(std::get<0>(aspect)), QString::number(std::get<1>(aspect))));

		ResetDownscales(cx, cy);
	}
}

void OBSBasicSettings::GeneralChanged()
{
	if (!loading) {
		generalChanged = true;
		sender()->setProperty("changed", QVariant(true));
		EnableApplyButton(true);
	}
}

void OBSBasicSettings::Stream1Changed()
{
	if (!loading) {
		stream1Changed = true;
		sender()->setProperty("changed", QVariant(true));
		EnableApplyButton(true);
	}
}

void OBSBasicSettings::OutputsChanged()
{
	if (!loading) {
		outputsChanged = true;
		sender()->setProperty("changed", QVariant(true));
		EnableApplyButton(true);

		UpdateMultitrackVideo();
	}
}

void OBSBasicSettings::AudioChanged()
{
	if (!loading) {
		audioChanged = true;
		sender()->setProperty("changed", QVariant(true));
		EnableApplyButton(true);
	}
}

void OBSBasicSettings::AudioChangedRestart()
{
	ui->audioMsg->setVisible(false);

	if (!loading) {
		int currentChannelIndex = ui->channelSetup->currentIndex();
		int currentSampleRateIndex = ui->sampleRate->currentIndex();
		bool currentLLAudioBufVal = ui->lowLatencyBuffering->isChecked();

		if (currentChannelIndex != channelIndex || currentSampleRateIndex != sampleRateIndex ||
		    currentLLAudioBufVal != llBufferingEnabled) {
			ui->audioMsg->setText(QTStr("Basic.Settings.ProgramRestart"));
			ui->audioMsg->setVisible(true);
		} else {
			ui->audioMsg->setText("");
		}

		audioChanged = true;
		sender()->setProperty("changed", QVariant(true));
		EnableApplyButton(true);
	}
}

void OBSBasicSettings::ReloadAudioSources()
{
	LoadAudioSources();
}

#define MULTI_CHANNEL_WARNING "Basic.Settings.Audio.MultichannelWarning"

void OBSBasicSettings::SpeakerLayoutChanged(int idx)
{
	QString speakerLayoutQstr = ui->channelSetup->itemText(idx);
	std::string speakerLayout = QT_TO_UTF8(speakerLayoutQstr);
	bool surround = IsSurround(speakerLayout.c_str());
	bool isOpus = ui->simpleOutStrAEncoder->currentData().toString() == "opus";

	if (surround) {
		/*
		 * Display all bitrates
		 */
		PopulateSimpleBitrates(ui->simpleOutputABitrate, isOpus);

		string stream_encoder_id = ui->advOutAEncoder->currentData().toString().toStdString();
		string record_encoder_id = ui->advOutRecAEncoder->currentData().toString().toStdString();
		PopulateAdvancedBitrates({ui->advOutTrack1Bitrate, ui->advOutTrack2Bitrate, ui->advOutTrack3Bitrate,
					  ui->advOutTrack4Bitrate, ui->advOutTrack5Bitrate, ui->advOutTrack6Bitrate},
					 stream_encoder_id.c_str(),
					 record_encoder_id == "none" ? stream_encoder_id.c_str()
								     : record_encoder_id.c_str());
	} else {
		/*
		 * Reset audio bitrate for simple and adv mode, update list of
		 * bitrates and save setting.
		 */
		RestrictResetBitrates({ui->simpleOutputABitrate, ui->advOutTrack1Bitrate, ui->advOutTrack2Bitrate,
				       ui->advOutTrack3Bitrate, ui->advOutTrack4Bitrate, ui->advOutTrack5Bitrate,
				       ui->advOutTrack6Bitrate},
				      320);

		SaveCombo(ui->simpleOutputABitrate, "SimpleOutput", "ABitrate");
		SaveCombo(ui->advOutTrack1Bitrate, "AdvOut", "Track1Bitrate");
		SaveCombo(ui->advOutTrack2Bitrate, "AdvOut", "Track2Bitrate");
		SaveCombo(ui->advOutTrack3Bitrate, "AdvOut", "Track3Bitrate");
		SaveCombo(ui->advOutTrack4Bitrate, "AdvOut", "Track4Bitrate");
		SaveCombo(ui->advOutTrack5Bitrate, "AdvOut", "Track5Bitrate");
		SaveCombo(ui->advOutTrack6Bitrate, "AdvOut", "Track6Bitrate");
	}

	UpdateAudioWarnings();
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
void OBSBasicSettings::HideOBSWindowWarning(Qt::CheckState state)
#else
void OBSBasicSettings::HideOBSWindowWarning(int state)
#endif
{
	if (loading || state == Qt::Unchecked)
		return;

	if (config_get_bool(App()->GetUserConfig(), "General", "WarnedAboutHideOBSFromCapture"))
		return;

	OBSMessageBox::information(this, QTStr("Basic.Settings.General.HideOBSWindowsFromCapture"),
				   QTStr("Basic.Settings.General.HideOBSWindowsFromCapture.Message"));

	config_set_bool(App()->GetUserConfig(), "General", "WarnedAboutHideOBSFromCapture", true);
	config_save_safe(App()->GetUserConfig(), "tmp", nullptr);
}

/*
 * resets current bitrate if too large and restricts the number of bitrates
 * displayed when multichannel OFF
 */

void RestrictResetBitrates(initializer_list<QComboBox *> boxes, int maxbitrate)
{
	for (auto box : boxes) {
		int idx = box->currentIndex();
		int max_bitrate = FindClosestAvailableAudioBitrate(box, maxbitrate);
		int count = box->count();
		int max_idx = box->findText(QT_UTF8(std::to_string(max_bitrate).c_str()));

		for (int i = (count - 1); i > max_idx; i--)
			box->removeItem(i);

		if (idx > max_idx) {
			int default_bitrate = FindClosestAvailableAudioBitrate(box, maxbitrate / 2);
			int default_idx = box->findText(QT_UTF8(std::to_string(default_bitrate).c_str()));

			box->setCurrentIndex(default_idx);
			box->setProperty("changed", QVariant(true));
		} else {
			box->setCurrentIndex(idx);
		}
	}
}

void OBSBasicSettings::AdvancedChangedRestart()
{
	ui->advancedMsg->setVisible(false);

	if (!loading) {
		advancedChanged = true;
		ui->advancedMsg->setText(QTStr("Basic.Settings.ProgramRestart"));
		ui->advancedMsg->setVisible(true);
		sender()->setProperty("changed", QVariant(true));
		EnableApplyButton(true);
	}
}

void OBSBasicSettings::VideoChangedResolution()
{
	if (!loading && ValidResolutions(ui.get())) {
		videoChanged = true;
		sender()->setProperty("changed", QVariant(true));
		EnableApplyButton(true);
	}
}

void OBSBasicSettings::VideoChanged()
{
	if (!loading) {
		videoChanged = true;
		sender()->setProperty("changed", QVariant(true));
		EnableApplyButton(true);
	}
}

void OBSBasicSettings::HotkeysChanged()
{
	using namespace std;
	if (loading)
		return;

	hotkeysChanged = any_of(begin(hotkeys), end(hotkeys), [](const pair<bool, QPointer<OBSHotkeyWidget>> &hotkey) {
		const auto &hw = *hotkey.second;
		return hw.Changed();
	});

	if (hotkeysChanged)
		EnableApplyButton(true);
}

void OBSBasicSettings::SearchHotkeys(const QString &text, obs_key_combination_t filterCombo)
{

	if (ui->hotkeyFormLayout->rowCount() == 0)
		return;

	std::vector<obs_key_combination_t> combos;
	bool showHotkey;
	ui->hotkeyScrollArea->ensureVisible(0, 0);

	QLayoutItem *hotkeysItem = ui->hotkeyFormLayout->itemAt(0);
	QWidget *hotkeys = hotkeysItem->widget();
	if (!hotkeys)
		return;

	QFormLayout *hotkeysLayout = qobject_cast<QFormLayout *>(hotkeys->layout());
	hotkeysLayout->setEnabled(false);

	QString needle = text.toLower();

	for (int i = 0; i < hotkeysLayout->rowCount(); i++) {
		auto label = hotkeysLayout->itemAt(i, QFormLayout::LabelRole);
		if (!label)
			continue;

		OBSHotkeyLabel *item = qobject_cast<OBSHotkeyLabel *>(label->widget());
		if (!item)
			continue;

		QString fullname = item->property("fullName").value<QString>();

		showHotkey = needle.isEmpty() || fullname.toLower().contains(needle);

		if (showHotkey && !obs_key_combination_is_empty(filterCombo)) {
			showHotkey = false;

			item->widget->GetCombinations(combos);
			for (auto combo : combos) {
				if (combo == filterCombo) {
					showHotkey = true;
					break;
				}
			}
		}

		label->widget()->setVisible(showHotkey);

		auto field = hotkeysLayout->itemAt(i, QFormLayout::FieldRole);
		if (field)
			field->widget()->setVisible(showHotkey);
	}
	hotkeysLayout->setEnabled(true);
}

void OBSBasicSettings::on_hotkeyFilterReset_clicked()
{
	ui->hotkeyFilterSearch->setText("");
	ui->hotkeyFilterInput->ResetKey();
}

void OBSBasicSettings::on_hotkeyFilterSearch_textChanged(const QString text)
{
	SearchHotkeys(text, ui->hotkeyFilterInput->key);
}

void OBSBasicSettings::on_hotkeyFilterInput_KeyChanged(obs_key_combination_t combo)
{
	SearchHotkeys(ui->hotkeyFilterSearch->text(), combo);
}

namespace std {
template<> struct hash<obs_key_combination_t> {
	size_t operator()(obs_key_combination_t value) const
	{
		size_t h1 = hash<uint32_t>{}(value.modifiers);
		size_t h2 = hash<int>{}(value.key);
		// Same as boost::hash_combine()
		h2 ^= h1 + 0x9e3779b9 + (h2 << 6) + (h2 >> 2);
		return h2;
	}
};
} // namespace std

bool OBSBasicSettings::ScanDuplicateHotkeys(QFormLayout *layout)
{
	typedef struct assignment {
		OBSHotkeyLabel *label;
		OBSHotkeyEdit *edit;
	} assignment;

	unordered_map<obs_key_combination_t, vector<assignment>> assignments;
	vector<OBSHotkeyLabel *> items;
	bool hasDupes = false;

	for (int i = 0; i < layout->rowCount(); i++) {
		auto label = layout->itemAt(i, QFormLayout::LabelRole);
		if (!label)
			continue;
		OBSHotkeyLabel *item = qobject_cast<OBSHotkeyLabel *>(label->widget());
		if (!item)
			continue;

		items.push_back(item);

		for (auto &edit : item->widget->edits) {
			edit->hasDuplicate = false;

			if (obs_key_combination_is_empty(edit->key))
				continue;

			for (assignment &assign : assignments[edit->key]) {
				if (item->pairPartner == assign.label)
					continue;

				assign.edit->hasDuplicate = true;
				edit->hasDuplicate = true;
				hasDupes = true;
			}

			assignments[edit->key].push_back({item, edit});
		}
	}

	for (auto *item : items)
		for (auto &edit : item->widget->edits)
			edit->UpdateDuplicationState();

	return hasDupes;
}

void OBSBasicSettings::ReloadHotkeys(obs_hotkey_id ignoreKey)
{
	if (!hotkeysLoaded)
		return;
	LoadHotkeySettings(ignoreKey);
}

void OBSBasicSettings::A11yChanged()
{
	if (!loading) {
		a11yChanged = true;
		sender()->setProperty("changed", QVariant(true));
		EnableApplyButton(true);
	}
}

void OBSBasicSettings::AppearanceChanged()
{
	if (!loading) {
		appearanceChanged = true;
		sender()->setProperty("changed", QVariant(true));
		EnableApplyButton(true);
	}
}

void OBSBasicSettings::AdvancedChanged()
{
	if (!loading) {
		advancedChanged = true;
		sender()->setProperty("changed", QVariant(true));
		EnableApplyButton(true);
	}
}

void OBSBasicSettings::AdvOutSplitFileChanged()
{
	bool splitFile = ui->advOutSplitFile->isChecked();
	int splitFileType = splitFile ? ui->advOutSplitFileType->currentIndex() : -1;

	ui->advOutSplitFileType->setEnabled(splitFile);
	ui->advOutSplitFileTimeLabel->setVisible(splitFileType == 0);
	ui->advOutSplitFileTime->setVisible(splitFileType == 0);
	ui->advOutSplitFileSizeLabel->setVisible(splitFileType == 1);
	ui->advOutSplitFileSize->setVisible(splitFileType == 1);
}

static void DisableIncompatibleCodecs(QComboBox *cbox, const QString &format, const QString &formatName,
				      const QString &streamEncoder)
{
	QString strEncLabel = QTStr("Basic.Settings.Output.Adv.Recording.UseStreamEncoder");
	QString recEncoder = cbox->currentData().toString();

	/* Check if selected encoders and output format are compatible, disable incompatible items. */
	bool currentCompatible = true;
	for (int idx = 0; idx < cbox->count(); idx++) {
		QString encName = cbox->itemData(idx).toString();
		string encoderId = (encName == "none") ? streamEncoder.toStdString() : encName.toStdString();
		QString encDisplayName = (encName == "none") ? strEncLabel
							     : obs_encoder_get_display_name(encoderId.c_str());

		/* Something has gone horribly wrong and there's no encoder */
		if (encoderId.empty())
			continue;

		if (obs_get_encoder_caps(encoderId.c_str()) & OBS_ENCODER_CAP_DEPRECATED) {
			encDisplayName += " (" + QTStr("Deprecated") + ")";
		}

		const char *codec = obs_get_encoder_codec(encoderId.c_str());

		bool is_compatible = ContainerSupportsCodec(format.toStdString(), codec);
		/* Fall back to FFmpeg check if codec not one of the built-in ones. */
		if (!is_compatible && !IsBuiltinCodec(codec)) {
			string ext = GetFormatExt(QT_TO_UTF8(format));
			is_compatible = FFCodecAndFormatCompatible(codec, ext.c_str());
		}

		QStandardItemModel *model = dynamic_cast<QStandardItemModel *>(cbox->model());
		QStandardItem *item = model->item(idx);

		if (is_compatible) {
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		} else {
			if (recEncoder == encName)
				currentCompatible = false;

			item->setFlags(Qt::NoItemFlags);
			encDisplayName += " ";
			encDisplayName += QTStr("CodecCompat.Incompatible").arg(formatName);
		}

		item->setText(encDisplayName);
	}

	// Set to invalid entry if encoder was incompatible
	if (!currentCompatible)
		cbox->setCurrentIndex(-1);
}

void OBSBasicSettings::AdvOutRecCheckCodecs()
{
	QString recFormat = ui->advOutRecFormat->currentData().toString();
	QString recFormatName = ui->advOutRecFormat->currentText();

	/* Set tooltip if available */
	QString tooltip = QTStr("Basic.Settings.Output.Format.TT." + recFormat.toUtf8());

	if (!tooltip.startsWith("Basic.Settings.Output"))
		ui->advOutRecFormat->setToolTip(tooltip);
	else
		ui->advOutRecFormat->setToolTip(nullptr);

	QString streamEncoder = ui->advOutEncoder->currentData().toString();
	QString streamAudioEncoder = ui->advOutAEncoder->currentData().toString();

	int oldVEncoderIdx = ui->advOutRecEncoder->currentIndex();
	int oldAEncoderIdx = ui->advOutRecAEncoder->currentIndex();
	DisableIncompatibleCodecs(ui->advOutRecEncoder, recFormat, recFormatName, streamEncoder);
	DisableIncompatibleCodecs(ui->advOutRecAEncoder, recFormat, recFormatName, streamAudioEncoder);

	/* Only invoke AdvOutRecCheckWarnings() if it wouldn't already have
	 * been triggered by one of the encoder selections being reset. */
	if (ui->advOutRecEncoder->currentIndex() == oldVEncoderIdx &&
	    ui->advOutRecAEncoder->currentIndex() == oldAEncoderIdx)
		AdvOutRecCheckWarnings();
}

#if defined(__APPLE__) && QT_VERSION < QT_VERSION_CHECK(6, 5, 1)
// Workaround for QTBUG-56064 on macOS
static void ResetInvalidSelection(QComboBox *cbox)
{
	int idx = cbox->currentIndex();
	if (idx < 0)
		return;

	QStandardItemModel *model = dynamic_cast<QStandardItemModel *>(cbox->model());
	QStandardItem *item = model->item(idx);

	if (item->isEnabled())
		return;

	// Reset to "invalid" state if item was disabled
	cbox->blockSignals(true);
	cbox->setCurrentIndex(-1);
	cbox->blockSignals(false);
}
#endif

void OBSBasicSettings::AdvOutRecCheckWarnings()
{
	auto Checked = [](QCheckBox *box) {
		return box->isChecked() ? 1 : 0;
	};

	QString errorMsg;
	QString warningMsg;
	uint32_t tracks = Checked(ui->advOutRecTrack1) + Checked(ui->advOutRecTrack2) + Checked(ui->advOutRecTrack3) +
			  Checked(ui->advOutRecTrack4) + Checked(ui->advOutRecTrack5) + Checked(ui->advOutRecTrack6);

	bool useStreamEncoder = ui->advOutRecEncoder->currentIndex() == 0;
	if (useStreamEncoder) {
		if (!warningMsg.isEmpty())
			warningMsg += "\n\n";
		warningMsg += QTStr("OutputWarnings.CannotPause");
	}

	QString recFormat = ui->advOutRecFormat->currentData().toString();

	if (recFormat == "flv") {
		ui->advRecTrackWidget->setCurrentWidget(ui->flvTracks);
	} else {
		ui->advRecTrackWidget->setCurrentWidget(ui->recTracks);

		if (tracks == 0)
			errorMsg = QTStr("OutputWarnings.NoTracksSelected");
	}

	if (recFormat == "mp4" || recFormat == "mov") {
		if (!warningMsg.isEmpty())
			warningMsg += "\n\n";

		warningMsg += QTStr("OutputWarnings.MP4Recording");
		ui->autoRemux->setText(QTStr("Basic.Settings.Advanced.AutoRemux").arg("mp4") + " " +
				       QTStr("Basic.Settings.Advanced.AutoRemux.MP4"));
	} else {
		ui->autoRemux->setText(QTStr("Basic.Settings.Advanced.AutoRemux").arg("mp4"));
	}

#if defined(__APPLE__) && QT_VERSION < QT_VERSION_CHECK(6, 5, 1)
	// Workaround for QTBUG-56064 on macOS
	ResetInvalidSelection(ui->advOutRecEncoder);
	ResetInvalidSelection(ui->advOutRecAEncoder);
#endif

	// Show warning if codec selection was reset to an invalid state
	if (ui->advOutRecEncoder->currentIndex() == -1 || ui->advOutRecAEncoder->currentIndex() == -1) {
		if (!warningMsg.isEmpty())
			warningMsg += "\n\n";

		warningMsg += QTStr("OutputWarnings.CodecIncompatible");
	}

	delete advOutRecWarning;

	if (!errorMsg.isEmpty() || !warningMsg.isEmpty()) {
		advOutRecWarning = new QLabel(errorMsg.isEmpty() ? warningMsg : errorMsg, this);
		advOutRecWarning->setProperty("class", errorMsg.isEmpty() ? "text-warning" : "text-danger");
		advOutRecWarning->setWordWrap(true);

		ui->advOutRecInfoLayout->addWidget(advOutRecWarning);
	}
}

static inline QString MakeMemorySizeString(int bitrate, int seconds)
{
	QString str = QTStr("Basic.Settings.Advanced.StreamDelay.MemoryUsage");
	int megabytes = bitrate * seconds / 1000 / 8;

	return str.arg(QString::number(megabytes));
}

void OBSBasicSettings::UpdateSimpleOutStreamDelayEstimate()
{
	int seconds = ui->streamDelaySec->value();
	int vBitrate = ui->simpleOutputVBitrate->value();
	int aBitrate = ui->simpleOutputABitrate->currentText().toInt();

	QString msg = MakeMemorySizeString(vBitrate + aBitrate, seconds);

	ui->streamDelayInfo->setText(msg);
}

void OBSBasicSettings::UpdateAdvOutStreamDelayEstimate()
{
	if (!streamEncoderProps)
		return;

	OBSData settings = streamEncoderProps->GetSettings();
	int trackIndex = config_get_int(main->Config(), "AdvOut", "TrackIndex");
	QString aBitrateText;

	switch (trackIndex) {
	case 1:
		aBitrateText = ui->advOutTrack1Bitrate->currentText();
		break;
	case 2:
		aBitrateText = ui->advOutTrack2Bitrate->currentText();
		break;
	case 3:
		aBitrateText = ui->advOutTrack3Bitrate->currentText();
		break;
	case 4:
		aBitrateText = ui->advOutTrack4Bitrate->currentText();
		break;
	case 5:
		aBitrateText = ui->advOutTrack5Bitrate->currentText();
		break;
	case 6:
		aBitrateText = ui->advOutTrack6Bitrate->currentText();
		break;
	}

	int seconds = ui->streamDelaySec->value();
	int vBitrate = (int)obs_data_get_int(settings, "bitrate");
	int aBitrate = aBitrateText.toInt();

	QString msg = MakeMemorySizeString(vBitrate + aBitrate, seconds);

	ui->streamDelayInfo->setText(msg);
}

void OBSBasicSettings::UpdateStreamDelayEstimate()
{
	if (ui->outputMode->currentIndex() == 0)
		UpdateSimpleOutStreamDelayEstimate();
	else
		UpdateAdvOutStreamDelayEstimate();

	UpdateAutomaticReplayBufferCheckboxes();
}

bool EncoderAvailable(const char *encoder)
{
	const char *val;
	int i = 0;

	while (obs_enum_encoder_types(i++, &val))
		if (strcmp(val, encoder) == 0)
			return true;

	return false;
}

void OBSBasicSettings::FillSimpleRecordingValues()
{
#define ADD_QUALITY(str) \
	ui->simpleOutRecQuality->addItem(QTStr("Basic.Settings.Output.Simple.RecordingQuality." str), QString(str));
#define ENCODER_STR(str) QTStr("Basic.Settings.Output.Simple.Encoder." str)

	ADD_QUALITY("Stream");
	ADD_QUALITY("Small");
	ADD_QUALITY("HQ");
	ADD_QUALITY("Lossless");

	ui->simpleOutRecEncoder->addItem(ENCODER_STR("Software"), QString(SIMPLE_ENCODER_X264));
	ui->simpleOutRecEncoder->addItem(ENCODER_STR("SoftwareLowCPU"), QString(SIMPLE_ENCODER_X264_LOWCPU));
	if (EncoderAvailable("obs_qsv11"))
		ui->simpleOutRecEncoder->addItem(ENCODER_STR("Hardware.QSV.H264"), QString(SIMPLE_ENCODER_QSV));
	if (EncoderAvailable("obs_qsv11_av1"))
		ui->simpleOutRecEncoder->addItem(ENCODER_STR("Hardware.QSV.AV1"), QString(SIMPLE_ENCODER_QSV_AV1));
	if (EncoderAvailable("ffmpeg_nvenc"))
		ui->simpleOutRecEncoder->addItem(ENCODER_STR("Hardware.NVENC.H264"), QString(SIMPLE_ENCODER_NVENC));
	if (EncoderAvailable("obs_nvenc_av1_tex"))
		ui->simpleOutRecEncoder->addItem(ENCODER_STR("Hardware.NVENC.AV1"), QString(SIMPLE_ENCODER_NVENC_AV1));
#ifdef ENABLE_HEVC
	if (EncoderAvailable("h265_texture_amf"))
		ui->simpleOutRecEncoder->addItem(ENCODER_STR("Hardware.AMD.HEVC"), QString(SIMPLE_ENCODER_AMD_HEVC));
	if (EncoderAvailable("ffmpeg_hevc_nvenc"))
		ui->simpleOutRecEncoder->addItem(ENCODER_STR("Hardware.NVENC.HEVC"),
						 QString(SIMPLE_ENCODER_NVENC_HEVC));
#endif
	if (EncoderAvailable("h264_texture_amf"))
		ui->simpleOutRecEncoder->addItem(ENCODER_STR("Hardware.AMD.H264"), QString(SIMPLE_ENCODER_AMD));
	if (EncoderAvailable("av1_texture_amf"))
		ui->simpleOutRecEncoder->addItem(ENCODER_STR("Hardware.AMD.AV1"), QString(SIMPLE_ENCODER_AMD_AV1));
	if (EncoderAvailable("com.apple.videotoolbox.videoencoder.ave.avc")
#ifndef __aarch64__
	    && os_get_emulation_status() == true
#endif
	)
		ui->simpleOutRecEncoder->addItem(ENCODER_STR("Hardware.Apple.H264"),
						 QString(SIMPLE_ENCODER_APPLE_H264));
#ifdef ENABLE_HEVC
	if (EncoderAvailable("com.apple.videotoolbox.videoencoder.ave.hevc")
#ifndef __aarch64__
	    && os_get_emulation_status() == true
#endif
	)
		ui->simpleOutRecEncoder->addItem(ENCODER_STR("Hardware.Apple.HEVC"),
						 QString(SIMPLE_ENCODER_APPLE_HEVC));
#endif

	if (EncoderAvailable("CoreAudio_AAC") || EncoderAvailable("libfdk_aac") || EncoderAvailable("ffmpeg_aac"))
		ui->simpleOutRecAEncoder->addItem(QTStr("Basic.Settings.Output.Simple.Codec.AAC.Default"), "aac");
	if (EncoderAvailable("ffmpeg_opus"))
		ui->simpleOutRecAEncoder->addItem(QTStr("Basic.Settings.Output.Simple.Codec.Opus"), "opus");

#undef ADD_QUALITY
#undef ENCODER_STR
}

void OBSBasicSettings::FillAudioMonitoringDevices()
{
	QComboBox *cb = ui->monitoringDevice;

	auto enum_devices = [](void *param, const char *name, const char *id) {
		QComboBox *cb = (QComboBox *)param;
		cb->addItem(name, id);
		return true;
	};

	cb->addItem(QTStr("Basic.Settings.Advanced.Audio.MonitoringDevice"
			  ".Default"),
		    "default");

	obs_enum_audio_monitoring_devices(enum_devices, cb);
}

void OBSBasicSettings::SimpleRecordingQualityChanged()
{
	QString qual = ui->simpleOutRecQuality->currentData().toString();
	bool streamQuality = qual == "Stream";
	bool losslessQuality = !streamQuality && qual == "Lossless";

	bool showEncoder = !streamQuality && !losslessQuality;
	ui->simpleOutRecEncoder->setVisible(showEncoder);
	ui->simpleOutRecEncoderLabel->setVisible(showEncoder);
	ui->simpleOutRecAEncoder->setVisible(showEncoder);
	ui->simpleOutRecAEncoderLabel->setVisible(showEncoder);
	ui->simpleOutRecFormat->setVisible(!losslessQuality);
	ui->simpleOutRecFormatLabel->setVisible(!losslessQuality);

	UpdateMultitrackVideo();
	SimpleRecordingEncoderChanged();
	SimpleReplayBufferChanged();
}

extern const char *get_simple_output_encoder(const char *encoder);

void OBSBasicSettings::SimpleStreamingEncoderChanged()
{
	QString encoder = ui->simpleOutStrEncoder->currentData().toString();
	QString preset;
	const char *defaultPreset = nullptr;

	ui->simpleOutAdvanced->setVisible(true);
	ui->simpleOutPresetLabel->setVisible(true);
	ui->simpleOutPreset->setVisible(true);
	ui->simpleOutPreset->clear();

	if (encoder == SIMPLE_ENCODER_QSV || encoder == SIMPLE_ENCODER_QSV_AV1) {
		ui->simpleOutPreset->addItem("speed", "speed");
		ui->simpleOutPreset->addItem("balanced", "balanced");
		ui->simpleOutPreset->addItem("quality", "quality");

		defaultPreset = "balanced";
		preset = curQSVPreset;

	} else if (encoder == SIMPLE_ENCODER_NVENC || encoder == SIMPLE_ENCODER_NVENC_HEVC ||
		   encoder == SIMPLE_ENCODER_NVENC_AV1) {

		const char *name = get_simple_output_encoder(QT_TO_UTF8(encoder));
		const bool isFFmpegEncoder = strncmp(name, "ffmpeg_", 7) == 0;
		obs_properties_t *props = obs_get_encoder_properties(name);

		obs_property_t *p = obs_properties_get(props, isFFmpegEncoder ? "preset2" : "preset");
		size_t num = obs_property_list_item_count(p);
		for (size_t i = 0; i < num; i++) {
			const char *name = obs_property_list_item_name(p, i);
			const char *val = obs_property_list_item_string(p, i);

			ui->simpleOutPreset->addItem(QT_UTF8(name), val);
		}

		obs_properties_destroy(props);

		defaultPreset = "default";
		preset = curNVENCPreset;

	} else if (encoder == SIMPLE_ENCODER_AMD || encoder == SIMPLE_ENCODER_AMD_HEVC) {
		ui->simpleOutPreset->addItem("Speed", "speed");
		ui->simpleOutPreset->addItem("Balanced", "balanced");
		ui->simpleOutPreset->addItem("Quality", "quality");

		defaultPreset = "balanced";
		preset = curAMDPreset;
	} else if (encoder == SIMPLE_ENCODER_APPLE_H264
#ifdef ENABLE_HEVC
		   || encoder == SIMPLE_ENCODER_APPLE_HEVC
#endif
	) {
		ui->simpleOutAdvanced->setChecked(false);
		ui->simpleOutAdvanced->setVisible(false);
		ui->simpleOutPreset->setVisible(false);
		ui->simpleOutPresetLabel->setVisible(false);

	} else if (encoder == SIMPLE_ENCODER_AMD_AV1) {
		ui->simpleOutPreset->addItem("Speed", "speed");
		ui->simpleOutPreset->addItem("Balanced", "balanced");
		ui->simpleOutPreset->addItem("Quality", "quality");
		ui->simpleOutPreset->addItem("High Quality", "highQuality");

		defaultPreset = "balanced";
		preset = curAMDAV1Preset;
	} else {

#define PRESET_STR(val) QString(Str("Basic.Settings.Output.EncoderPreset." val)).arg(val)
		ui->simpleOutPreset->addItem(PRESET_STR("ultrafast"), "ultrafast");
		ui->simpleOutPreset->addItem("superfast", "superfast");
		ui->simpleOutPreset->addItem(PRESET_STR("veryfast"), "veryfast");
		ui->simpleOutPreset->addItem("faster", "faster");
		ui->simpleOutPreset->addItem(PRESET_STR("fast"), "fast");
#undef PRESET_STR

		/* Users might have previously selected a preset which is no
		 * longer available in simple mode. Make sure we don't mess
		 * with their setups without them knowing. */
		if (ui->simpleOutPreset->findData(curPreset) == -1) {
			ui->simpleOutPreset->addItem(curPreset, curPreset);
			QStandardItemModel *model = qobject_cast<QStandardItemModel *>(ui->simpleOutPreset->model());
			QStandardItem *item = model->item(model->rowCount() - 1);
			item->setEnabled(false);
		}

		defaultPreset = "veryfast";
		preset = curPreset;
	}

	int idx = ui->simpleOutPreset->findData(QVariant(preset));
	if (idx == -1)
		idx = ui->simpleOutPreset->findData(QVariant(defaultPreset));

	ui->simpleOutPreset->setCurrentIndex(idx);
}

#define ESTIMATE_STR "Basic.Settings.Output.ReplayBuffer.Estimate"
#define ESTIMATE_TOO_LARGE_STR "Basic.Settings.Output.ReplayBuffer.EstimateTooLarge"
#define ESTIMATE_UNKNOWN_STR "Basic.Settings.Output.ReplayBuffer.EstimateUnknown"

void OBSBasicSettings::UpdateAutomaticReplayBufferCheckboxes()
{
	bool state = false;
	switch (ui->outputMode->currentIndex()) {
	case 0: {
		const bool lossless = ui->simpleOutRecQuality->currentData().toString() == "Lossless";
		state = ui->simpleReplayBuf->isChecked();
		ui->simpleReplayBuf->setEnabled(!obs_frontend_replay_buffer_active() && !lossless);
		break;
	}
	case 1: {
		state = ui->advReplayBuf->isChecked();
		bool customFFmpeg = ui->advOutRecType->currentIndex() == 1;
		ui->advReplayBuf->setEnabled(!obs_frontend_replay_buffer_active() && !customFFmpeg);
		ui->advReplayBufCustomFFmpeg->setVisible(customFFmpeg);
		break;
	}
	}
	ui->replayWhileStreaming->setEnabled(state);
	ui->keepReplayStreamStops->setEnabled(state && ui->replayWhileStreaming->isChecked());
}

void OBSBasicSettings::SimpleReplayBufferChanged()
{
	QString qual = ui->simpleOutRecQuality->currentData().toString();
	bool streamQuality = qual == "Stream";
	int abitrate = 0;

	ui->simpleRBMegsMax->setVisible(!streamQuality);
	ui->simpleRBMegsMaxLabel->setVisible(!streamQuality);

	if (ui->simpleOutRecFormat->currentText().compare("flv") == 0 || streamQuality) {
		abitrate = ui->simpleOutputABitrate->currentText().toInt();
	} else {
		int delta = ui->simpleOutputABitrate->currentText().toInt();
		if (ui->simpleOutRecTrack1->isChecked())
			abitrate += delta;
		if (ui->simpleOutRecTrack2->isChecked())
			abitrate += delta;
		if (ui->simpleOutRecTrack3->isChecked())
			abitrate += delta;
		if (ui->simpleOutRecTrack4->isChecked())
			abitrate += delta;
		if (ui->simpleOutRecTrack5->isChecked())
			abitrate += delta;
		if (ui->simpleOutRecTrack6->isChecked())
			abitrate += delta;
	}

	int vbitrate = ui->simpleOutputVBitrate->value();
	int seconds = ui->simpleRBSecMax->value();

	// Set maximum to 75% of installed memory
	uint64_t memTotal = os_get_sys_total_size();
	int64_t memMaxMB = memTotal ? memTotal * 3 / 4 / 1024 / 1024 : 8192;

	int64_t memMB = int64_t(seconds) * int64_t(vbitrate + abitrate) * 1000 / 8 / 1024 / 1024;
	if (memMB < 1)
		memMB = 1;

	ui->simpleRBEstimate->setObjectName("");
	if (streamQuality) {
		if (memMB <= memMaxMB) {
			ui->simpleRBEstimate->setText(QTStr(ESTIMATE_STR).arg(QString::number(int(memMB))));
		} else {
			ui->simpleRBEstimate->setText(
				QTStr(ESTIMATE_TOO_LARGE_STR)
					.arg(QString::number(int(memMB)), QString::number(int(memMaxMB))));
			ui->simpleRBEstimate->setProperty("class", "text-warning");
		}
	} else {
		ui->simpleRBEstimate->setText(QTStr(ESTIMATE_UNKNOWN_STR));
		ui->simpleRBMegsMax->setMaximum(memMaxMB);
	}

	ui->simpleRBEstimate->style()->polish(ui->simpleRBEstimate);

	UpdateAutomaticReplayBufferCheckboxes();
}

#define TEXT_USE_STREAM_ENC QTStr("Basic.Settings.Output.Adv.Recording.UseStreamEncoder")

void OBSBasicSettings::AdvReplayBufferChanged()
{
	obs_data_t *settings;
	QString encoder = ui->advOutRecEncoder->currentText();
	bool useStream = QString::compare(encoder, TEXT_USE_STREAM_ENC) == 0;

	if (useStream && streamEncoderProps) {
		settings = streamEncoderProps->GetSettings();
	} else if (!useStream && recordEncoderProps) {
		settings = recordEncoderProps->GetSettings();
	} else {
		if (useStream)
			encoder = GetComboData(ui->advOutEncoder);
		settings = obs_encoder_defaults(encoder.toUtf8().constData());

		if (!settings)
			return;

		const OBSProfile &currentProfile = main->GetCurrentProfile();

		const std::filesystem::path jsonFilePath =
			currentProfile.path / std::filesystem::u8path("recordEncoder.json");

		if (!jsonFilePath.empty()) {
			OBSDataAutoRelease data =
				obs_data_create_from_json_file_safe(jsonFilePath.u8string().c_str(), "bak");
			obs_data_apply(settings, data);
		}
	}

	int vbitrate = (int)obs_data_get_int(settings, "bitrate");
	const char *rateControl = obs_data_get_string(settings, "rate_control");

	if (!rateControl)
		rateControl = "";

	bool lossless = strcmp(rateControl, "lossless") == 0 || ui->advOutRecType->currentIndex() == 1;
	bool replayBufferEnabled = ui->advReplayBuf->isChecked();

	int abitrate = 0;
	if (ui->advOutRecTrack1->isChecked())
		abitrate += ui->advOutTrack1Bitrate->currentText().toInt();
	if (ui->advOutRecTrack2->isChecked())
		abitrate += ui->advOutTrack2Bitrate->currentText().toInt();
	if (ui->advOutRecTrack3->isChecked())
		abitrate += ui->advOutTrack3Bitrate->currentText().toInt();
	if (ui->advOutRecTrack4->isChecked())
		abitrate += ui->advOutTrack4Bitrate->currentText().toInt();
	if (ui->advOutRecTrack5->isChecked())
		abitrate += ui->advOutTrack5Bitrate->currentText().toInt();
	if (ui->advOutRecTrack6->isChecked())
		abitrate += ui->advOutTrack6Bitrate->currentText().toInt();

	int seconds = ui->advRBSecMax->value();

	// Set maximum to 75% of installed memory
	uint64_t memTotal = os_get_sys_total_size();
	int64_t memMaxMB = memTotal ? memTotal * 3 / 4 / 1024 / 1024 : 8192;

	int64_t memMB = int64_t(seconds) * int64_t(vbitrate + abitrate) * 1000 / 8 / 1024 / 1024;
	if (memMB < 1)
		memMB = 1;

	bool varRateControl = (astrcmpi(rateControl, "CBR") == 0 || astrcmpi(rateControl, "VBR") == 0 ||
			       astrcmpi(rateControl, "ABR") == 0);
	if (vbitrate == 0)
		varRateControl = false;

	ui->advRBEstimate->setObjectName("");
	if (varRateControl) {
		ui->advRBMegsMax->setVisible(false);
		ui->advRBMegsMaxLabel->setVisible(false);

		if (memMB <= memMaxMB) {
			ui->advRBEstimate->setText(QTStr(ESTIMATE_STR).arg(QString::number(int(memMB))));
		} else {
			ui->advRBEstimate->setText(
				QTStr(ESTIMATE_TOO_LARGE_STR)
					.arg(QString::number(int(memMB)), QString::number(int(memMaxMB))));
			ui->advRBEstimate->setProperty("class", "text-warning");
		}
	} else {
		ui->advRBMegsMax->setVisible(true);
		ui->advRBMegsMaxLabel->setVisible(true);
		ui->advRBMegsMax->setMaximum(memMaxMB);
		ui->advRBEstimate->setText(QTStr(ESTIMATE_UNKNOWN_STR));
	}

	ui->advReplayBufferFrame->setEnabled(!lossless && replayBufferEnabled);
	ui->advRBEstimate->style()->polish(ui->advRBEstimate);
	ui->advReplayBuf->setEnabled(!lossless);

	UpdateAutomaticReplayBufferCheckboxes();
}

#define SIMPLE_OUTPUT_WARNING(str) QTStr("Basic.Settings.Output.Simple.Warn." str)

static void DisableIncompatibleSimpleCodecs(QComboBox *cbox, const QString &format)
{
	/* Unlike in advanced mode the available simple mode encoders are
	 * hardcoded, so this check is also a simpler, hardcoded one. */
	QString encoder = cbox->currentData().toString();

	bool currentCompatible = true;
	for (int idx = 0; idx < cbox->count(); idx++) {
		QString encName = cbox->itemData(idx).toString();
		QString codec;

		/* Simple mode does not expose audio encoder variants directly,
		 * so we have to simply set the codec to the internal name. */
		if (encName == "opus" || encName == "aac") {
			codec = encName;
		} else {
			const char *encoder_id = get_simple_output_encoder(QT_TO_UTF8(encName));
			codec = obs_get_encoder_codec(encoder_id);
		}

		QStandardItemModel *model = dynamic_cast<QStandardItemModel *>(cbox->model());
		QStandardItem *item = model->item(idx);

		if (ContainerSupportsCodec(format.toStdString(), codec.toStdString())) {
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		} else {
			if (encoder == encName)
				currentCompatible = false;

			item->setFlags(Qt::NoItemFlags);
		}
	}

	if (!currentCompatible)
		cbox->setCurrentIndex(-1);
}

static void DisableIncompatibleSimpleContainer(QComboBox *cbox, const QString &currentFormat, const QString &vEncoder,
					       const QString &aEncoder)
{
	/* Similar to above, but works in reverse to disable incompatible formats
	 * based on the encoder selection. */
	string vCodec = obs_get_encoder_codec(get_simple_output_encoder(QT_TO_UTF8(vEncoder)));
	string aCodec = aEncoder.toStdString();

	bool currentCompatible = true;
	for (int idx = 0; idx < cbox->count(); idx++) {
		QString format = cbox->itemData(idx).toString();
		string formatStr = format.toStdString();

		QStandardItemModel *model = dynamic_cast<QStandardItemModel *>(cbox->model());
		QStandardItem *item = model->item(idx);

		if (ContainerSupportsCodec(formatStr, vCodec) && ContainerSupportsCodec(formatStr, aCodec)) {
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		} else {
			if (format == currentFormat)
				currentCompatible = false;

			item->setFlags(Qt::NoItemFlags);
		}
	}

	if (!currentCompatible)
		cbox->setCurrentIndex(-1);
}

void OBSBasicSettings::SimpleRecordingEncoderChanged()
{
	QString qual = ui->simpleOutRecQuality->currentData().toString();
	QString warning;
	bool enforceBitrate = !ui->ignoreRecommended->isChecked();
	OBSService service = GetStream1Service();

	delete simpleOutRecWarning;

	if (enforceBitrate && service) {
		OBSDataAutoRelease videoSettings = obs_data_create();
		OBSDataAutoRelease audioSettings = obs_data_create();
		int oldVBitrate = ui->simpleOutputVBitrate->value();
		int oldABitrate = ui->simpleOutputABitrate->currentText().toInt();
		obs_data_set_int(videoSettings, "bitrate", oldVBitrate);
		obs_data_set_int(audioSettings, "bitrate", oldABitrate);

		obs_service_apply_encoder_settings(service, videoSettings, audioSettings);

		int newVBitrate = obs_data_get_int(videoSettings, "bitrate");
		int newABitrate = obs_data_get_int(audioSettings, "bitrate");

		if (newVBitrate < oldVBitrate)
			warning = SIMPLE_OUTPUT_WARNING("VideoBitrate").arg(newVBitrate);
		if (newABitrate < oldABitrate) {
			if (!warning.isEmpty())
				warning += "\n\n";
			warning += SIMPLE_OUTPUT_WARNING("AudioBitrate").arg(newABitrate);
		}
	}

	QString format = ui->simpleOutRecFormat->currentData().toString();
	/* Set tooltip if available */
	QString tooltip = QTStr("Basic.Settings.Output.Format.TT." + format.toUtf8());

	if (!tooltip.startsWith("Basic.Settings.Output"))
		ui->simpleOutRecFormat->setToolTip(tooltip);
	else
		ui->simpleOutRecFormat->setToolTip(nullptr);

	if (qual == "Lossless") {
		if (!warning.isEmpty())
			warning += "\n\n";
		warning += SIMPLE_OUTPUT_WARNING("Lossless");
		warning += "\n\n";
		warning += SIMPLE_OUTPUT_WARNING("Encoder");

	} else if (qual != "Stream") {
		QString enc = ui->simpleOutRecEncoder->currentData().toString();
		QString streamEnc = ui->simpleOutStrEncoder->currentData().toString();
		bool x264RecEnc = (enc == SIMPLE_ENCODER_X264 || enc == SIMPLE_ENCODER_X264_LOWCPU);

		if (streamEnc == SIMPLE_ENCODER_X264 && x264RecEnc) {
			if (!warning.isEmpty())
				warning += "\n\n";
			warning += SIMPLE_OUTPUT_WARNING("Encoder");
		}

		/* Prevent function being called recursively if changes happen. */
		ui->simpleOutRecEncoder->blockSignals(true);
		ui->simpleOutRecAEncoder->blockSignals(true);
		DisableIncompatibleSimpleCodecs(ui->simpleOutRecEncoder, format);
		DisableIncompatibleSimpleCodecs(ui->simpleOutRecAEncoder, format);
		ui->simpleOutRecAEncoder->blockSignals(false);
		ui->simpleOutRecEncoder->blockSignals(false);

		if (ui->simpleOutRecEncoder->currentIndex() == -1 || ui->simpleOutRecAEncoder->currentIndex() == -1) {
			if (!warning.isEmpty())
				warning += "\n\n";
			warning += QTStr("OutputWarnings.CodecIncompatible");
		}
	} else {
		/* When using stream encoders do the reverse; Disable containers that are incompatible. */
		QString streamEnc = ui->simpleOutStrEncoder->currentData().toString();
		QString streamAEnc = ui->simpleOutStrAEncoder->currentData().toString();

		ui->simpleOutRecFormat->blockSignals(true);
		DisableIncompatibleSimpleContainer(ui->simpleOutRecFormat, format, streamEnc, streamAEnc);
		ui->simpleOutRecFormat->blockSignals(false);

		if (ui->simpleOutRecFormat->currentIndex() == -1) {
			if (!warning.isEmpty())
				warning += "\n\n";
			warning += SIMPLE_OUTPUT_WARNING("IncompatibleContainer");
		}

		if (!warning.isEmpty())
			warning += "\n\n";
		warning += SIMPLE_OUTPUT_WARNING("CannotPause");
	}

	if (qual != "Lossless" && (format == "mp4" || format == "mov")) {
		if (!warning.isEmpty())
			warning += "\n\n";
		warning += QTStr("OutputWarnings.MP4Recording");
		ui->autoRemux->setText(QTStr("Basic.Settings.Advanced.AutoRemux").arg("mp4") + " " +
				       QTStr("Basic.Settings.Advanced.AutoRemux.MP4"));
	} else {
		ui->autoRemux->setText(QTStr("Basic.Settings.Advanced.AutoRemux").arg("mp4"));
	}

	if (qual == "Stream") {
		ui->simpleRecTrackWidget->setCurrentWidget(ui->simpleFlvTracks);
	} else if (qual == "Lossless") {
		ui->simpleRecTrackWidget->setCurrentWidget(ui->simpleRecTracks);
	} else {
		if (format == "flv") {
			ui->simpleRecTrackWidget->setCurrentWidget(ui->simpleFlvTracks);
		} else {
			ui->simpleRecTrackWidget->setCurrentWidget(ui->simpleRecTracks);
		}
	}

	if (warning.isEmpty())
		return;

	simpleOutRecWarning = new QLabel(warning, this);
	simpleOutRecWarning->setProperty("class", "text-warning");
	simpleOutRecWarning->setWordWrap(true);
	ui->simpleOutInfoLayout->addWidget(simpleOutRecWarning);
}

void OBSBasicSettings::SurroundWarning(int idx)
{
	if (idx == lastChannelSetupIdx || idx == -1)
		return;

	if (loading) {
		lastChannelSetupIdx = idx;
		return;
	}

	QString speakerLayoutQstr = ui->channelSetup->itemText(idx);
	bool surround = IsSurround(QT_TO_UTF8(speakerLayoutQstr));

	QString lastQstr = ui->channelSetup->itemText(lastChannelSetupIdx);
	bool wasSurround = IsSurround(QT_TO_UTF8(lastQstr));

	if (surround && !wasSurround) {
		QMessageBox::StandardButton button;

		QString warningString = QTStr("Basic.Settings.ProgramRestart") + QStringLiteral("\n\n") +
					QTStr(MULTI_CHANNEL_WARNING) + QStringLiteral("\n\n") +
					QTStr(MULTI_CHANNEL_WARNING ".Confirm");

		button = OBSMessageBox::question(this, QTStr(MULTI_CHANNEL_WARNING ".Title"), warningString);

		if (button == QMessageBox::No) {
			QMetaObject::invokeMethod(ui->channelSetup, "setCurrentIndex", Qt::QueuedConnection,
						  Q_ARG(int, lastChannelSetupIdx));
			return;
		}
	}

	lastChannelSetupIdx = idx;
}

#define LL_BUFFERING_WARNING "Basic.Settings.Audio.LowLatencyBufferingWarning"

void OBSBasicSettings::UpdateAudioWarnings()
{
	QString speakerLayoutQstr = ui->channelSetup->currentText();
	bool surround = IsSurround(QT_TO_UTF8(speakerLayoutQstr));
	bool lowBufferingActive = ui->lowLatencyBuffering->isChecked();

	QString text;

	if (surround) {
		text = QTStr(MULTI_CHANNEL_WARNING ".Enabled") + QStringLiteral("\n\n") + QTStr(MULTI_CHANNEL_WARNING);
	}

	if (lowBufferingActive) {
		if (!text.isEmpty())
			text += QStringLiteral("\n\n");

		text += QTStr(LL_BUFFERING_WARNING ".Enabled") + QStringLiteral("\n\n") + QTStr(LL_BUFFERING_WARNING);
	}

	ui->audioMsg_2->setText(text);
	ui->audioMsg_2->setVisible(!text.isEmpty());
}

void OBSBasicSettings::LowLatencyBufferingChanged(bool checked)
{
	if (checked) {
		QString warningStr =
			QTStr(LL_BUFFERING_WARNING) + QStringLiteral("\n\n") + QTStr(LL_BUFFERING_WARNING ".Confirm");

		auto button = OBSMessageBox::question(this, QTStr(LL_BUFFERING_WARNING ".Title"), warningStr);

		if (button == QMessageBox::No) {
			QMetaObject::invokeMethod(ui->lowLatencyBuffering, "setChecked", Qt::QueuedConnection,
						  Q_ARG(bool, false));
			return;
		}
	}

	QMetaObject::invokeMethod(this, "UpdateAudioWarnings", Qt::QueuedConnection);
	QMetaObject::invokeMethod(this, "AudioChangedRestart");
}

void OBSBasicSettings::SimpleRecordingQualityLosslessWarning(int idx)
{
	if (idx == lastSimpleRecQualityIdx || idx == -1)
		return;

	QString qual = ui->simpleOutRecQuality->itemData(idx).toString();

	if (loading) {
		lastSimpleRecQualityIdx = idx;
		return;
	}

	if (qual == "Lossless") {
		QMessageBox::StandardButton button;

		QString warningString = SIMPLE_OUTPUT_WARNING("Lossless") + QStringLiteral("\n\n") +
					SIMPLE_OUTPUT_WARNING("Lossless.Msg");

		button = OBSMessageBox::question(this, SIMPLE_OUTPUT_WARNING("Lossless.Title"), warningString);

		if (button == QMessageBox::No) {
			QMetaObject::invokeMethod(ui->simpleOutRecQuality, "setCurrentIndex", Qt::QueuedConnection,
						  Q_ARG(int, lastSimpleRecQualityIdx));
			return;
		}
	}

	lastSimpleRecQualityIdx = idx;
}

void OBSBasicSettings::on_disableOSXVSync_clicked()
{
#ifdef __APPLE__
	if (!loading) {
		bool disable = ui->disableOSXVSync->isChecked();
		ui->resetOSXVSync->setEnabled(disable);
	}
#endif
}

QIcon OBSBasicSettings::GetGeneralIcon() const
{
	return generalIcon;
}

QIcon OBSBasicSettings::GetAppearanceIcon() const
{
	return appearanceIcon;
}

QIcon OBSBasicSettings::GetStreamIcon() const
{
	return streamIcon;
}

QIcon OBSBasicSettings::GetOutputIcon() const
{
	return outputIcon;
}

QIcon OBSBasicSettings::GetAudioIcon() const
{
	return audioIcon;
}

QIcon OBSBasicSettings::GetVideoIcon() const
{
	return videoIcon;
}

QIcon OBSBasicSettings::GetHotkeysIcon() const
{
	return hotkeysIcon;
}

QIcon OBSBasicSettings::GetAccessibilityIcon() const
{
	return accessibilityIcon;
}

QIcon OBSBasicSettings::GetAdvancedIcon() const
{
	return advancedIcon;
}

void OBSBasicSettings::SetGeneralIcon(const QIcon &icon)
{
	ui->listWidget->item(Pages::GENERAL)->setIcon(icon);
}

void OBSBasicSettings::SetAppearanceIcon(const QIcon &icon)
{
	ui->listWidget->item(Pages::APPEARANCE)->setIcon(icon);
}

void OBSBasicSettings::SetStreamIcon(const QIcon &icon)
{
	ui->listWidget->item(Pages::STREAM)->setIcon(icon);
}

void OBSBasicSettings::SetOutputIcon(const QIcon &icon)
{
	ui->listWidget->item(Pages::OUTPUT)->setIcon(icon);
}

void OBSBasicSettings::SetAudioIcon(const QIcon &icon)
{
	ui->listWidget->item(Pages::AUDIO)->setIcon(icon);
}

void OBSBasicSettings::SetVideoIcon(const QIcon &icon)
{
	ui->listWidget->item(Pages::VIDEO)->setIcon(icon);
}

void OBSBasicSettings::SetHotkeysIcon(const QIcon &icon)
{
	ui->listWidget->item(Pages::HOTKEYS)->setIcon(icon);
}

void OBSBasicSettings::SetAccessibilityIcon(const QIcon &icon)
{
	ui->listWidget->item(Pages::ACCESSIBILITY)->setIcon(icon);
}

void OBSBasicSettings::SetAdvancedIcon(const QIcon &icon)
{
	ui->listWidget->item(Pages::ADVANCED)->setIcon(icon);
}

int OBSBasicSettings::CurrentFLVTrack()
{
	if (ui->flvTrack1->isChecked())
		return 1;
	else if (ui->flvTrack2->isChecked())
		return 2;
	else if (ui->flvTrack3->isChecked())
		return 3;
	else if (ui->flvTrack4->isChecked())
		return 4;
	else if (ui->flvTrack5->isChecked())
		return 5;
	else if (ui->flvTrack6->isChecked())
		return 6;

	return 0;
}

int OBSBasicSettings::SimpleOutGetSelectedAudioTracks()
{
	int tracks = (ui->simpleOutRecTrack1->isChecked() ? (1 << 0) : 0) |
		     (ui->simpleOutRecTrack2->isChecked() ? (1 << 1) : 0) |
		     (ui->simpleOutRecTrack3->isChecked() ? (1 << 2) : 0) |
		     (ui->simpleOutRecTrack4->isChecked() ? (1 << 3) : 0) |
		     (ui->simpleOutRecTrack5->isChecked() ? (1 << 4) : 0) |
		     (ui->simpleOutRecTrack6->isChecked() ? (1 << 5) : 0);
	return tracks;
}

int OBSBasicSettings::AdvOutGetSelectedAudioTracks()
{
	int tracks =
		(ui->advOutRecTrack1->isChecked() ? (1 << 0) : 0) | (ui->advOutRecTrack2->isChecked() ? (1 << 1) : 0) |
		(ui->advOutRecTrack3->isChecked() ? (1 << 2) : 0) | (ui->advOutRecTrack4->isChecked() ? (1 << 3) : 0) |
		(ui->advOutRecTrack5->isChecked() ? (1 << 4) : 0) | (ui->advOutRecTrack6->isChecked() ? (1 << 5) : 0);
	return tracks;
}

int OBSBasicSettings::AdvOutGetStreamingSelectedAudioTracks()
{
	int tracks = (ui->advOutMultiTrack1->isChecked() ? (1 << 0) : 0) |
		     (ui->advOutMultiTrack2->isChecked() ? (1 << 1) : 0) |
		     (ui->advOutMultiTrack3->isChecked() ? (1 << 2) : 0) |
		     (ui->advOutMultiTrack4->isChecked() ? (1 << 3) : 0) |
		     (ui->advOutMultiTrack5->isChecked() ? (1 << 4) : 0) |
		     (ui->advOutMultiTrack6->isChecked() ? (1 << 5) : 0);
	return tracks;
}

/* Using setEditable(true) on a QComboBox when there's a custom style in use
 * does not work properly, so instead completely recreate the widget, which
 * seems to work fine. */
void OBSBasicSettings::RecreateOutputResolutionWidget()
{
	QSizePolicy sizePolicy = ui->outputResolution->sizePolicy();
	bool changed = WidgetChanged(ui->outputResolution);

	delete ui->outputResolution;
	ui->outputResolution = new QComboBox(ui->videoPage);
	ui->outputResolution->setObjectName(QString::fromUtf8("outputResolution"));
	ui->outputResolution->setSizePolicy(sizePolicy);
	ui->outputResolution->setEditable(true);
	ui->outputResolution->setProperty("changed", changed);
	ui->outputResLabel->setBuddy(ui->outputResolution);

	ui->outputResLayout->insertWidget(0, ui->outputResolution);

	QWidget::setTabOrder(ui->baseResolution, ui->outputResolution);
	QWidget::setTabOrder(ui->outputResolution, ui->downscaleFilter);

	HookWidget(ui->outputResolution, CBEDIT_CHANGED, VIDEO_RES);

	connect(ui->outputResolution, &QComboBox::editTextChanged, this,
		&OBSBasicSettings::on_outputResolution_editTextChanged);

	ui->outputResolution->lineEdit()->setValidator(ui->baseResolution->lineEdit()->validator());
}

void OBSBasicSettings::UpdateAdvNetworkGroup()
{
	bool enabled = protocol.contains("RTMP");

	ui->advNetworkDisabled->setVisible(!enabled);

	ui->bindToIPLabel->setVisible(enabled);
	ui->bindToIP->setVisible(enabled);
	ui->dynBitrate->setVisible(enabled);
	ui->ipFamilyLabel->setVisible(enabled);
	ui->ipFamily->setVisible(enabled);
#ifdef _WIN32
	ui->enableNewSocketLoop->setVisible(enabled);
	ui->enableLowLatencyMode->setVisible(enabled);
#endif
}

extern bool MultitrackVideoDeveloperModeEnabled();

void OBSBasicSettings::UpdateMultitrackVideo()
{
	// Technically, it should currently be safe to toggle multitrackVideo
	// while not streaming (recording should be irrelevant), but practically
	// output settings aren't currently being tracked with that degree of
	// flexibility, so just disable everything while outputs are active.
	auto toggle_available = !main->Active();

	// FIXME: protocol is not updated properly for WHIP; what do?
	auto available = protocol.startsWith("RTMP");

	if (available && !IsCustomService()) {
		OBSDataAutoRelease settings = obs_data_create();
		obs_data_set_string(settings, "service", QT_TO_UTF8(ui->service->currentText()));
		OBSServiceAutoRelease temp_service =
			obs_service_create_private("rtmp_common", "auto config query service", settings);
		settings = obs_service_get_settings(temp_service);
		available = obs_data_has_user_value(settings, "multitrack_video_configuration_url");
		if (!available && ui->enableMultitrackVideo->isChecked())
			ui->enableMultitrackVideo->setChecked(false);
	}

#ifndef _WIN32
	available = available && MultitrackVideoDeveloperModeEnabled();
#endif

	if (IsCustomService())
		available = available && MultitrackVideoDeveloperModeEnabled();

	ui->multitrackVideoGroupBox->setVisible(available);

	ui->enableMultitrackVideo->setEnabled(toggle_available);

	ui->multitrackVideoMaximumAggregateBitrateLabel->setEnabled(toggle_available &&
								    ui->enableMultitrackVideo->isChecked());
	ui->multitrackVideoMaximumAggregateBitrateAuto->setEnabled(toggle_available &&
								   ui->enableMultitrackVideo->isChecked());
	ui->multitrackVideoMaximumAggregateBitrate->setEnabled(
		toggle_available && ui->enableMultitrackVideo->isChecked() &&
		!ui->multitrackVideoMaximumAggregateBitrateAuto->isChecked());

	ui->multitrackVideoMaximumVideoTracksLabel->setEnabled(toggle_available &&
							       ui->enableMultitrackVideo->isChecked());
	ui->multitrackVideoMaximumVideoTracksAuto->setEnabled(toggle_available &&
							      ui->enableMultitrackVideo->isChecked());
	ui->multitrackVideoMaximumVideoTracks->setEnabled(toggle_available && ui->enableMultitrackVideo->isChecked() &&
							  !ui->multitrackVideoMaximumVideoTracksAuto->isChecked());

	ui->multitrackVideoStreamDumpEnable->setVisible(available && MultitrackVideoDeveloperModeEnabled());
	ui->multitrackVideoConfigOverrideEnable->setVisible(available && MultitrackVideoDeveloperModeEnabled());
	ui->multitrackVideoConfigOverrideLabel->setVisible(available && MultitrackVideoDeveloperModeEnabled());
	ui->multitrackVideoConfigOverride->setVisible(available && MultitrackVideoDeveloperModeEnabled());

	ui->multitrackVideoStreamDumpEnable->setEnabled(toggle_available && ui->enableMultitrackVideo->isChecked());
	ui->multitrackVideoConfigOverrideEnable->setEnabled(toggle_available && ui->enableMultitrackVideo->isChecked());
	ui->multitrackVideoConfigOverrideLabel->setEnabled(toggle_available && ui->enableMultitrackVideo->isChecked() &&
							   ui->multitrackVideoConfigOverrideEnable->isChecked());
	ui->multitrackVideoConfigOverride->setEnabled(toggle_available && ui->enableMultitrackVideo->isChecked() &&
						      ui->multitrackVideoConfigOverrideEnable->isChecked());

	auto update_simple_output_settings = [&](bool mtv_enabled) {
		auto recording_uses_stream_encoder = ui->simpleOutRecQuality->currentData().toString() == "Stream";
		mtv_enabled = mtv_enabled && !recording_uses_stream_encoder;

		ui->simpleOutputVBitrateLabel->setDisabled(mtv_enabled);
		ui->simpleOutputVBitrate->setDisabled(mtv_enabled);

		ui->simpleOutputABitrateLabel->setDisabled(mtv_enabled);
		ui->simpleOutputABitrate->setDisabled(mtv_enabled);

		ui->simpleOutStrEncoderLabel->setDisabled(mtv_enabled);
		ui->simpleOutStrEncoder->setDisabled(mtv_enabled);

		ui->simpleOutPresetLabel->setDisabled(mtv_enabled);
		ui->simpleOutPreset->setDisabled(mtv_enabled);

		ui->simpleOutCustomLabel->setDisabled(mtv_enabled);
		ui->simpleOutCustom->setDisabled(mtv_enabled);

		ui->simpleOutStrAEncoderLabel->setDisabled(mtv_enabled);
		ui->simpleOutStrAEncoder->setDisabled(mtv_enabled);
	};

	auto update_advanced_output_settings = [&](bool mtv_enabled) {
		auto recording_uses_stream_video_encoder = ui->advOutRecEncoder->currentText() == TEXT_USE_STREAM_ENC;
		auto recording_uses_stream_audio_encoder = ui->advOutRecAEncoder->currentData() == "none";
		auto disable_video = mtv_enabled && !recording_uses_stream_video_encoder;
		auto disable_audio = mtv_enabled && !recording_uses_stream_audio_encoder;

		ui->advOutAEncLabel->setDisabled(disable_audio);
		ui->advOutAEncoder->setDisabled(disable_audio);

		ui->advOutEncLabel->setDisabled(disable_video);
		ui->advOutEncoder->setDisabled(disable_video);

		ui->advOutUseRescale->setDisabled(disable_video);
		ui->advOutRescale->setDisabled(disable_video);
		ui->advOutRescaleFilter->setDisabled(disable_video);

		if (streamEncoderProps)
			streamEncoderProps->SetDisabled(disable_video);
	};

	auto update_advanced_output_audio_tracks = [&](bool mtv_enabled) {
		auto vod_track_enabled = vodTrackCheckbox && vodTrackCheckbox->isChecked();

		auto vod_track_idx_enabled = [&](size_t idx) {
			return vod_track_enabled && vodTrack[idx - 1] && vodTrack[idx - 1]->isChecked();
		};

		auto track1_warning_visible = mtv_enabled &&
					      (ui->advOutTrack1->isChecked() || vod_track_idx_enabled(1));
		auto track1_disabled = track1_warning_visible && !ui->advOutRecTrack1->isChecked();
		ui->advOutTrack1BitrateLabel->setDisabled(track1_disabled);
		ui->advOutTrack1Bitrate->setDisabled(track1_disabled);

		auto track2_warning_visible = mtv_enabled &&
					      (ui->advOutTrack2->isChecked() || vod_track_idx_enabled(2));
		auto track2_disabled = track2_warning_visible && !ui->advOutRecTrack2->isChecked();
		ui->advOutTrack2BitrateLabel->setDisabled(track2_disabled);
		ui->advOutTrack2Bitrate->setDisabled(track2_disabled);

		auto track3_warning_visible = mtv_enabled &&
					      (ui->advOutTrack3->isChecked() || vod_track_idx_enabled(3));
		auto track3_disabled = track3_warning_visible && !ui->advOutRecTrack3->isChecked();
		ui->advOutTrack3BitrateLabel->setDisabled(track3_disabled);
		ui->advOutTrack3Bitrate->setDisabled(track3_disabled);

		auto track4_warning_visible = mtv_enabled &&
					      (ui->advOutTrack4->isChecked() || vod_track_idx_enabled(4));
		auto track4_disabled = track4_warning_visible && !ui->advOutRecTrack4->isChecked();
		ui->advOutTrack4BitrateLabel->setDisabled(track4_disabled);
		ui->advOutTrack4Bitrate->setDisabled(track4_disabled);

		auto track5_warning_visible = mtv_enabled &&
					      (ui->advOutTrack5->isChecked() || vod_track_idx_enabled(5));
		auto track5_disabled = track5_warning_visible && !ui->advOutRecTrack5->isChecked();
		ui->advOutTrack5BitrateLabel->setDisabled(track5_disabled);
		ui->advOutTrack5Bitrate->setDisabled(track5_disabled);

		auto track6_warning_visible = mtv_enabled &&
					      (ui->advOutTrack6->isChecked() || vod_track_idx_enabled(6));
		auto track6_disabled = track6_warning_visible && !ui->advOutRecTrack6->isChecked();
		ui->advOutTrack6BitrateLabel->setDisabled(track6_disabled);
		ui->advOutTrack6Bitrate->setDisabled(track6_disabled);
	};

	if (available) {
		OBSDataAutoRelease settings;
		{
			auto service_name = ui->service->currentText();
			auto custom_server = ui->customServer->text().trimmed();

			obs_properties_t *props = obs_get_service_properties("rtmp_common");
			obs_property_t *service = obs_properties_get(props, "service");

			settings = obs_data_create();

			obs_data_set_string(settings, "service", QT_TO_UTF8(service_name));
			obs_property_modified(service, settings);

			obs_properties_destroy(props);
		}

		auto multitrack_video_name = QTStr("Basic.Settings.Stream.MultitrackVideoLabel");
		if (obs_data_has_user_value(settings, "multitrack_video_name"))
			multitrack_video_name = obs_data_get_string(settings, "multitrack_video_name");

		ui->enableMultitrackVideo->setText(
			QTStr("Basic.Settings.Stream.EnableMultitrackVideo").arg(multitrack_video_name));

		if (obs_data_has_user_value(settings, "multitrack_video_disclaimer")) {
			ui->multitrackVideoInfo->setVisible(true);
			ui->multitrackVideoInfo->setText(obs_data_get_string(settings, "multitrack_video_disclaimer"));
		} else {
			ui->multitrackVideoInfo->setText(
				QTStr("MultitrackVideo.Info").arg(multitrack_video_name, ui->service->currentText()));
		}

		auto disabled_text = QTStr("Basic.Settings.MultitrackVideoDisabledSettings")
					     .arg(ui->service->currentText())
					     .arg(multitrack_video_name);

		ui->multitrackVideoNotice->setText(disabled_text);

		auto mtv_enabled = ui->enableMultitrackVideo->isChecked();
		ui->multitrackVideoNoticeBox->setVisible(mtv_enabled);

		update_simple_output_settings(mtv_enabled);
		update_advanced_output_settings(mtv_enabled);
		update_advanced_output_audio_tracks(mtv_enabled);
	} else {
		ui->multitrackVideoNoticeBox->setVisible(false);

		update_simple_output_settings(false);
		update_advanced_output_settings(false);
		update_advanced_output_audio_tracks(false);
	}
}

void OBSBasicSettings::SimpleStreamAudioEncoderChanged()
{
	PopulateSimpleBitrates(ui->simpleOutputABitrate, ui->simpleOutStrAEncoder->currentData().toString() == "opus");

	if (IsSurround(QT_TO_UTF8(ui->channelSetup->currentText())))
		return;

	RestrictResetBitrates({ui->simpleOutputABitrate}, 320);
}

void OBSBasicSettings::AdvAudioEncodersChanged()
{
	QString streamEncoder = ui->advOutAEncoder->currentData().toString();
	QString recEncoder = ui->advOutRecAEncoder->currentData().toString();

	if (recEncoder == "none")
		recEncoder = streamEncoder;

	PopulateAdvancedBitrates({ui->advOutTrack1Bitrate, ui->advOutTrack2Bitrate, ui->advOutTrack3Bitrate,
				  ui->advOutTrack4Bitrate, ui->advOutTrack5Bitrate, ui->advOutTrack6Bitrate},
				 QT_TO_UTF8(streamEncoder), QT_TO_UTF8(recEncoder));

	if (IsSurround(QT_TO_UTF8(ui->channelSetup->currentText())))
		return;

	RestrictResetBitrates({ui->advOutTrack1Bitrate, ui->advOutTrack2Bitrate, ui->advOutTrack3Bitrate,
			       ui->advOutTrack4Bitrate, ui->advOutTrack5Bitrate, ui->advOutTrack6Bitrate},
			      320);
}
