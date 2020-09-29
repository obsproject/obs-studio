#pragma once

#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QUrlQuery>
#include <QUrl>
#include <QSize>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "pre-stream-current-settings.hpp"

namespace StreamWizard {

class FacebookEncoderSettingsProvider : public QObject {
	Q_OBJECT

public:
	FacebookEncoderSettingsProvider(QObject *parent = nullptr);

	// Pass in encoder request to use, returns FALSE is there is an error.
	void setEncoderRequest(QSharedPointer<EncoderSettingsRequest> request);

	// Uses the EncoderSettingsRequest to generate SettingsMap
	// Does not return in sync becuase can be an async API call
	// Success: emits returnedEncoderSettings(SettingsMap)
	// Failure: emits returnedError(QString title, QString description)
	void run();

signals:
	// On success.
	void newSettings(QSharedPointer<SettingsMap> response);
	// On failure. Will be shown to user.
	void returnErrorDescription(QString title, QString description);

private:
	QSharedPointer<EncoderSettingsRequest> currentSettings_;
	QNetworkAccessManager *restclient_;

	void makeRequest(QUrl &url);
	QUrlQuery inputVideoQueryFromCurrentSettings();
	QString getOBSVersionString();
	void jsonParseError();

private slots:
	void handleResponse(QNetworkReply *reply);
};

} // namespace StreamWizard
