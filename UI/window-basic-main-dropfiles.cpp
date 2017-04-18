#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QMimeData>
#include <string>

#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"

using namespace std;

static const char *textExtensions[] = {
	"txt", "log", nullptr
};

static const char *htmlExtensions[] = {
	"htm", "html", nullptr
};

static const char *imageExtensions[] = {
	"bmp", "tga", "png", "jpg", "jpeg", "gif", nullptr
};

static const char *mediaExtensions[] = {
	"3ga", "669", "a52", "aac", "ac3", "adt", "adts", "aif", "aifc",
	"aiff", "amb", "amr", "aob", "ape", "au", "awb", "caf", "dts",
	"flac", "it", "kar", "m4a", "m4b", "m4p", "m5p", "mid", "mka",
	"mlp", "mod", "mpa", "mp1", "mp2", "mp3", "mpc", "mpga", "mus",
	"oga", "ogg", "oma", "opus", "qcp", "ra", "rmi", "s3m", "sid",
	"spx", "tak", "thd", "tta", "voc", "vqf", "w64", "wav", "wma",
	"wv", "xa", "xm" "3g2", "3gp", "3gp2", "3gpp", "amv", "asf", "avi",
	"bik", "crf", "divx", "drc", "dv", "evo", "f4v", "flv", "gvi",
	"gxf", "iso", "m1v", "m2v", "m2t", "m2ts", "m4v", "mkv", "mov",
	"mp2", "mp2v", "mp4", "mp4v", "mpe", "mpeg", "mpeg1", "mpeg2",
	"mpeg4", "mpg", "mpv2", "mts", "mtv", "mxf", "mxg", "nsv", "nuv",
	"ogg", "ogm", "ogv", "ogx", "ps", "rec", "rm", "rmvb", "rpl", "thp",
	"tod", "ts", "tts", "txd", "vob", "vro", "webm", "wm", "wmv", "wtv",
	nullptr
};

static string GenerateSourceName(const char *base)
{
	string name;
	int inc = 0;

	for (;; inc++) {
		name = base;

		if (inc) {
			name += " (";
			name += to_string(inc+1);
			name += ")";
		}

		obs_source_t *source = obs_get_source_by_name(name.c_str());
		if (!source)
			return name;
	}
}

#if defined(_WIN32)
#define PLUGIN_NAME_TEXT_GDIPLUS    "obs-text"
#endif
#define PLUGIN_NAME_TEXT_FREETYPE2  "text-freetype2"
#define PLUGIN_NAME_IMAGE           "image-source"
#define PLUGIN_NAME_MEDIA           "obs-ffmpeg"
#define PLUGIN_NAME_HTML            "obs-browser"

static bool plugin_exist_initialized = false;

static struct _plugin_info
{
	const char *plugin_name;
	const char *plugin_id;
	bool       is_exist;
	const char *target_param_name;
} plugin_info[] = {
#if defined(_WIN32)
	{ PLUGIN_NAME_TEXT_GDIPLUS,   "text_gdiplus",    false, "text" },
#endif
	{ PLUGIN_NAME_TEXT_FREETYPE2, "text_ft2_source", false, "text" },
	{ PLUGIN_NAME_IMAGE,          "image_source",    false, "file" },
	{ PLUGIN_NAME_MEDIA,          "ffmpeg_source",   false, "local_file" },
	{ PLUGIN_NAME_HTML,           "browser_source",  false, "local_file" },
};
typedef _plugin_info plugin_info_t;

#define PLUGIN_INFO_COUNT (sizeof(plugin_info)/sizeof(plugin_info[0]))

#define INIT_PLUGIN_INFO() \
if (!plugin_exist_initialized) { \
	initialze_plugin_exist(); \
	plugin_exist_initialized = true; \
}

static void initialze_plugin_exist_callback(
		void *param, obs_module_t *module)
{
	const char *module_name = *(const char**)module;
	for (int i = 0; i < PLUGIN_INFO_COUNT; ++i) {
		if (strcmp(plugin_info[i].plugin_name, module_name) == 0) {
			plugin_info[i].is_exist = true;
			break;
		}
	}

	UNUSED_PARAMETER(param);
}

static void initialze_plugin_exist()
{
	obs_enum_modules(initialze_plugin_exist_callback, NULL);
}

static bool get_plugin_info(const char *plugin_name, plugin_info_t **info)
{
	for (int i = 0; i < PLUGIN_INFO_COUNT; ++i) {
		if (strcmp(plugin_info[i].plugin_name, plugin_name) == 0) {
			*info = &plugin_info[i];
			return true;
		}
	}
	return false;
}

static bool is_plugin_exist(const char *plugin_name)
{
	plugin_info_t *info;
	INIT_PLUGIN_INFO();

	return get_plugin_info(plugin_name, &info) && info->is_exist;
}

static bool set_raw_text(const char *data, obs_data_t *settings,
		const char **type)
{
	plugin_info_t *info;
	INIT_PLUGIN_INFO();

#if defined(_WIN32)
	if (get_plugin_info(PLUGIN_NAME_TEXT_GDIPLUS, &info) &&
	    info->is_exist) {
		obs_data_set_string(settings, "text", data);
		*type = info->plugin_id;
		return true;
	}
#endif

	if (get_plugin_info(PLUGIN_NAME_TEXT_FREETYPE2, &info) &&
	    info->is_exist) {
		obs_data_set_string(settings, "text", data);
		*type = info->plugin_id;
		return true;
	}

	return false;
}

static bool set_text(const char *data, obs_data_t *settings, const char **type)
{
	plugin_info_t *info;
	INIT_PLUGIN_INFO();

#if defined(_WIN32)
	if (get_plugin_info(PLUGIN_NAME_TEXT_GDIPLUS, &info) &&
	    info->is_exist) {
		obs_data_set_bool(settings, "read_from_file", true);
		obs_data_set_string(settings, "file", data);
		*type = info->plugin_id;
		return true;
	}
#endif

	if (get_plugin_info(PLUGIN_NAME_TEXT_FREETYPE2, &info) &&
	    info->is_exist) {
		obs_data_set_bool(settings, "from_file", true);
		obs_data_set_string(settings, "text_file", data);
		*type = info->plugin_id;
		return true;
	}

	return false;
}

static bool set_data(const char *data, obs_data_t *settings,
		const char **type, const char *plugin_name)
{
	plugin_info_t *info;
	INIT_PLUGIN_INFO();

	if (get_plugin_info(plugin_name, &info) && info->is_exist) {
		obs_data_set_string(settings, info->target_param_name, data);
		*type = info->plugin_id;
		return true;
	}

	return false;
}

static bool set_image(const char *data, obs_data_t *settings, const char **type)
{
	return set_data(data, settings, type, PLUGIN_NAME_IMAGE);
}

static bool set_media(const char *data, obs_data_t *settings, const char **type)
{
	return set_data(data, settings, type, PLUGIN_NAME_MEDIA);
}

static bool set_html(const char *data, obs_data_t *settings, const char **type)
{
	if (set_data(data, settings, type, PLUGIN_NAME_HTML)) {
		obs_data_set_bool(settings, "is_local_file", true);
		return true;
	}
	return false;
}

void OBSBasic::AddDropSource(const char *data, DropType image)
{
	bool ret;
	OBSBasic *main = reinterpret_cast<OBSBasic*>(App()->GetMainWindow());
	obs_data_t *settings = obs_data_create();
	obs_source_t *source = nullptr;
	const char *type = nullptr;

	switch (image) {
	case DropType_RawText:
		ret = set_raw_text(data, settings, &type);
		break;
	case DropType_Text:
		ret = set_text(data, settings, &type);
		break;
	case DropType_Image:
		ret = set_image(data, settings, &type);
		break;
	case DropType_Media:
		ret = set_media(data, settings, &type);
		break;
	case DropType_Html:
		ret = set_html(data, settings, &type);
		break;
	}
	if (!ret)
		return;

	const char *name = obs_source_get_display_name(type);
	source = obs_source_create(type, GenerateSourceName(name).c_str(),
			settings, nullptr);
	if (source) {
		OBSScene scene = main->GetCurrentScene();
		obs_scene_add(scene, source);
		obs_source_release(source);
	}

	obs_data_release(settings);
}

void OBSBasic::dragEnterEvent(QDragEnterEvent *event)
{
	const QMimeData* mimeData = event->mimeData();
	if (mimeData->hasUrls() || mimeData->hasText()) {
		event->acceptProposedAction();
	}
}

void OBSBasic::dropEvent(QDropEvent *event)
{
	const QMimeData* mimeData = event->mimeData();

	if (mimeData->hasUrls()) {
		QList<QUrl> urls = mimeData->urls();

		for (int i = 0; i < urls.size() && i < 5; i++) {
			QString file = urls.at(i).toLocalFile();
			QFileInfo fileInfo(file);

			if (!fileInfo.exists())
				continue;

			QString suffixQStr = fileInfo.suffix();
			QByteArray suffixArray = suffixQStr.toUtf8();
			const char *suffix = suffixArray.constData();
			bool found = false;

			const char **cmp;

#define CHECK_SUFFIX(extensions, type) \
cmp = extensions; \
while (*cmp) { \
	if (strcmp(*cmp, suffix) == 0) { \
		AddDropSource(QT_TO_UTF8(file), type); \
		found = true; \
		break; \
	} \
\
	cmp++; \
} \
\
if (found) \
	continue;

#if defined(_WIN32)
			if (is_plugin_exist(PLUGIN_NAME_TEXT_GDIPLUS) ||
			    is_plugin_exist(PLUGIN_NAME_TEXT_FREETYPE2)) {
#else
			if (is_plugin_exist(PLUGIN_NAME_TEXT_FREETYPE2)) {
#endif
				CHECK_SUFFIX(textExtensions, DropType_Text);
			}

			if (is_plugin_exist(PLUGIN_NAME_HTML)) {
				CHECK_SUFFIX(htmlExtensions, DropType_Html);
			}

			if (is_plugin_exist(PLUGIN_NAME_IMAGE)) {
				CHECK_SUFFIX(imageExtensions, DropType_Image);
			}

			if (is_plugin_exist(PLUGIN_NAME_MEDIA)) {
				CHECK_SUFFIX(mediaExtensions, DropType_Media);
			}
#undef CHECK_SUFFIX
		}
	} else if (mimeData->hasText()) {
		AddDropSource(QT_TO_UTF8(mimeData->text()), DropType_RawText);
	}
}

