/*
 * Carla plugin host, adjusted for OBS
 * Copyright (C) 2011-2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CarlaBackendUtils.hpp>
#include <CarlaUtils.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QPointer>

#include "pluginlistdialog.hpp"
#include "pluginrefreshdialog.hpp"

#include "common.h"
#include "qtutils.h"

CARLA_BACKEND_USE_NAMESPACE

/* check if the plugin IO makes sense for OBS */

template<class T> static bool isSupportedIO(const T &info)
{
	return info.cvIns == 0 && info.cvOuts == 0 &&
	       info.audioIns <= MAX_AV_PLANES &&
	       info.audioOuts <= MAX_AV_PLANES;
}

/* getenv with a fallback value if unset */

static inline const char *getEnvWithFallback(const char *const env,
					     const char *const fallback)
{
	if (const char *const value = std::getenv(env))
		return value;

	return fallback;
}

/* Plugin paths (from env vars first, then default locations) */

struct PluginPaths {
	QUtf8String ladspa;
	QUtf8String lv2;
	QUtf8String vst2;
	QUtf8String vst3;
	QUtf8String clap;
	QUtf8String jsfx;

	PluginPaths()
	{
		/* get common env vars first */
		const QString HOME = QDir::toNativeSeparators(QDir::homePath());

#if defined(Q_OS_WINDOWS)
		const char *const envAPPDATA = std::getenv("APPDATA");
		const char *const envLOCALAPPDATA =
			getEnvWithFallback("LOCALAPPDATA", envAPPDATA);
		const char *const envPROGRAMFILES = std::getenv("PROGRAMFILES");
		const char *const envCOMMONPROGRAMFILES =
			std::getenv("COMMONPROGRAMFILES");

		/* small integrity tests */
		if (envAPPDATA == nullptr) {
			qFatal("APPDATA variable not set, cannot continue");
			abort();
		}

		if (envPROGRAMFILES == nullptr) {
			qFatal("PROGRAMFILES variable not set, cannot continue");
			abort();
		}

		if (envCOMMONPROGRAMFILES == nullptr) {
			qFatal("COMMONPROGRAMFILES variable not set, cannot continue");
			abort();
		}

		const QUtf8String APPDATA(envAPPDATA);
		const QUtf8String LOCALAPPDATA(envLOCALAPPDATA);
		const QUtf8String PROGRAMFILES(envPROGRAMFILES);
		const QUtf8String COMMONPROGRAMFILES(envCOMMONPROGRAMFILES);
#elif !defined(Q_OS_DARWIN)
		const QUtf8String CONFIG_HOME(getEnvWithFallback(
			"XDG_CONFIG_HOME", (HOME + "/.config").toUtf8()));
#endif

		/* now we set paths, listing format path spec if available */
		if (const char *const envLADSPA = std::getenv("LADSPA_PATH")) {
			ladspa = envLADSPA;
		} else {
			/* no official spec for LADSPA, use most common paths */
#if defined(Q_OS_WINDOWS)
			ladspa = APPDATA + "\\LADSPA";
			ladspa += ";" + PROGRAMFILES + "\\LADSPA";
#elif defined(Q_OS_DARWIN)
			ladspa = HOME + "/Library/Audio/Plug-Ins/LADSPA";
			ladspa += ":/Library/Audio/Plug-Ins/LADSPA";
#else
			ladspa = HOME + "/.ladspa";
			ladspa += ":/usr/local/lib/ladspa";
			ladspa += ":/usr/lib/ladspa";
#endif
		}

		if (const char *const envLV2 = std::getenv("LV2_PATH")) {
			lv2 = envLV2;
		} else {
			/* use path spec as defined in:
			 * https://lv2plug.in/pages/filesystem-hierarchy-standard.html
			 */
#if defined(Q_OS_WINDOWS)
			lv2 = APPDATA + "\\LV2";
			lv2 += ";" + COMMONPROGRAMFILES + "\\LV2";
#elif defined(Q_OS_DARWIN)
			lv2 = HOME + "/Library/Audio/Plug-Ins/LV2";
			lv2 += ":/Library/Audio/Plug-Ins/LV2";
#else
			lv2 = HOME + "/.lv2";
			lv2 += ":/usr/local/lib/lv2";
			lv2 += ":/usr/lib/lv2";
#endif
		}

		if (const char *const envVST2 = std::getenv("VST_PATH")) {
			vst2 = envVST2;
		} else {
#if defined(Q_OS_WINDOWS)
			/* use path spec as defined in:
			 * https://helpcenter.steinberg.de/hc/en-us/articles/115000177084
			 */
			vst2 = PROGRAMFILES + "\\VSTPlugins";
			vst2 += ";" + PROGRAMFILES + "\\Steinberg\\VSTPlugins";
			vst2 += ";" + COMMONPROGRAMFILES + "\\VST2";
			vst2 += ";" + COMMONPROGRAMFILES + "\\Steinberg\\VST2";
#elif defined(Q_OS_DARWIN)
			/* use path spec as defined in:
			 * https://helpcenter.steinberg.de/hc/en-us/articles/115000171310
			 */
			vst2 = HOME + "/Library/Audio/Plug-Ins/VST";
			vst2 += ":/Library/Audio/Plug-Ins/VST";
#else
			/* no official spec for VST2 on non-win/mac, use most common paths */
			vst2 = HOME + "/.vst";
			vst2 += ":" + HOME + "/.lxvst";
			vst2 += ":/usr/local/lib/vst";
			vst2 += ":/usr/local/lib/lxvst";
			vst2 += ":/usr/lib/vst";
			vst2 += ":/usr/lib/lxvst";
#endif
		}

		if (const char *const envVST3 = std::getenv("VST3_PATH")) {
			vst3 = envVST3;
		} else {
			/* use path spec as defined in:
			 * https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Locations.html
			 */
#if defined(Q_OS_WINDOWS)
			vst3 = LOCALAPPDATA + "\\Programs\\Common\\VST3";
			vst3 += ";" + COMMONPROGRAMFILES + "\\VST3";
#elif defined(Q_OS_DARWIN)
			vst3 = HOME + "/Library/Audio/Plug-Ins/VST3";
			vst3 += ":/Library/Audio/Plug-Ins/VST3";
#else
			vst3 = HOME + "/.vst3";
			vst3 += ":/usr/local/lib/vst3";
			vst3 += ":/usr/lib/vst3";
#endif
		}

		if (const char *const envCLAP = std::getenv("CLAP_PATH")) {
			clap = envCLAP;
		} else {
			/* use path spec as defined in:
			 * https://github.com/free-audio/clap/blob/main/include/clap/entry.h
			 */
#if defined(Q_OS_WINDOWS)
			clap = LOCALAPPDATA + "\\Programs\\Common\\CLAP";
			clap += ";" + COMMONPROGRAMFILES + "\\CLAP";
#elif defined(Q_OS_DARWIN)
			clap = HOME + "/Library/Audio/Plug-Ins/CLAP";
			clap += ":/Library/Audio/Plug-Ins/CLAP";
#else
			clap = HOME + "/.clap";
			clap += ":/usr/local/lib/clap";
			clap += ":/usr/lib/clap";
#endif
		}

		if (const char *const envJSFX = std::getenv("JSFX_PATH")) {
			jsfx = envJSFX;
		} else {
			/* there is no path spec, use REAPER's user data directory */
#if defined(Q_OS_WINDOWS)
			jsfx = APPDATA + "\\REAPER\\Effects";
#elif defined(Q_OS_DARWIN)
			jsfx = HOME +
			       "/Library/Application Support/REAPER/Effects";
#else
			jsfx = CONFIG_HOME + "/REAPER/Effects";
#endif
		}
	}
};

/* Qt-compatible plugin info (convert to and from QVariant) */

/* base details, nicely packed and POD-only so we can directly use as binary */
struct PluginInfoHeader {
	uint16_t build;
	uint16_t type;
	uint32_t hints;
	uint64_t uniqueId;
	uint16_t audioIns;
	uint16_t audioOuts;
	uint16_t cvIns;
	uint16_t cvOuts;
	uint16_t midiIns;
	uint16_t midiOuts;
	uint16_t parameterIns;
	uint16_t parameterOuts;
};

/* full details, now with non-POD types */
struct PluginInfo : PluginInfoHeader {
	QString category;
	QString filename;
	QString name;
	QString label;
	QString maker;
};

/* convert PluginInfo to Qt types */
static QVariant asByteArray(const PluginInfo &info)
{
	QByteArray qdata;

	/* start with the POD data, stored as-is */
	qdata.append(
		static_cast<const char *>(static_cast<const void *>(&info)),
		sizeof(PluginInfoHeader));

	/* then all the strings, with a null terminating byte */
	{
		const QByteArray qcategory(info.category.toUtf8());
		qdata += qcategory.constData();
		qdata += '\0';
	}

	{
		const QByteArray qfilename(info.filename.toUtf8());
		qdata += qfilename.constData();
		qdata += '\0';
	}

	{
		const QByteArray qname(info.name.toUtf8());
		qdata += qname.constData();
		qdata += '\0';
	}

	{
		const QByteArray qlabel(info.label.toUtf8());
		qdata += qlabel.constData();
		qdata += '\0';
	}

	{
		const QByteArray qmaker(info.maker.toUtf8());
		qdata += qmaker.constData();
		qdata += '\0';
	}

	return qdata;
}

static QVariant asVariant(const PluginInfo &info)
{
	return QVariant(asByteArray(info));
}

/* convert Qt types to PluginInfo */
static PluginInfo asPluginInfo(const QByteArray &qdata)
{
	/* make sure data is big enough to fit POD data + 5 strings */
	CARLA_SAFE_ASSERT_RETURN(static_cast<size_t>(qdata.size()) >=
					 sizeof(PluginInfoHeader) +
						 sizeof(char) * 5,
				 {});

	/* read POD data first */
	const PluginInfoHeader *const data =
		static_cast<const PluginInfoHeader *>(
			static_cast<const void *>(qdata.constData()));
	PluginInfo info = {data->build,
			   data->type,
			   data->hints,
			   data->uniqueId,
			   data->audioIns,
			   data->audioOuts,
			   data->cvIns,
			   data->cvOuts,
			   data->midiIns,
			   data->midiOuts,
			   data->parameterIns,
			   data->parameterOuts,
			   {},
			   {},
			   {},
			   {},
			   {}};

	/* then all the strings, keeping the same order as in `asVariant` */
	const char *sdata =
		static_cast<const char *>(static_cast<const void *>(data + 1));

	info.category = QString::fromUtf8(sdata);
	sdata += info.category.size() + 1;

	info.filename = QString::fromUtf8(sdata);
	sdata += info.filename.size() + 1;

	info.name = QString::fromUtf8(sdata);
	sdata += info.name.size() + 1;

	info.label = QString::fromUtf8(sdata);
	sdata += info.label.size() + 1;

	info.maker = QString::fromUtf8(sdata);
	sdata += info.maker.size() + 1;

	return info;
}

static PluginInfo asPluginInfo(const QVariant &var)
{
	return asPluginInfo(var.toByteArray());
}

static QList<PluginInfo> asPluginInfoList(const QVariant &var)
{
	QCompatByteArray qdata(var.toByteArray());

	QList<PluginInfo> plist;

	while (!qdata.isEmpty()) {
		const PluginInfo info = asPluginInfo(qdata);
		CARLA_SAFE_ASSERT_RETURN(info.build != BINARY_NONE, {});

		plist.append(info);
		qdata = qdata.sliced(sizeof(PluginInfoHeader) +
				     info.category.size() +
				     info.filename.size() + info.name.size() +
				     info.label.size() + info.maker.size() + 5);
	}

	return plist;
}

#ifndef CARLA_2_6_FEATURES
/* convert cached plugin stuff to PluginInfo */
static PluginInfo asPluginInfo(const CarlaCachedPluginInfo *const desc,
			       const PluginType ptype)
{
	PluginInfo pinfo = {};
	pinfo.build = BINARY_NATIVE;
	pinfo.type = ptype;
	pinfo.hints = desc->hints;
	pinfo.name = desc->name;
	pinfo.label = desc->label;
	pinfo.maker = desc->maker;
	pinfo.category = getPluginCategoryAsString(desc->category);

	pinfo.audioIns = desc->audioIns;
	pinfo.audioOuts = desc->audioOuts;

	pinfo.cvIns = desc->cvIns;
	pinfo.cvOuts = desc->cvOuts;

	pinfo.midiIns = desc->midiIns;
	pinfo.midiOuts = desc->midiOuts;

	pinfo.parameterIns = desc->parameterIns;
	pinfo.parameterOuts = desc->parameterOuts;

	if (ptype == PLUGIN_LV2) {
		const QString label(desc->label);
		pinfo.filename = label.split(CARLA_OS_SEP).first();
		pinfo.label = label.section(CARLA_OS_SEP, 1);
	}

	return pinfo;
}
#endif

/* Qt-compatible plugin favorite (convert to and from QVariant) */

/* base details, nicely packed and POD-only so we can directly use as binary */
struct PluginFavoriteHeader {
	uint16_t type;
	uint64_t uniqueId;
};

/* full details, now with non-POD types */
struct PluginFavorite : PluginFavoriteHeader {
	QString filename;
	QString label;

	bool operator==(const PluginFavorite &other) const
	{
		return type == other.type && uniqueId == other.uniqueId &&
		       filename == other.filename && label == other.label;
	}
};

/* convert PluginFavorite to Qt types */
static QByteArray asByteArray(const PluginFavorite &fav)
{
	QByteArray qdata;

	/* start with the POD data, stored as-is */
	qdata.append(static_cast<const char *>(static_cast<const void *>(&fav)),
		     sizeof(PluginFavoriteHeader));

	/* then all the strings, with a null terminating byte */
	{
		const QByteArray qfilename(fav.filename.toUtf8());
		qdata += qfilename.constData();
		qdata += '\0';
	}

	{
		const QByteArray qlabel(fav.label.toUtf8());
		qdata += qlabel.constData();
		qdata += '\0';
	}

	return qdata;
}

static QVariant asVariant(const QList<PluginFavorite> &favlist)
{
	QByteArray qdata;

	for (const PluginFavorite &fav : favlist)
		qdata += asByteArray(fav);

	return QVariant(qdata);
}

/* convert Qt types to PluginInfo */
static PluginFavorite asPluginFavorite(const QByteArray &qdata)
{
	/* make sure data is big enough to fit POD data + 3 strings */
	CARLA_SAFE_ASSERT_RETURN(static_cast<size_t>(qdata.size()) >=
					 sizeof(PluginFavoriteHeader) +
						 sizeof(char) * 3,
				 {});

	/* read POD data first */
	const PluginFavoriteHeader *const data =
		static_cast<const PluginFavoriteHeader *>(
			static_cast<const void *>(qdata.constData()));
	PluginFavorite fav = {data->type, data->uniqueId, {}, {}};

	/* then all the strings, keeping the same order as in `asVariant` */
	const char *sdata =
		static_cast<const char *>(static_cast<const void *>(data + 1));

	fav.filename = QString::fromUtf8(sdata);
	sdata += fav.filename.size() + 1;

	fav.label = QString::fromUtf8(sdata);
	sdata += fav.label.size() + 1;

	return fav;
}

static QList<PluginFavorite> asPluginFavoriteList(const QVariant &var)
{
	QCompatByteArray qdata(var.toByteArray());

	QList<PluginFavorite> favlist;

	while (!qdata.isEmpty()) {
		const PluginFavorite fav = asPluginFavorite(qdata);
		CARLA_SAFE_ASSERT_RETURN(fav.type != PLUGIN_NONE, {});

		favlist.append(fav);
		qdata = qdata.sliced(sizeof(PluginFavoriteHeader) +
				     fav.filename.size() + fav.label.size() +
				     2);
	}

	return favlist;
}

/* create PluginFavorite from PluginInfo data */
static PluginFavorite asPluginFavorite(const PluginInfo &info)
{
	return PluginFavorite{info.type, info.uniqueId, info.filename,
			      info.label};
}

#ifdef CARLA_2_6_FEATURES
static void discoveryCallback(void *const ptr,
			      const CarlaPluginDiscoveryInfo *const info,
			      const char *const sha1sum)
{
	static_cast<PluginListDialog *>(ptr)->addPluginInfo(info, sha1sum);
}

static bool checkCacheCallback(void *const ptr, const char *const filename,
			       const char *const sha1sum)
{
	if (sha1sum == nullptr)
		return false;

	return static_cast<PluginListDialog *>(ptr)->checkPluginCache(filename,
								      sha1sum);
}
#endif

struct PluginListDialog::PrivateData {
	int lastTableWidgetIndex = 0;
	int timerId = 0;
	PluginInfo retPlugin;

	struct Discovery {
		PluginType ptype = PLUGIN_NONE;
		bool firstInit = true;
#ifdef CARLA_2_6_FEATURES
		bool ignoreCache = false;
		bool checkInvalid = false;
		CarlaPluginDiscoveryHandle handle = nullptr;
		QUtf8String tool;
		QPointer<PluginRefreshDialog> dialog;
		Discovery()
		{
			tool = get_carla_bin_path();
#ifdef Q_OS_WINDOWS
			tool += "\\carla-discovery-native.exe";
#else
			tool += "/carla-discovery-native";
#endif
		}

		~Discovery()
		{
			if (handle != nullptr)
				carla_plugin_discovery_stop(handle);
		}
#endif
	} discovery;

	PluginPaths paths;

	struct {
		std::vector<PluginInfo> internal;
		std::vector<PluginInfo> lv2;
		std::vector<PluginInfo> jsfx;
#ifdef CARLA_2_6_FEATURES
		std::vector<PluginInfo> ladspa;
		std::vector<PluginInfo> vst2;
		std::vector<PluginInfo> vst3;
		std::vector<PluginInfo> clap;
		QMap<QString, QList<PluginInfo>> cache;
#endif
		QList<PluginFavorite> favorites;

		bool add(const PluginInfo &pinfo)
		{
			switch (pinfo.type) {
			case PLUGIN_INTERNAL:
				internal.push_back(pinfo);
				return true;
			case PLUGIN_LV2:
				lv2.push_back(pinfo);
				return true;
			case PLUGIN_JSFX:
				jsfx.push_back(pinfo);
				return true;
#ifdef CARLA_2_6_FEATURES
			case PLUGIN_LADSPA:
				ladspa.push_back(pinfo);
				return true;
			case PLUGIN_VST2:
				vst2.push_back(pinfo);
				return true;
			case PLUGIN_VST3:
				vst3.push_back(pinfo);
				return true;
			case PLUGIN_CLAP:
				clap.push_back(pinfo);
				return true;
#endif
			default:
				return false;
			}
		}
	} plugins;
};

PluginListDialog::PluginListDialog(QWidget *const parent)
	: QDialog(parent),
	  p(new PrivateData)
{
	ui.setupUi(this);

	/* Set-up GUI */

	ui.b_load->setEnabled(false);

	/* do not resize info frame so much */
	const QLayout *const infoLayout = ui.frame_info->layout();
	const QMargins infoMargins = infoLayout->contentsMargins();
	ui.frame_info->setMinimumWidth(
		infoMargins.left() + infoMargins.right() +
		infoLayout->spacing() * 3 +
		ui.la_id->fontMetrics().horizontalAdvance(
			"Has Custom GUI: 9999999999"));

#ifndef CARLA_2_6_FEATURES
	ui.ch_ladspa->hide();
	ui.ch_vst->hide();
	ui.ch_vst3->hide();
	ui.ch_clap->hide();
#endif

	/* start with no plugin selected */
	checkPlugin(-1);

	/* custom action that listens for Ctrl+F shortcut */
	addAction(ui.act_focus_search);

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
#ifdef Q_OS_DARWIN
	setWindowModality(Qt::WindowModal);
#endif

	/* Load settings */

	loadSettings();

	/* Set-up Icons */

	ui.b_clear_filters->setProperty("themeID", "clearIconSmall");
	ui.b_refresh->setProperty("themeID", "refreshIconSmall");

	/* FIXME get a star/bookmark/favorite icon
	QTableWidgetItem* const hhi = ui.tableWidget->horizontalHeaderItem(TW_FAVORITE);
	hhi->setProperty("themeID", "starIconSmall");
	*/

	/* Set-up connections */

	QObject::connect(this, &QDialog::finished, this,
			 &PluginListDialog::saveSettings);
	QObject::connect(ui.b_load, &QPushButton::clicked, this,
			 &QDialog::accept);
	QObject::connect(ui.b_cancel, &QPushButton::clicked, this,
			 &QDialog::reject);

	QObject::connect(ui.b_refresh, &QPushButton::clicked, this,
			 &PluginListDialog::refreshPlugins);
	QObject::connect(ui.b_clear_filters, &QPushButton::clicked, this,
			 &PluginListDialog::clearFilters);
	QObject::connect(ui.lineEdit, &QLineEdit::textChanged, this,
			 &PluginListDialog::checkFilters);
	QObject::connect(ui.tableWidget, &QTableWidget::currentCellChanged,
			 this, &PluginListDialog::checkPlugin);
	QObject::connect(ui.tableWidget, &QTableWidget::cellClicked, this,
			 &PluginListDialog::cellClicked);
	QObject::connect(ui.tableWidget, &QTableWidget::cellDoubleClicked, this,
			 &PluginListDialog::cellDoubleClicked);

	QObject::connect(ui.ch_internal, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFilters);
	QObject::connect(ui.ch_ladspa, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFilters);
	QObject::connect(ui.ch_lv2, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFilters);
	QObject::connect(ui.ch_vst, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFilters);
	QObject::connect(ui.ch_vst3, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFilters);
	QObject::connect(ui.ch_clap, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFilters);
	QObject::connect(ui.ch_jsfx, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFilters);
	QObject::connect(ui.ch_effects, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFilters);
	QObject::connect(ui.ch_instruments, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFilters);
	QObject::connect(ui.ch_midi, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFilters);
	QObject::connect(ui.ch_other, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFilters);
	QObject::connect(ui.ch_favorites, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFilters);
	QObject::connect(ui.ch_gui, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFilters);
	QObject::connect(ui.ch_stereo, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFilters);
	QObject::connect(ui.ch_cat_all, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFiltersCategoryAll);
	QObject::connect(ui.ch_cat_delay, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFiltersCategorySpecific);
	QObject::connect(ui.ch_cat_distortion, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFiltersCategorySpecific);
	QObject::connect(ui.ch_cat_dynamics, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFiltersCategorySpecific);
	QObject::connect(ui.ch_cat_eq, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFiltersCategorySpecific);
	QObject::connect(ui.ch_cat_filter, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFiltersCategorySpecific);
	QObject::connect(ui.ch_cat_modulator, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFiltersCategorySpecific);
	QObject::connect(ui.ch_cat_synth, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFiltersCategorySpecific);
	QObject::connect(ui.ch_cat_utility, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFiltersCategorySpecific);
	QObject::connect(ui.ch_cat_other, &QCheckBox::clicked, this,
			 &PluginListDialog::checkFiltersCategorySpecific);

	QObject::connect(ui.act_focus_search, &QAction::triggered, this,
			 &PluginListDialog::focusSearchFieldAndSelectAllText);
}

PluginListDialog::~PluginListDialog()
{
	if (p->timerId != 0)
		killTimer(p->timerId);

	delete p;
}

/* public methods */

const PluginInfo &PluginListDialog::getSelectedPluginInfo() const
{
	return p->retPlugin;
}

#ifdef CARLA_2_6_FEATURES
void PluginListDialog::addPluginInfo(const CarlaPluginDiscoveryInfo *const info,
				     const char *const sha1sum)
{
	if (info == nullptr) {
		if (sha1sum != nullptr) {
			QSafeSettings settings;
			settings.setValue(
				QString("PluginCache/%1").arg(sha1sum),
				QByteArray());

			p->plugins.cache[QString(sha1sum)] = {};
		}
		return;
	}

	const PluginInfo pinfo = {
		static_cast<uint16_t>(info->btype),
		static_cast<uint16_t>(info->ptype),
		info->metadata.hints,
		info->uniqueId,
		static_cast<uint16_t>(info->io.audioIns),
		static_cast<uint16_t>(info->io.audioOuts),
		static_cast<uint16_t>(info->io.cvIns),
		static_cast<uint16_t>(info->io.cvOuts),
		static_cast<uint16_t>(info->io.midiIns),
		static_cast<uint16_t>(info->io.midiOuts),
		static_cast<uint16_t>(info->io.parameterIns),
		static_cast<uint16_t>(info->io.parameterOuts),
		getPluginCategoryAsString(info->metadata.category),
		QString::fromUtf8(info->filename),
		QString::fromUtf8(info->metadata.name),
		QString::fromUtf8(info->label),
		QString::fromUtf8(info->metadata.maker),
	};

	if (sha1sum != nullptr) {
		QSafeSettings settings;
		const QString qsha1sum(sha1sum);
		const QString key = QString("PluginCache/%1").arg(sha1sum);

		/* single sha1sum can contain >1 plugin */
		QByteArray qdata;
		if (p->plugins.cache.contains(qsha1sum))
			qdata = settings.valueByteArray(key);
		qdata += asVariant(pinfo).toByteArray();

		settings.setValue(key, qdata);

		p->plugins.cache[qsha1sum].append(pinfo);
	}

	if (isSupportedIO(pinfo))
		p->plugins.add(pinfo);
}

bool PluginListDialog::checkPluginCache(const char *const filename,
					const char *const sha1sum)
{
	/* sha1sum is always valid for this call */
	const QString qsha1sum(sha1sum);

	if (filename != nullptr)
		p->discovery.dialog->progressBar->setFormat(filename);

	if (!p->plugins.cache.contains(qsha1sum))
		return false;

	const QList<PluginInfo> &plist(p->plugins.cache[qsha1sum]);

	if (plist.isEmpty())
		return p->discovery.ignoreCache || !p->discovery.checkInvalid;

	/* if filename does not match, abort (hash collision?) */
	if (filename == nullptr || plist.first().filename != filename) {
		p->plugins.cache.remove(qsha1sum);
		return false;
	}

	for (const PluginInfo &info : plist) {
		if (isSupportedIO(info))
			p->plugins.add(info);
	}

	return true;
}
#endif

/* protected methods */

void PluginListDialog::done(const int r)
{
	if (r == QDialog::Accepted && ui.tableWidget->currentRow() >= 0) {
		p->retPlugin = asPluginInfo(
			ui.tableWidget
				->item(ui.tableWidget->currentRow(), TW_NAME)
				->data(Qt::UserRole + UR_PLUGIN_INFO));
	} else {
		p->retPlugin = {};
	}

	QDialog::done(r);
}

void PluginListDialog::showEvent(QShowEvent *const event)
{
	focusSearchFieldAndSelectAllText();
	QDialog::showEvent(event);

	/* Set up initial discovery */
	if (p->discovery.firstInit) {
		p->discovery.firstInit = false;

#ifdef CARLA_2_6_FEATURES
		p->discovery.dialog = new PluginRefreshDialog(this);
		p->discovery.dialog->b_start->setEnabled(false);
		p->discovery.dialog->b_skip->setEnabled(true);
		p->discovery.dialog->ch_updated->setChecked(true);
		p->discovery.dialog->ch_invalid->setChecked(false);
		p->discovery.dialog->group->setEnabled(false);
		p->discovery.dialog->progressBar->setFormat(
			"Starting initial discovery...");
		p->discovery.dialog->show();

		QObject::connect(p->discovery.dialog->b_skip,
				 &QPushButton::clicked, this,
				 &PluginListDialog::refreshPluginsSkip);
		QObject::connect(p->discovery.dialog, &QDialog::finished, this,
				 &PluginListDialog::refreshPluginsStop);
#endif

		p->timerId = startTimer(0);
	}
}

void PluginListDialog::timerEvent(QTimerEvent *const event)
{
	if (event->timerId() == p->timerId) {
		do {
#ifdef CARLA_2_6_FEATURES
			/* discovery in progress, keep it going */
			if (p->discovery.handle != nullptr) {
				if (!carla_plugin_discovery_idle(
					    p->discovery.handle)) {
					carla_plugin_discovery_stop(
						p->discovery.handle);
					p->discovery.handle = nullptr;
				}
				break;
			}
#endif
			/* start next discovery */
			QUtf8String path;
			switch (p->discovery.ptype) {
			case PLUGIN_NONE:
				ui.label->setText(
					tr("Discovering internal plugins..."));
				p->discovery.ptype = PLUGIN_INTERNAL;
				break;
			case PLUGIN_INTERNAL:
				ui.label->setText(
					tr("Discovering LV2 plugins..."));
				path = p->paths.lv2;
				p->discovery.ptype = PLUGIN_LV2;
				break;
			case PLUGIN_LV2:
				if (p->paths.jsfx.isNotEmpty()) {
					ui.label->setText(tr(
						"Discovering JSFX plugins..."));
					path = p->paths.jsfx;
					p->discovery.ptype = PLUGIN_JSFX;
					break;
				}
				[[fallthrough]];
#ifdef CARLA_2_6_FEATURES
			case PLUGIN_JSFX:
				ui.label->setText(
					tr("Discovering LADSPA plugins..."));
				path = p->paths.ladspa;
				p->discovery.ptype = PLUGIN_LADSPA;
				break;
			case PLUGIN_LADSPA:
				ui.label->setText(
					tr("Discovering VST2 plugins..."));
				path = p->paths.vst2;
				p->discovery.ptype = PLUGIN_VST2;
				break;
			case PLUGIN_VST2:
				ui.label->setText(
					tr("Discovering VST3 plugins..."));
				path = p->paths.vst3;
				p->discovery.ptype = PLUGIN_VST3;
				break;
			case PLUGIN_VST3:
				ui.label->setText(
					tr("Discovering CLAP plugins..."));
				path = p->paths.clap;
				p->discovery.ptype = PLUGIN_CLAP;
				break;
#endif
			default:
				/* discovery complete */
				refreshPluginsStop();
			}

			if (p->timerId == 0)
				break;

#ifdef CARLA_2_6_FEATURES
			p->discovery.handle = carla_plugin_discovery_start(
				p->discovery.tool.toUtf8().constData(),
				BINARY_NATIVE, p->discovery.ptype,
				path.toUtf8().constData(), discoveryCallback,
				checkCacheCallback, this);
#else
			if (const uint count = carla_get_cached_plugin_count(
				    p->discovery.ptype,
				    path.toUtf8().constData())) {
				for (uint i = 0; i < count; ++i) {
					const CarlaCachedPluginInfo *const info =
						carla_get_cached_plugin_info(
							p->discovery.ptype, i);

					if (!info || !info->valid)
						continue;

					/* ignore plugins with non-compatible IO */
					if (isSupportedIO(*info))
						p->plugins.add(asPluginInfo(
							info,
							p->discovery.ptype));
				}
			}
#endif
		} while (false);
	}

	QDialog::timerEvent(event);
}

/* private methods */

void PluginListDialog::addPluginsToTable()
{
	/* sum plugins first, creating all needed rows in advance */

	ui.tableWidget->setSortingEnabled(false);
	ui.tableWidget->clearContents();

#ifdef CARLA_2_6_FEATURES
	ui.tableWidget->setRowCount(
		int(p->plugins.internal.size() + p->plugins.ladspa.size() +
		    p->plugins.lv2.size() + p->plugins.vst2.size() +
		    p->plugins.vst3.size() + p->plugins.clap.size() +
		    p->plugins.jsfx.size()));

	ui.label->setText(
		tr("Have %1 Internal, %2 LADSPA, %3 LV2, %4 VST2, %5 VST3, %6 CLAP and %7 JSFX plugins")
			.arg(QString::number(p->plugins.internal.size()))
			.arg(QString::number(p->plugins.ladspa.size()))
			.arg(QString::number(p->plugins.lv2.size()))
			.arg(QString::number(p->plugins.vst2.size()))
			.arg(QString::number(p->plugins.vst3.size()))
			.arg(QString::number(p->plugins.clap.size()))
			.arg(QString::number(p->plugins.jsfx.size())));
#else
	ui.tableWidget->setRowCount(int(p->plugins.internal.size() +
					p->plugins.lv2.size() +
					p->plugins.jsfx.size()));

	ui.label->setText(
		tr("Have %1 Internal, %2 LV2 and %3 JSFX plugins")
			.arg(QString::number(p->plugins.internal.size()))
			.arg(QString::number(p->plugins.lv2.size()))
			.arg(QString::number(p->plugins.jsfx.size())));
#endif

	/* now add all plugins to the table */

	auto addPluginToTable = [=](const PluginInfo &info) {
		const int index = p->lastTableWidgetIndex++;
		const bool isFav =
			p->plugins.favorites.contains(asPluginFavorite(info));

		QTableWidgetItem *const itemFav = new QTableWidgetItem;
		itemFav->setCheckState(isFav ? Qt::Checked : Qt::Unchecked);
		itemFav->setText(isFav ? " " : "  ");

		const QString pluginText =
			(info.name + info.label + info.maker + info.filename)
				.toLower();
		ui.tableWidget->setItem(index, TW_FAVORITE, itemFav);
		ui.tableWidget->setItem(index, TW_NAME,
					new QTableWidgetItem(info.name));
		ui.tableWidget->setItem(index, TW_LABEL,
					new QTableWidgetItem(info.label));
		ui.tableWidget->setItem(index, TW_MAKER,
					new QTableWidgetItem(info.maker));
		ui.tableWidget->setItem(
			index, TW_BINARY,
			new QTableWidgetItem(
				QFileInfo(info.filename).fileName()));

		QTableWidgetItem *const itemName =
			ui.tableWidget->item(index, TW_NAME);
		itemName->setData(Qt::UserRole + UR_PLUGIN_INFO,
				  asVariant(info));
		itemName->setData(Qt::UserRole + UR_SEARCH_TEXT, pluginText);
	};

	p->lastTableWidgetIndex = 0;

	for (const PluginInfo &plugin : p->plugins.internal)
		addPluginToTable(plugin);

	for (const PluginInfo &plugin : p->plugins.lv2)
		addPluginToTable(plugin);

	for (const PluginInfo &plugin : p->plugins.jsfx)
		addPluginToTable(plugin);

#ifdef CARLA_2_6_FEATURES
	for (const PluginInfo &plugin : p->plugins.ladspa)
		addPluginToTable(plugin);

	for (const PluginInfo &plugin : p->plugins.vst2)
		addPluginToTable(plugin);

	for (const PluginInfo &plugin : p->plugins.vst3)
		addPluginToTable(plugin);

	for (const PluginInfo &plugin : p->plugins.clap)
		addPluginToTable(plugin);
#endif

	CARLA_SAFE_ASSERT_INT2(
		p->lastTableWidgetIndex == ui.tableWidget->rowCount(),
		p->lastTableWidgetIndex, ui.tableWidget->rowCount());

	/* and reenable sorting + filtering */

	ui.tableWidget->setSortingEnabled(true);

	checkFilters();
	checkPlugin(ui.tableWidget->currentRow());
}

void PluginListDialog::loadSettings()
{
	const QSafeSettings settings;

	restoreGeometry(settings.valueByteArray("PluginListDialog/Geometry"));
	ui.ch_effects->setChecked(
		settings.valueBool("PluginListDialog/ShowEffects", true));
	ui.ch_instruments->setChecked(
		settings.valueBool("PluginListDialog/ShowInstruments", true));
	ui.ch_midi->setChecked(
		settings.valueBool("PluginListDialog/ShowMIDI", true));
	ui.ch_other->setChecked(
		settings.valueBool("PluginListDialog/ShowOther", true));
	ui.ch_internal->setChecked(
		settings.valueBool("PluginListDialog/ShowInternal", true));
	ui.ch_ladspa->setChecked(
		settings.valueBool("PluginListDialog/ShowLADSPA", true));
	ui.ch_lv2->setChecked(
		settings.valueBool("PluginListDialog/ShowLV2", true));
	ui.ch_vst->setChecked(
		settings.valueBool("PluginListDialog/ShowVST2", true));
	ui.ch_vst3->setChecked(
		settings.valueBool("PluginListDialog/ShowVST3", true));
	ui.ch_clap->setChecked(
		settings.valueBool("PluginListDialog/ShowCLAP", true));
	ui.ch_jsfx->setChecked(
		settings.valueBool("PluginListDialog/ShowJSFX", true));
	ui.ch_favorites->setChecked(
		settings.valueBool("PluginListDialog/ShowFavorites", false));
	ui.ch_gui->setChecked(
		settings.valueBool("PluginListDialog/ShowHasGUI", false));
	ui.ch_stereo->setChecked(
		settings.valueBool("PluginListDialog/ShowStereoOnly", false));
	ui.lineEdit->setText(
		settings.valueString("PluginListDialog/SearchText", ""));

	const QString categories =
		settings.valueString("PluginListDialog/ShowCategory", "all");
	if (categories == "all" or categories.length() < 2) {
		ui.ch_cat_all->setChecked(true);
		ui.ch_cat_delay->setChecked(false);
		ui.ch_cat_distortion->setChecked(false);
		ui.ch_cat_dynamics->setChecked(false);
		ui.ch_cat_eq->setChecked(false);
		ui.ch_cat_filter->setChecked(false);
		ui.ch_cat_modulator->setChecked(false);
		ui.ch_cat_synth->setChecked(false);
		ui.ch_cat_utility->setChecked(false);
		ui.ch_cat_other->setChecked(false);
	} else {
		ui.ch_cat_all->setChecked(false);
		ui.ch_cat_delay->setChecked(categories.contains(":delay:"));
		ui.ch_cat_distortion->setChecked(
			categories.contains(":distortion:"));
		ui.ch_cat_dynamics->setChecked(
			categories.contains(":dynamics:"));
		ui.ch_cat_eq->setChecked(categories.contains(":eq:"));
		ui.ch_cat_filter->setChecked(categories.contains(":filter:"));
		ui.ch_cat_modulator->setChecked(
			categories.contains(":modulator:"));
		ui.ch_cat_synth->setChecked(categories.contains(":synth:"));
		ui.ch_cat_utility->setChecked(categories.contains(":utility:"));
		ui.ch_cat_other->setChecked(categories.contains(":other:"));
	}

	const QByteArray tableGeometry =
		settings.valueByteArray("PluginListDialog/TableGeometry");
	QHeaderView *const horizontalHeader =
		ui.tableWidget->horizontalHeader();
	if (!tableGeometry.isNull()) {
		horizontalHeader->restoreState(tableGeometry);
	} else {
		ui.tableWidget->setColumnWidth(TW_NAME, 250);
		ui.tableWidget->setColumnWidth(TW_LABEL, 200);
		ui.tableWidget->setColumnWidth(TW_MAKER, 150);
		ui.tableWidget->sortByColumn(TW_NAME, Qt::AscendingOrder);
	}

	horizontalHeader->setSectionResizeMode(TW_FAVORITE, QHeaderView::Fixed);
	ui.tableWidget->setColumnWidth(TW_FAVORITE, 24);
	ui.tableWidget->setSortingEnabled(true);

	p->plugins.favorites = asPluginFavoriteList(
		settings.valueByteArray("PluginListDialog/Favorites"));

#ifdef CARLA_2_6_FEATURES
	/* load entire plugin cache */
	const QStringList keys = settings.allKeys();
	for (const QUtf8String key : keys) {
		if (!key.startsWith("PluginCache/"))
			continue;

		const QByteArray data(settings.valueByteArray(key));

		if (data.isEmpty())
			p->plugins.cache.insert(key.sliced(12), {});
		else
			p->plugins.cache.insert(key.sliced(12),
						asPluginInfoList(data));
	}
#endif
}

/* private slots */

void PluginListDialog::cellClicked(const int row, const int column)
{
	if (column != TW_FAVORITE)
		return;

	const PluginInfo info =
		asPluginInfo(ui.tableWidget->item(row, TW_NAME)
				     ->data(Qt::UserRole + UR_PLUGIN_INFO));
	const PluginFavorite fav = asPluginFavorite(info);
	const bool isFavorite = p->plugins.favorites.contains(fav);

	if (ui.tableWidget->item(row, TW_FAVORITE)->checkState() ==
	    Qt::Checked) {
		if (!isFavorite)
			p->plugins.favorites.append(fav);
	} else if (isFavorite) {
		p->plugins.favorites.removeAll(fav);
	}

	QSafeSettings settings;
	settings.setValue("PluginListDialog/Favorites",
			  asVariant(p->plugins.favorites));
}

void PluginListDialog::cellDoubleClicked(int, const int column)
{
	if (column != TW_FAVORITE)
		done(QDialog::Accepted);
}

void PluginListDialog::focusSearchFieldAndSelectAllText()
{
	ui.lineEdit->setFocus();
	ui.lineEdit->selectAll();
}

void PluginListDialog::checkFilters()
{
	const QUtf8String text = ui.lineEdit->text().toLower();

	const bool hideEffects = !ui.ch_effects->isChecked();
	const bool hideInstruments = !ui.ch_instruments->isChecked();
	const bool hideMidi = !ui.ch_midi->isChecked();
	const bool hideOther = !ui.ch_other->isChecked();

	const bool hideInternal = !ui.ch_internal->isChecked();
	const bool hideLV2 = !ui.ch_lv2->isChecked();
	const bool hideJSFX = !ui.ch_jsfx->isChecked();
#ifdef CARLA_2_6_FEATURES
	const bool hideLadspa = !ui.ch_ladspa->isChecked();
	const bool hideVST2 = !ui.ch_vst->isChecked();
	const bool hideVST3 = !ui.ch_vst3->isChecked();
	const bool hideCLAP = !ui.ch_clap->isChecked();
#endif

	const bool hideNonFavs = ui.ch_favorites->isChecked();
	const bool hideNonGui = ui.ch_gui->isChecked();
	const bool hideNonStereo = ui.ch_stereo->isChecked();

	for (int i = 0, c = ui.tableWidget->rowCount(); i < c; ++i) {
		const PluginInfo info = asPluginInfo(
			ui.tableWidget->item(i, TW_NAME)
				->data(Qt::UserRole + UR_PLUGIN_INFO));
		const QString ptext =
			ui.tableWidget->item(i, TW_NAME)
				->data(Qt::UserRole + UR_SEARCH_TEXT)
				.toString();
		const uint16_t ptype = info.type;
		const uint32_t phints = info.hints;
		const uint16_t aIns = info.audioIns;
		const uint16_t aOuts = info.audioOuts;
		const uint16_t mIns = info.midiIns;
		const uint16_t mOuts = info.midiOuts;
		const QString categ = info.category;
		const bool isSynth = phints & PLUGIN_IS_SYNTH;
		const bool isEffect = aIns > 0 && aOuts > 0 && !isSynth;
		const bool isMidi = aIns == 0 && aOuts == 0 && mIns > 0 &&
				    mOuts > 0;
		const bool isOther = !(isEffect || isSynth || isMidi);
		const bool isStereo = (aIns == 2 && aOuts == 2) ||
				      (isSynth && aOuts == 2);
		const bool hasGui = phints & PLUGIN_HAS_CUSTOM_UI;

		const auto hasText = [text, ptext]() {
			const QStringList textSplit = text.strip().split(' ');
			for (const QString &t : textSplit)
				if (ptext.contains(t))
					return true;
			return false;
		};

		/**/ if (hideEffects && isEffect)
			ui.tableWidget->hideRow(i);
		else if (hideInstruments && isSynth)
			ui.tableWidget->hideRow(i);
		else if (hideMidi && isMidi)
			ui.tableWidget->hideRow(i);
		else if (hideOther && isOther)
			ui.tableWidget->hideRow(i);
		else if (hideInternal && ptype == PLUGIN_INTERNAL)
			ui.tableWidget->hideRow(i);
		else if (hideLV2 && ptype == PLUGIN_LV2)
			ui.tableWidget->hideRow(i);
		else if (hideJSFX && ptype == PLUGIN_JSFX)
			ui.tableWidget->hideRow(i);
#ifdef CARLA_2_6_FEATURES
		else if (hideLadspa && ptype == PLUGIN_LADSPA)
			ui.tableWidget->hideRow(i);
		else if (hideVST2 && ptype == PLUGIN_VST2)
			ui.tableWidget->hideRow(i);
		else if (hideVST3 && ptype == PLUGIN_VST3)
			ui.tableWidget->hideRow(i);
		else if (hideCLAP && ptype == PLUGIN_CLAP)
			ui.tableWidget->hideRow(i);
#endif
		else if (hideNonGui && not hasGui)
			ui.tableWidget->hideRow(i);
		else if (hideNonStereo && not isStereo)
			ui.tableWidget->hideRow(i);
		else if (text.isNotEmpty() && !hasText())
			ui.tableWidget->hideRow(i);
		else if (hideNonFavs &&
			 !p->plugins.favorites.contains(asPluginFavorite(info)))
			ui.tableWidget->hideRow(i);
		else if (ui.ch_cat_all->isChecked() or
			 (ui.ch_cat_delay->isChecked() && categ == "delay") or
			 (ui.ch_cat_distortion->isChecked() &&
			  categ == "distortion") or
			 (ui.ch_cat_dynamics->isChecked() &&
			  categ == "dynamics") or
			 (ui.ch_cat_eq->isChecked() && categ == "eq") or
			 (ui.ch_cat_filter->isChecked() && categ == "filter") or
			 (ui.ch_cat_modulator->isChecked() &&
			  categ == "modulator") or
			 (ui.ch_cat_synth->isChecked() && categ == "synth") or
			 (ui.ch_cat_utility->isChecked() &&
			  categ == "utility") or
			 (ui.ch_cat_other->isChecked() && categ == "other"))
			ui.tableWidget->showRow(i);
		else
			ui.tableWidget->hideRow(i);
	}
}

void PluginListDialog::checkFiltersCategoryAll(const bool clicked)
{
	const bool notClicked = !clicked;
	ui.ch_cat_delay->setChecked(notClicked);
	ui.ch_cat_distortion->setChecked(notClicked);
	ui.ch_cat_dynamics->setChecked(notClicked);
	ui.ch_cat_eq->setChecked(notClicked);
	ui.ch_cat_filter->setChecked(notClicked);
	ui.ch_cat_modulator->setChecked(notClicked);
	ui.ch_cat_synth->setChecked(notClicked);
	ui.ch_cat_utility->setChecked(notClicked);
	ui.ch_cat_other->setChecked(notClicked);
	checkFilters();
}

void PluginListDialog::checkFiltersCategorySpecific(bool clicked)
{
	if (clicked) {
		ui.ch_cat_all->setChecked(false);
	} else if (!(ui.ch_cat_delay->isChecked() ||
		     ui.ch_cat_distortion->isChecked() ||
		     ui.ch_cat_dynamics->isChecked() ||
		     ui.ch_cat_eq->isChecked() ||
		     ui.ch_cat_filter->isChecked() ||
		     ui.ch_cat_modulator->isChecked() ||
		     ui.ch_cat_synth->isChecked() ||
		     ui.ch_cat_utility->isChecked() ||
		     ui.ch_cat_other->isChecked())) {
		ui.ch_cat_all->setChecked(true);
	}
	checkFilters();
}

void PluginListDialog::clearFilters()
{
	auto setCheckedWithoutSignaling = [](auto &w, bool checked) {
		w->blockSignals(true);
		w->setChecked(checked);
		w->blockSignals(false);
	};

	setCheckedWithoutSignaling(ui.ch_internal, true);
	setCheckedWithoutSignaling(ui.ch_ladspa, true);
	setCheckedWithoutSignaling(ui.ch_lv2, true);
	setCheckedWithoutSignaling(ui.ch_vst, true);
	setCheckedWithoutSignaling(ui.ch_vst3, true);
	setCheckedWithoutSignaling(ui.ch_clap, true);
	setCheckedWithoutSignaling(ui.ch_jsfx, true);

	setCheckedWithoutSignaling(ui.ch_instruments, true);
	setCheckedWithoutSignaling(ui.ch_effects, true);
	setCheckedWithoutSignaling(ui.ch_midi, true);
	setCheckedWithoutSignaling(ui.ch_other, true);

	setCheckedWithoutSignaling(ui.ch_favorites, false);
	setCheckedWithoutSignaling(ui.ch_stereo, false);
	setCheckedWithoutSignaling(ui.ch_gui, false);

	setCheckedWithoutSignaling(ui.ch_cat_all, true);
	setCheckedWithoutSignaling(ui.ch_cat_delay, false);
	setCheckedWithoutSignaling(ui.ch_cat_distortion, false);
	setCheckedWithoutSignaling(ui.ch_cat_dynamics, false);
	setCheckedWithoutSignaling(ui.ch_cat_eq, false);
	setCheckedWithoutSignaling(ui.ch_cat_filter, false);
	setCheckedWithoutSignaling(ui.ch_cat_modulator, false);
	setCheckedWithoutSignaling(ui.ch_cat_synth, false);
	setCheckedWithoutSignaling(ui.ch_cat_utility, false);
	setCheckedWithoutSignaling(ui.ch_cat_other, false);

	ui.lineEdit->blockSignals(true);
	ui.lineEdit->clear();
	ui.lineEdit->blockSignals(false);

	checkFilters();
}

void PluginListDialog::checkPlugin(const int row)
{
	if (row >= 0) {
		ui.b_load->setEnabled(true);

		const PluginInfo info = asPluginInfo(
			ui.tableWidget->item(row, TW_NAME)
				->data(Qt::UserRole + UR_PLUGIN_INFO));

		const bool isSynth = info.hints & PLUGIN_IS_SYNTH;
		const bool isEffect = info.audioIns > 0 && info.audioOuts > 0 &&
				      !isSynth;
		const bool isMidi = info.audioIns == 0 && info.audioOuts == 0 &&
				    info.midiIns > 0 && info.midiOuts > 0;

		QString ptype;
		/**/ if (isSynth)
			ptype = "Instrument";
		else if (isEffect)
			ptype = "Effect";
		else if (isMidi)
			ptype = "MIDI Plugin";
		else
			ptype = "Other";

		ui.l_format->setText(getPluginTypeAsString(
			static_cast<PluginType>(info.type)));

		ui.l_type->setText(ptype);
		ui.l_id->setText(QString::number(info.uniqueId));
		ui.l_ains->setText(QString::number(info.audioIns));
		ui.l_aouts->setText(QString::number(info.audioOuts));
		ui.l_mins->setText(QString::number(info.midiIns));
		ui.l_mouts->setText(QString::number(info.midiOuts));
		ui.l_pins->setText(QString::number(info.parameterIns));
		ui.l_pouts->setText(QString::number(info.parameterOuts));
		ui.l_gui->setText(info.hints & PLUGIN_HAS_CUSTOM_UI ? tr("Yes")
								    : tr("No"));
		ui.l_synth->setText(isSynth ? tr("Yes") : tr("No"));
	} else {
		ui.b_load->setEnabled(false);
		ui.l_format->setText("---");
		ui.l_type->setText("---");
		ui.l_id->setText("---");
		ui.l_ains->setText("---");
		ui.l_aouts->setText("---");
		ui.l_mins->setText("---");
		ui.l_mouts->setText("---");
		ui.l_pins->setText("---");
		ui.l_pouts->setText("---");
		ui.l_gui->setText("---");
		ui.l_synth->setText("---");
	}
}

void PluginListDialog::refreshPlugins()
{
	refreshPluginsStop();

#ifdef CARLA_2_6_FEATURES
	p->discovery.dialog = new PluginRefreshDialog(this);
	p->discovery.dialog->show();

	QObject::connect(p->discovery.dialog->b_start, &QPushButton::clicked,
			 this, &PluginListDialog::refreshPluginsStart);
	QObject::connect(p->discovery.dialog->b_skip, &QPushButton::clicked,
			 this, &PluginListDialog::refreshPluginsSkip);
	QObject::connect(p->discovery.dialog, &QDialog::finished, this,
			 &PluginListDialog::refreshPluginsStop);
#else
	refreshPluginsStart();
#endif
}

void PluginListDialog::refreshPluginsStart()
{
	/* remove old plugins */
	p->plugins.internal.clear();
	p->plugins.lv2.clear();
	p->plugins.jsfx.clear();
#ifdef CARLA_2_6_FEATURES
	p->plugins.ladspa.clear();
	p->plugins.vst2.clear();
	p->plugins.vst3.clear();
	p->plugins.clap.clear();
	p->discovery.dialog->b_start->setEnabled(false);
	p->discovery.dialog->b_skip->setEnabled(true);
	p->discovery.ignoreCache = p->discovery.dialog->ch_all->isChecked();
	p->discovery.checkInvalid =
		p->discovery.dialog->ch_invalid->isChecked();
	if (p->discovery.ignoreCache)
		p->plugins.cache.clear();
#endif

	/* start discovery again */
	p->discovery.ptype = PLUGIN_NONE;

	if (p->timerId == 0)
		p->timerId = startTimer(0);
}

void PluginListDialog::refreshPluginsStop()
{
#ifdef CARLA_2_6_FEATURES
	/* stop previous discovery if still running */
	if (p->discovery.handle != nullptr) {
		carla_plugin_discovery_stop(p->discovery.handle);
		p->discovery.handle = nullptr;
	}

	if (p->discovery.dialog) {
		p->discovery.dialog->close();
		p->discovery.dialog = nullptr;
	}
#endif

	if (p->timerId != 0) {
		killTimer(p->timerId);
		p->timerId = 0;
		addPluginsToTable();
	}
}

void PluginListDialog::refreshPluginsSkip()
{
#ifdef CARLA_2_6_FEATURES
	if (p->discovery.handle != nullptr)
		carla_plugin_discovery_skip(p->discovery.handle);
#endif
}

void PluginListDialog::saveSettings()
{
	QSafeSettings settings;
	settings.setValue("PluginListDialog/Geometry", saveGeometry());
	settings.setValue("PluginListDialog/TableGeometry",
			  ui.tableWidget->horizontalHeader()->saveState());
	settings.setValue("PluginListDialog/ShowEffects",
			  ui.ch_effects->isChecked());
	settings.setValue("PluginListDialog/ShowInstruments",
			  ui.ch_instruments->isChecked());
	settings.setValue("PluginListDialog/ShowMIDI", ui.ch_midi->isChecked());
	settings.setValue("PluginListDialog/ShowOther",
			  ui.ch_other->isChecked());
	settings.setValue("PluginListDialog/ShowInternal",
			  ui.ch_internal->isChecked());
	settings.setValue("PluginListDialog/ShowLADSPA",
			  ui.ch_ladspa->isChecked());
	settings.setValue("PluginListDialog/ShowLV2", ui.ch_lv2->isChecked());
	settings.setValue("PluginListDialog/ShowVST2", ui.ch_vst->isChecked());
	settings.setValue("PluginListDialog/ShowVST3", ui.ch_vst3->isChecked());
	settings.setValue("PluginListDialog/ShowCLAP", ui.ch_clap->isChecked());
	settings.setValue("PluginListDialog/ShowJSFX", ui.ch_jsfx->isChecked());
	settings.setValue("PluginListDialog/ShowFavorites",
			  ui.ch_favorites->isChecked());
	settings.setValue("PluginListDialog/ShowHasGUI",
			  ui.ch_gui->isChecked());
	settings.setValue("PluginListDialog/ShowStereoOnly",
			  ui.ch_stereo->isChecked());
	settings.setValue("PluginListDialog/SearchText", ui.lineEdit->text());

	if (ui.ch_cat_all->isChecked()) {
		settings.setValue("PluginListDialog/ShowCategory", "all");
	} else {
		QUtf8String categories;
		if (ui.ch_cat_delay->isChecked())
			categories += ":delay";
		if (ui.ch_cat_distortion->isChecked())
			categories += ":distortion";
		if (ui.ch_cat_dynamics->isChecked())
			categories += ":dynamics";
		if (ui.ch_cat_eq->isChecked())
			categories += ":eq";
		if (ui.ch_cat_filter->isChecked())
			categories += ":filter";
		if (ui.ch_cat_modulator->isChecked())
			categories += ":modulator";
		if (ui.ch_cat_synth->isChecked())
			categories += ":synth";
		if (ui.ch_cat_utility->isChecked())
			categories += ":utility";
		if (ui.ch_cat_other->isChecked())
			categories += ":other";
		if (categories.isNotEmpty())
			categories += ":";
		settings.setValue("PluginListDialog/ShowCategory", categories);
	}

	settings.setValue("PluginListDialog/Favorites",
			  asVariant(p->plugins.favorites));
}

const PluginListDialogResults *carla_exec_plugin_list_dialog()
{
	/* create and keep dialog around, as recreating the dialog means doing
	 * a rescan. Qt will delete it later together with the main window
	 */
	static PluginListDialog *const gui =
		new PluginListDialog(carla_qt_get_main_window());

	if (gui->exec()) {
		static PluginListDialogResults ret;
		static std::string filename;
		static std::string label;

		const PluginInfo &plugin(gui->getSelectedPluginInfo());

		filename = plugin.filename.toUtf8();
		label = plugin.label.toUtf8();

		ret.build = plugin.build;
		ret.type = plugin.type;
		ret.filename = filename.c_str();
		ret.label = label.c_str();
		ret.uniqueId = plugin.uniqueId;

		return &ret;
	}

	return nullptr;
}
