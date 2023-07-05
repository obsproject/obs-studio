// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>

#include <oauth.hpp>
#include <util/util.hpp>

namespace OAuth {

class OBSBrowserLogin : public QDialog {
	Q_OBJECT

	QScopedPointer<QWidget> cefWidget;

	QString baseUrl;
	std::string url;

	BPtr<char *> popupWhitelistUrls = nullptr;

	QString code;

	QString lastError;

public:
	OBSBrowserLogin(const QString &baseUrl, const std::string &urlPath,
			const QStringList &popupWhitelistUrls, QWidget *parent);
	~OBSBrowserLogin() {}

	inline QString GetCode() { return code; }

	inline QString GetLastError() { return lastError; }

private slots:
	void UrlChanged(const QString &url);

public slots:
	int exec() override;
};

}
