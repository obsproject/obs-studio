#include "encoder-settings-provider-facebook.hpp"

#include <QString>
#include <QUrl>
#include <QUrlQuery>
#include <QList>
#include <QPair>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>

#include "obs-app.hpp"
#include "obs-config.h"

#include "remote-text.hpp"
#include <qt-wrappers.hpp>

namespace StreamWizard {

FacebookEncoderSettingsProvider::FacebookEncoderSettingsProvider(QObject *parent)
	: QObject(parent)
{
	currentSettings_ = nullptr;
}

void FacebookEncoderSettingsProvider::setEncoderRequest(
	QSharedPointer<EncoderSettingsRequest> request)
{
	currentSettings_ = request;
}

void FacebookEncoderSettingsProvider::run()
{
	// Base URL for request
	QUrl requestUrl(
		"https://graph.facebook.com/v8.0/video_encoder_settings");
	QUrlQuery inputVideoSettingsQuery =
		inputVideoQueryFromCurrentSettings();
	requestUrl.setQuery(inputVideoSettingsQuery);

	if (requestUrl.isValid()) {
		makeRequest(requestUrl);
	} else {
		emit returnErrorDescription(
			QTStr("PreLiveWizard.Configure.UrlError"),
			requestUrl.toDisplayString());
	}
}

void FacebookEncoderSettingsProvider::makeRequest(QUrl &url)
{
	blog(LOG_INFO, "FacebookEncoderSettingsProvider sending request");

	bool requestSuccess = false;

	std::string urlString = url.toString().toStdString();
	std::string reply;
	std::string error;
	long responseCode = 0;
	const char *contentType = "application/json";
	const char *postData = nullptr;
	std::vector<std::string> extraHeaders = std::vector<std::string>();
	int timeout = 3; // seconds

	auto apiRequestBlock = [&]() {
		requestSuccess = GetRemoteFile(urlString.c_str(), reply, error,
					       &responseCode, contentType,
					       postData, extraHeaders, nullptr,
					       timeout);
	};

	ExecuteFuncSafeBlock(apiRequestBlock);

	if (!requestSuccess || responseCode >= 400) {
		handleTimeout();
		blog(LOG_WARNING, "Server response with error: %s",
		     error.c_str());
	}

	if (reply.empty()) {
		handleEmpty();
		blog(LOG_WARNING, "Server response was empty");
	}

	QByteArray jsonBytes = QByteArray::fromStdString(reply);
	handleResponse(jsonBytes);
}

QUrlQuery FacebookEncoderSettingsProvider::inputVideoQueryFromCurrentSettings()
{
	// Get input settings, shorten name
	EncoderSettingsRequest *input = currentSettings_.data();

	QUrlQuery inputVideoSettingsQuery;
	inputVideoSettingsQuery.addQueryItem("video_type", "live");
	if (input->userSelectedResolution.isNull()) {
		inputVideoSettingsQuery.addQueryItem(
			"input_video_width",
			QString::number(input->videoWidth));
		inputVideoSettingsQuery.addQueryItem(
			"input_video_height",
			QString::number(input->videoHeight));
	} else {
		QSize wizardResolution = input->userSelectedResolution.toSize();
		inputVideoSettingsQuery.addQueryItem(
			"input_video_width",
			QString::number(wizardResolution.width()));
		inputVideoSettingsQuery.addQueryItem(
			"input_video_height",
			QString::number(wizardResolution.height()));
	}
	inputVideoSettingsQuery.addQueryItem("input_video_framerate",
					     QString::number(input->framerate));
	inputVideoSettingsQuery.addQueryItem(
		"input_video_bitrate", QString::number(input->videoBitrate));
	inputVideoSettingsQuery.addQueryItem(
		"input_audio_channels", QString::number(input->audioChannels));
	inputVideoSettingsQuery.addQueryItem(
		"input_audio_samplerate",
		QString::number(input->audioSamplerate));
	if (input->protocol == StreamProtocol::rtmps) {
		inputVideoSettingsQuery.addQueryItem("cap_streaming_protocols",
						     "rtmps");
	}
	// Defaults in OBS
	inputVideoSettingsQuery.addQueryItem("cap_video_codecs", "h264");
	inputVideoSettingsQuery.addQueryItem("cap_audio_codecs", "aac");
	return inputVideoSettingsQuery;
}

QString FacebookEncoderSettingsProvider::getOBSVersionString()
{
	QString versionString;

#ifdef HAVE_OBSCONFIG_H
	versionString += OBS_VERSION;
#else
	versionString += LIBOBS_API_MAJOR_VER + "." + LIBOBS_API_MINOR_VER +
			 "." + LIBOBS_API_PATCH_VER;
#endif

	return versionString;
}

// Helper methods for FacebookEncoderSettingsProvider::handleResponse
void addInt(const QJsonObject &json, const char *jsonKey, SettingsMap *map,
	    const char *mapKey)
{
	if (json[jsonKey].isDouble()) {
		map->insert(mapKey,
			    QPair(QVariant(json[jsonKey].toInt()), true));
	} else {
		blog(LOG_WARNING,
		     "FacebookEncoderSettingsProvider could not parse %s to Int",
		     jsonKey);
	}
}

void addStringDouble(const QJsonObject &json, const char *jsonKey,
		     SettingsMap *map, const char *mapKey)
{
	if (!json[jsonKey].isString()) {
		return;
	}
	bool converted = false;
	QString valueString = json[jsonKey].toString();
	double numberValue = valueString.toDouble(&converted);
	if (converted) {
		map->insert(mapKey, QPair(QVariant(numberValue), true));
	} else {
		blog(LOG_WARNING,
		     "FacebookEncoderSettingsProvider couldn't parse %s to Double from String",
		     jsonKey);
	}
}

void addQString(const QJsonObject &json, const char *jsonKey, SettingsMap *map,
		const char *mapKey)
{
	if (json[jsonKey].isString()) {
		map->insert(mapKey,
			    QPair(QVariant(json[jsonKey].toString()), true));
	} else {
		blog(LOG_WARNING,
		     "FacebookEncoderSettingsProvider could not parse %s to Strng",
		     jsonKey);
	}
}

void addBool(const QJsonObject &json, const char *jsonKey, SettingsMap *map,
	     const char *mapKey)
{
	if (json[jsonKey].isBool()) {
		map->insert(mapKey,
			    QPair(QVariant(json[jsonKey].toBool()), true));
	} else {
		blog(LOG_WARNING,
		     "FacebookEncoderSettingsProvider could not parse %s to Bool",
		     jsonKey);
	}
}

void FacebookEncoderSettingsProvider::handleResponse(QByteArray reply)
{
	QJsonDocument jsonDoc = QJsonDocument::fromJson(reply);

	// Parse bytes to json object
	if (!jsonDoc.isObject()) {
		blog(LOG_WARNING,
		     "FacebookEncoderSettingsProvider json doc not an object as expected");
		jsonParseError();
		return;
	}
	QJsonObject responseObject = jsonDoc.object();

	// Get the RTMPS settings object
	if (!responseObject["rtmps_settings"].isObject()) {
		QString error =
			responseObject["error"].toObject()["message"].toString();
		blog(LOG_INFO,
		     "FacebookEncoderSettingsProvider rtmps_settings not an object");
		jsonParseError();
		return;
	}
	QJsonObject rmtpSettings = responseObject["rtmps_settings"].toObject();

	// Get the video codec object
	if (!rmtpSettings["video_codec_settings"].isObject()) {
		blog(LOG_INFO,
		     "FacebookEncoderSettingsProvider video_codec_settings not an object");
		jsonParseError();
		return;
	}
	QJsonObject videoSettingsJsob =
		rmtpSettings["video_codec_settings"].toObject();

	if (videoSettingsJsob.isEmpty()) {
		handleEmpty();
	}

	// Create map to send to wizard
	SettingsMap *settingsMap = new SettingsMap();

	addInt(videoSettingsJsob, "video_bitrate", settingsMap,
	       SettingsResponseKeys.videoBitrate);
	addInt(videoSettingsJsob, "video_width", settingsMap,
	       SettingsResponseKeys.videoWidth);
	addInt(videoSettingsJsob, "video_height", settingsMap,
	       SettingsResponseKeys.videoHeight);
	addStringDouble(videoSettingsJsob, "video_framerate", settingsMap,
			SettingsResponseKeys.framerate);
	addQString(videoSettingsJsob, "video_h264_profile", settingsMap,
		   SettingsResponseKeys.h264Profile);
	addQString(videoSettingsJsob, "video_h264_level", settingsMap,
		   SettingsResponseKeys.h264Level);
	addInt(videoSettingsJsob, "video_gop_size", settingsMap,
	       SettingsResponseKeys.gopSizeInFrames);
	addQString(videoSettingsJsob, "video_gop_type", settingsMap,
		   SettingsResponseKeys.gopType);
	addBool(videoSettingsJsob, "video_gop_closed", settingsMap,
		SettingsResponseKeys.gopClosed);
	addInt(videoSettingsJsob, "video_gop_num_b_frames", settingsMap,
	       SettingsResponseKeys.gopBFrames);
	addInt(videoSettingsJsob, "video_gop_num_ref_frames", settingsMap,
	       SettingsResponseKeys.gopRefFrames);
	addQString(videoSettingsJsob, "rate_control_mode", settingsMap,
		   SettingsResponseKeys.streamRateControlMode);
	addInt(videoSettingsJsob, "buffer_size", settingsMap,
	       SettingsResponseKeys.streamBufferSize);

	// If Empty emit to empty / error state
	if (settingsMap->isEmpty()) {
		handleEmpty();
	}

	// Wrap into shared pointer and emit
	QSharedPointer<SettingsMap> settingsMapShrdPtr =
		QSharedPointer<SettingsMap>(settingsMap);
	emit newSettings(settingsMapShrdPtr);
}

void FacebookEncoderSettingsProvider::handleTimeout()
{
	QString errorTitle = QTStr("PreLiveWizard.Configure.Error.JsonParse");
	QString errorDescription = QTStr("PreLiveWizard.Error.NetworkTimeout");
	emit returnErrorDescription(errorTitle, errorDescription);
}

void FacebookEncoderSettingsProvider::handleEmpty()
{
	emit returnErrorDescription(
		QTStr("PreLiveWizard.Configure.Error.NoData"),
		QTStr("PreLiveWizard.Configure.Error.NoData.Description"));
}

void FacebookEncoderSettingsProvider::jsonParseError()
{
	emit returnErrorDescription(
		QTStr("PreLiveWizard.Configure.Error.JsonParse"),
		QTStr("PreLiveWizard.Configure.Error.JsonParse.Description"));
}

} // namespace StreamWizard
