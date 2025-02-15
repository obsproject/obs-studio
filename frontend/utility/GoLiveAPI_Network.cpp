#include "GoLiveAPI_Network.hpp"
#include "GoLiveAPI_CensoredJson.hpp"

#include <OBSApp.hpp>
#include <utility/MultitrackVideoError.hpp>
#include <utility/RemoteTextThread.hpp>

#include <obs.hpp>

#include <QMessageBox>
#include <QThreadPool>
#include <nlohmann/json.hpp>
#include <qstring.h>

#include <string>

using json = nlohmann::json;

Qt::ConnectionType BlockingConnectionTypeFor(QObject *object);

void HandleGoLiveApiErrors(QWidget *parent, const json &raw_json, const GoLiveApi::Config &config)
{
	using GoLiveApi::StatusResult;

	if (!config.status)
		return;

	auto &status = *config.status;
	if (status.result == StatusResult::Success)
		return;

	auto warn_continue = [&](QString message) {
		bool ret = false;
		QMetaObject::invokeMethod(
			parent,
			[=] {
				QMessageBox mb(parent);
				mb.setIcon(QMessageBox::Warning);
				mb.setWindowTitle(QTStr("ConfigDownload.WarningMessageTitle"));
				mb.setTextFormat(Qt::RichText);
				mb.setText(message + QTStr("FailedToStartStream.WarningRetry"));
				mb.setStandardButtons(QMessageBox::StandardButton::Yes |
						      QMessageBox::StandardButton::No);
				return mb.exec() == QMessageBox::StandardButton::No;
			},
			BlockingConnectionTypeFor(parent), &ret);
		if (ret)
			throw MultitrackVideoError::cancel();
	};

	auto missing_html = [] {
		return QTStr("FailedToStartStream.StatusMissingHTML").toStdString();
	};

	if (status.result == StatusResult::Unknown) {
		return warn_continue(QTStr("FailedToStartStream.WarningUnknownStatus")
					     .arg(raw_json["status"]["result"].dump().c_str()));

	} else if (status.result == StatusResult::Warning) {
		if (config.encoder_configurations.empty()) {
			throw MultitrackVideoError::warning(status.html_en_us.value_or(missing_html()).c_str());
		}

		return warn_continue(status.html_en_us.value_or(missing_html()).c_str());
	} else if (status.result == StatusResult::Error) {
		throw MultitrackVideoError::critical(status.html_en_us.value_or(missing_html()).c_str());
	}
}

GoLiveApi::Config DownloadGoLiveConfig(QWidget *parent, QString url, const GoLiveApi::PostData &post_data,
				       const QString &multitrack_video_name)
{
	json post_data_json = post_data;
	blog(LOG_INFO, "Go live POST data: %s", censoredJson(post_data_json).toUtf8().constData());

	if (url.isEmpty())
		throw MultitrackVideoError::critical(QTStr("FailedToStartStream.MissingConfigURL"));

	std::string encodeConfigText;
	std::string libraryError;

	std::vector<std::string> headers;
	headers.push_back("Content-Type: application/json");
	bool encodeConfigDownloadedOk = GetRemoteFile(url.toLocal8Bit(), encodeConfigText,
						      libraryError, // out params
						      nullptr,
						      nullptr, // out params (response code and content type)
						      "POST", post_data_json.dump().c_str(), headers,
						      nullptr, // signature
						      5);      // timeout in seconds

	if (!encodeConfigDownloadedOk)
		throw MultitrackVideoError::warning(
			QTStr("FailedToStartStream.ConfigRequestFailed").arg(url, libraryError.c_str()));
	try {
		auto data = json::parse(encodeConfigText);
		blog(LOG_INFO, "Go live response data: %s", censoredJson(data, true).toUtf8().constData());
		GoLiveApi::Config config = data;
		HandleGoLiveApiErrors(parent, data, config);
		return config;

	} catch (const json::exception &e) {
		blog(LOG_INFO, "Failed to parse go live config: %s", e.what());
		throw MultitrackVideoError::warning(
			QTStr("FailedToStartStream.FallbackToDefault").arg(multitrack_video_name));
	}
}

QString MultitrackVideoAutoConfigURL(obs_service_t *service)
{
	static const std::optional<QString> cli_url = []() -> std::optional<QString> {
		auto args = qApp->arguments();
		for (int i = 0; i < args.length() - 1; i++) {
			if (args[i] == "--config-url" && args.length() > (i + 1)) {
				return args[i + 1];
			}
		}
		return std::nullopt;
	}();

	QString url;
	if (cli_url.has_value()) {
		url = *cli_url;
	} else {
		OBSDataAutoRelease settings = obs_service_get_settings(service);
		url = obs_data_get_string(settings, "multitrack_video_configuration_url");
	}

	blog(LOG_INFO, "Go live URL: %s", url.toUtf8().constData());
	return url;
}
