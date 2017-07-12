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

static const char *imageExtensions[] = {
	"bmp", "tga", "png", "jpg", "jpeg", "gif", nullptr
};

static const char *htmlExtensions[] = {
	"htm", "html", nullptr
};

static const char *mediaExtensions[] = {
	"3ga", "669", "a52", "aac", "ac3", "adt", "adts", "aif", "aifc",
	"aiff", "amb", "amr", "aob", "ape", "au", "awb", "caf", "dts",
	"flac", "it", "kar", "m4a", "m4b", "m4p", "m5p", "mid", "mka",
	"mlp", "mod", "mpa", "mp1", "mp2", "mp3", "mpc", "mpga", "mus",
	"oga", "ogg", "oma", "opus", "qcp", "ra", "rmi", "s3m", "sid",
	"spx", "tak", "thd", "tta", "voc", "vqf", "w64", "wav", "wma",
	"wv", "xa", "xm", "3g2", "3gp", "3gp2", "3gpp", "amv", "asf", "avi",
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

void OBSBasic::AddDropSource(const char *data, DropType image)
{
	OBSBasic *main = reinterpret_cast<OBSBasic*>(App()->GetMainWindow());
	obs_data_t *settings = obs_data_create();
	obs_source_t *source = nullptr;
	const char *type = nullptr;
	QString name;

	switch (image) {
	case DropType_RawText:
		obs_data_set_string(settings, "text", data);
#ifdef _WIN32
		type = "text_gdiplus";
#else
		type = "text_ft2_source";
#endif
		break;
	case DropType_Text:
#ifdef _WIN32
		obs_data_set_bool(settings, "read_from_file", true);
		obs_data_set_string(settings, "file", data);
		name = QUrl::fromLocalFile(QString(data)).fileName();
		type = "text_gdiplus";
#else
		obs_data_set_bool(settings, "from_file", true);
		obs_data_set_string(settings, "text_file", data);
		type = "text_ft2_source";
#endif
		break;
	case DropType_Image:
		obs_data_set_string(settings, "file", data);
		name = QUrl::fromLocalFile(QString(data)).fileName();
		type = "image_source";
		break;
	case DropType_Media:
		obs_data_set_string(settings, "local_file", data);
		name = QUrl::fromLocalFile(QString(data)).fileName();
		type = "ffmpeg_source";
		break;
	case DropType_Html:
		obs_data_set_bool(settings, "is_local_file", true);
		obs_data_set_string(settings, "local_file", data);
		name = QUrl::fromLocalFile(QString(data)).fileName();
		type = "browser_source";
		break;
	}

	if (!obs_source_get_display_name(type))
		return;

	if (name.isEmpty())
		name = obs_source_get_display_name(type);
	source = obs_source_create(type,
			GenerateSourceName(QT_TO_UTF8(name)).c_str(),
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
	event->acceptProposedAction();
}

void OBSBasic::dragLeaveEvent(QDragLeaveEvent *event)
{
	event->accept();
}

void OBSBasic::dragMoveEvent(QDragMoveEvent *event)
{
	event->acceptProposedAction();
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

			CHECK_SUFFIX(textExtensions, DropType_Text);
			CHECK_SUFFIX(htmlExtensions, DropType_Html);
			CHECK_SUFFIX(imageExtensions, DropType_Image);
			CHECK_SUFFIX(mediaExtensions, DropType_Media);

#undef CHECK_SUFFIX
		}
	} else if (mimeData->hasText()) {
		AddDropSource(QT_TO_UTF8(mimeData->text()), DropType_RawText);
	}
}

