/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>
                          Zachary Lund <admin@computerquip.com>
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

#include "OBSBasic.hpp"

#include <qt-wrappers.hpp>

#include <QFileInfo>
#include <QMimeData>
#ifdef _WIN32
#include <QSettings>
#endif
#include <QUrlQuery>

using namespace std;

static const char *textExtensions[] = {"txt", "log", nullptr};

static const char *imageExtensions[] = {"bmp", "gif", "jpeg", "jpg",
#ifdef _WIN32
					"jxr",
#endif
					"png", "tga", "webp", nullptr};

static const char *htmlExtensions[] = {"htm", "html", nullptr};

static const char *mediaExtensions[] = {
	"3ga", "669",  "a52", "aac",  "ac3",   "adt",   "adts",  "aif",  "aifc", "aiff", "amb",  "amr",  "aob", "ape",
	"au",  "awb",  "caf", "dts",  "flac",  "it",    "kar",   "m4a",  "m4b",  "m4p",  "m5p",  "mid",  "mka", "mlp",
	"mod", "mpa",  "mp1", "mp2",  "mp3",   "mpc",   "mpga",  "mus",  "oga",  "ogg",  "oma",  "opus", "qcp", "ra",
	"rmi", "s3m",  "sid", "spx",  "tak",   "thd",   "tta",   "voc",  "vqf",  "w64",  "wav",  "wma",  "wv",  "xa",
	"xm",  "3g2",  "3gp", "3gp2", "3gpp",  "amv",   "asf",   "avi",  "bik",  "crf",  "divx", "drc",  "dv",  "evo",
	"f4v", "flv",  "gvi", "gxf",  "iso",   "m1v",   "m2v",   "m2t",  "m2ts", "m4v",  "mkv",  "mov",  "mp2", "mp2v",
	"mp4", "mp4v", "mpe", "mpeg", "mpeg1", "mpeg2", "mpeg4", "mpg",  "mpv2", "mts",  "mtv",  "mxf",  "mxg", "nsv",
	"nuv", "ogg",  "ogm", "ogv",  "ogx",   "ps",    "rec",   "rm",   "rmvb", "rpl",  "thp",  "tod",  "ts",  "tts",
	"txd", "vob",  "vro", "webm", "wm",    "wmv",   "wtv",   nullptr};

const std::map<OBSBasic::DropType, const char *> OBSBasic::DropTypes = {
#ifdef _WIN32
	{DropType_RawText, "text_gdiplus"}, {DropType_Text, "text_gdiplus"},
#else
	{DropType_RawText, "text_ft2_source"}, {DropType_Text, "text_ft2_source"},
#endif
	{DropType_Image, "image_source"},   {DropType_Media, "ffmpeg_source"},
	{DropType_Html, "browser_source"},  {DropType_Url, "browser_source"}};

struct OBSBasic::DropItem {
	QString url;
	QString displayName;
	DropType dropType;
	const char *typeId;

	DropItem(const QString &url, DropType dropType) : DropItem(url, QString(), dropType) {}
	DropItem(const QString &url, const QString &displayName, DropType dropType)
	{
		this->url = url;
		this->dropType = dropType;
		this->typeId = obs_get_latest_input_type_id(unversionedType());

		if (this->typeId == nullptr)
			throw invalid_argument("Unsupported drop type: " + std::string(unversionedType()));

		this->displayName = displayName.isEmpty() ? QString::fromUtf8(defaultName()) : displayName;
	};

	const char *unversionedType() const { return DropTypes.at(dropType); }
	const char *defaultName() const { return obs_source_get_display_name(typeId); }
};

static string GenerateSourceName(const char *base)
{
	string name;
	int inc = 0;

	for (;; inc++) {
		name = base;

		if (inc) {
			name += " (";
			name += to_string(inc + 1);
			name += ")";
		}

		OBSSourceAutoRelease source = obs_get_source_by_name(name.c_str());

		if (!source)
			return name;
	}
}

#ifdef _WIN32
static QString ReadWindowsURLFile(const QString &file)
{
	QSettings iniFile(file, QSettings::IniFormat);
	QVariant url = iniFile.value("InternetShortcut/URL");
	return url.toString();
}
#endif

QString OBSBasic::GetUrlDisplayName(const QString &url)
{
	QUrl qurl(url);
	QUrlQuery query = QUrlQuery(qurl.query(QUrl::FullyEncoded));
	return query.hasQueryItem("layer-name") ? query.queryItemValue("layer-name", QUrl::FullyDecoded) : qurl.host();
}

void OBSBasic::AddDropURL(const QString &url, obs_data_t *settings, const obs_video_info &ovi)
{
	QUrl path(url);
	QUrlQuery query = QUrlQuery(path.query(QUrl::FullyEncoded));

	int cx = (int)ovi.base_width;
	int cy = (int)ovi.base_height;

	if (query.hasQueryItem("layer-width"))
		cx = query.queryItemValue("layer-width").toInt();
	if (query.hasQueryItem("layer-height"))
		cy = query.queryItemValue("layer-height").toInt();
	if (query.hasQueryItem("layer-css")) {
		// QUrl::FullyDecoded does NOT properly decode a
		// application/x-www-form-urlencoded space represented as '+'
		// Thus, this is manually filtered out before QUrl's
		// decoding kicks in again. This is to allow JavaScript's
		// default searchParams.append function to simply append css
		// to the query parameters, which is the intended usecase for this.
		QString fullyEncoded = query.queryItemValue("layer-css", QUrl::FullyEncoded);
		fullyEncoded = fullyEncoded.replace("+", "%20");
		QString decoded = QUrl::fromPercentEncoding(QByteArray::fromStdString(QT_TO_UTF8(fullyEncoded)));
		obs_data_set_string(settings, "css", QT_TO_UTF8(decoded));
	}

	obs_data_set_int(settings, "width", cx);
	obs_data_set_int(settings, "height", cy);

	query.removeQueryItem("layer-width");
	query.removeQueryItem("layer-height");
	query.removeQueryItem("layer-name");
	query.removeQueryItem("layer-css");
	path.setQuery(query);

	obs_data_set_string(settings, "url", QT_TO_UTF8(path.url()));
}

void OBSBasic::AddDropSource(const DropItem &dropItem)
{
	OBSBasic *main = OBSBasic::Get();
	OBSDataAutoRelease settings = obs_data_create();
	QByteArray dataArr = dropItem.url.toUtf8();
	const char *data = dataArr.constData();
	const char *type = dropItem.typeId;

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	switch (dropItem.dropType) {
	case DropType_RawText:
		obs_data_set_string(settings, "text", data);
		break;
	case DropType_Text:
#ifdef _WIN32
		obs_data_set_bool(settings, "read_from_file", true);
		obs_data_set_string(settings, "file", data);
#else
		obs_data_set_bool(settings, "from_file", true);
		obs_data_set_string(settings, "text_file", data);
#endif
		break;
	case DropType_Image:
		obs_data_set_string(settings, "file", data);
		break;
	case DropType_Media:
		obs_data_set_string(settings, "local_file", data);
		break;
	case DropType_Html:
		obs_data_set_bool(settings, "is_local_file", true);
		obs_data_set_string(settings, "local_file", data);
		obs_data_set_int(settings, "width", ovi.base_width);
		obs_data_set_int(settings, "height", ovi.base_height);
		break;
	case DropType_Url:
		AddDropURL(dropItem.url, settings, ovi);
		break;
	}

	std::string sourceName = GenerateSourceName(QT_TO_UTF8(dropItem.displayName));
	OBSSourceAutoRelease source = obs_source_create(type, sourceName.c_str(), settings, nullptr);
	if (source) {
		OBSDataAutoRelease wrapper = obs_save_source(source);

		OBSScene scene = main->GetCurrentScene();
		std::string sceneUUID = obs_source_get_uuid(obs_scene_get_source(scene));
		std::string sourceUUID = obs_source_get_uuid(source);

		auto undo = [sceneUUID, sourceUUID](const std::string &) {
			OBSSourceAutoRelease source = obs_get_source_by_uuid(sourceUUID.c_str());
			obs_source_remove(source);
			OBSSourceAutoRelease scene = obs_get_source_by_uuid(sceneUUID.c_str());
			OBSBasic::Get()->SetCurrentScene(scene.Get(), true);
		};
		auto redo = [sceneUUID, sourceName, type](const std::string &data) {
			OBSSourceAutoRelease scene = obs_get_source_by_uuid(sceneUUID.c_str());
			OBSBasic::Get()->SetCurrentScene(scene.Get(), true);

			OBSDataAutoRelease dat = obs_data_create_from_json(data.c_str());
			OBSSourceAutoRelease source = obs_load_source(dat);
			obs_scene_add(obs_scene_from_source(scene), source.Get());
		};

		undo_s.add_action(QTStr("Undo.Add").arg(sourceName.c_str()), undo, redo, "",
				  std::string(obs_data_get_json(wrapper)));
		obs_scene_add(scene, source);
	}
}

void OBSBasic::dragEnterEvent(QDragEnterEvent *event)
{
	// refuse drops of our own widgets
	if (event->source() != nullptr) {
		event->setDropAction(Qt::IgnoreAction);
		return;
	}

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

bool OBSBasic::ConfirmDropUrl(const QString &url)
{
	if (url.left(7).compare("http://", Qt::CaseInsensitive) == 0 ||
	    url.left(8).compare("https://", Qt::CaseInsensitive) == 0) {

		activateWindow();

		QString msg = QTStr("AddUrl.Text");
		msg += "\n\n";
		msg += QTStr("AddUrl.Text.Url").arg(url);

		QMessageBox messageBox(this);
		messageBox.setWindowTitle(QTStr("AddUrl.Title"));
		messageBox.setText(msg);

		QPushButton *yesButton = messageBox.addButton(QTStr("Yes"), QMessageBox::YesRole);
		QPushButton *noButton = messageBox.addButton(QTStr("No"), QMessageBox::NoRole);
		messageBox.setDefaultButton(yesButton);
		messageBox.setEscapeButton(noButton);
		messageBox.setIcon(QMessageBox::Question);
		messageBox.exec();

		return messageBox.clickedButton() == yesButton;
	}
	return false;
}

void OBSBasic::dropEvent(QDropEvent *event)
{
	const QMimeData *mimeData = event->mimeData();
	QList<DropItem> addList{};

	try {
		if (mimeData->hasUrls()) {
			QList<QUrl> urls = mimeData->urls();

			for (int i = 0; i < urls.size(); i++) {
				QUrl url = urls[i];
				QString file = url.toLocalFile();
				QFileInfo fileInfo(file);

				if (!fileInfo.exists()) {

					if (ConfirmDropUrl(url.url())) {
						addList << DropItem{url.url(), GetUrlDisplayName(url.url()),
								    DropType_Url};
					}
					continue;
				}

#ifdef _WIN32
				if (fileInfo.suffix().compare("url", Qt::CaseInsensitive) == 0) {
					QString urlTarget = ReadWindowsURLFile(file);
					if (ConfirmDropUrl(urlTarget))
						addList << DropItem{urlTarget, GetUrlDisplayName(urlTarget),
								    DropType_Url};
					continue;
				} else if (fileInfo.isShortcut()) {
					file = fileInfo.symLinkTarget();
					fileInfo = QFileInfo(file);
					if (!fileInfo.exists()) {
						continue;
					}
				}
#endif
				QString suffixQStr = fileInfo.suffix();
				QByteArray suffixArray = suffixQStr.toUtf8();
				const char *suffix = suffixArray.constData();
				bool found = false;

				const char **cmp;

#define CHECK_SUFFIX(extensions, type)				             \
	cmp = extensions;                                                    \
	while (*cmp) {                                                       \
		if (astrcmpi(*cmp, suffix) == 0) {                           \
			addList << DropItem{file, fileInfo.fileName(), type};\
			found = true;                                        \
			break;                                               \
		}                                                            \
                                                                             \
		cmp++;                                                       \
	}                                                                    \
                                                                             \
	if (found)                                                           \
		continue;

				CHECK_SUFFIX(textExtensions, DropType_Text);
				CHECK_SUFFIX(htmlExtensions, DropType_Html);
				CHECK_SUFFIX(imageExtensions, DropType_Image);
				CHECK_SUFFIX(mediaExtensions, DropType_Media);

#undef CHECK_SUFFIX
			}
		} else if (mimeData->hasText()) {
			AddDropSource(DropItem{mimeData->text(), DropType_RawText});
		}
	}

	catch (const invalid_argument &error) {
		blog(LOG_ERROR, "%s", error.what());
	}

	if (addList.isEmpty())
		return;

	// Reverse sort order, since AddDropSource() sends signals that are processed in reverse order
	NaturalSort(addList, [](const DropItem &item) { return item.displayName; }, true);

	for (const auto &item : addList) {
		AddDropSource(item);
	}
}
