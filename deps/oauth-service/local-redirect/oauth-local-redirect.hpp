// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDesktopServices>
#include <QMessageBox>
#include <QTcpServer>
#include <QThread>
#include <QUrl>

#include <oauth.hpp>

namespace OAuth {

class AutoOpenUrlThread : public QThread {
	Q_OBJECT

	QUrl url;
	virtual void run() override { QDesktopServices::openUrl(url); }

public:
	explicit inline AutoOpenUrlThread(const QUrl &url_) : url(url_){};
};

class LocalRedirect : public QMessageBox {
	Q_OBJECT

	QString baseAuthUrl;
	QString clientId;
	QString scope;
	bool useState;

	QPushButton *reOpenUrl;
	QTcpServer server;
	QString state;

	QUrl url;

	QString redirectUri;
	QString code;

	QString lastError;

protected:
	virtual QString DialogTitle() = 0;
	virtual QString DialogText() = 0;
	virtual QString ReOpenUrlButtonText() = 0;

	virtual QString ServerPageTitle() = 0;
	virtual inline QString ServerResponsePageLogoUrl() { return {}; }
	virtual inline QString ServerResponsePageLogoAlt() { return {}; }
	virtual QString ServerResponseSuccessText() = 0;
	virtual QString ServerResponseFailureText() = 0;

	virtual inline void DebugLog(const QString & /* info */){};

public:
	LocalRedirect(const QString &baseAuthUrl, const QString &clientId,
		      const QString &scope, bool useState, QWidget *parent);
	~LocalRedirect() {}

	inline QString GetRedirectUri() { return redirectUri; }
	inline QString GetCode() { return code; }

	inline QString GetLastError() { return lastError; }

private slots:
	inline void OpenUrl() { QDesktopServices::openUrl(url); }
	void NewConnection();

public slots:
	/* Return QDialog::Accepted if redirect succeeded */
	int exec() override;
};
}
