// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "oauth-obs-browser-login.hpp"

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QUuid>

#include <obs-frontend-api.h>
#include <util/util.hpp>

OAuth::OBSBrowserLogin::OBSBrowserLogin(const QString &baseUrl_,
					const std::string &urlPath_,
					const QStringList &popupWhitelistUrls_,
					QWidget *parent)
	: QDialog(parent),
	  baseUrl(baseUrl_),
	  url(baseUrl_.toStdString() + urlPath_)
{
	if (!popupWhitelistUrls_.empty()) {
		QString popupWhitelistUrlsStr = popupWhitelistUrls_.join(";");
		popupWhitelistUrls = strlist_split(
			popupWhitelistUrlsStr.toUtf8().constData(), ';', false);
	}

	setWindowTitle("Auth");
	setMinimumSize(400, 400);
	resize(700, 700);

	setWindowFlag(Qt::WindowContextHelpButtonHint, false);

	QPushButton *close = new QPushButton(tr("Cancel"));
	connect(close, &QAbstractButton::clicked, this, &QDialog::reject);

	QHBoxLayout *bottomLayout = new QHBoxLayout();
	bottomLayout->addStretch();
	bottomLayout->addWidget(close);
	bottomLayout->addStretch();

	QVBoxLayout *topLayout = new QVBoxLayout();
	topLayout->addLayout(bottomLayout);

	setLayout(topLayout);
}

void OAuth::OBSBrowserLogin::UrlChanged(const QString &url)
{
	std::string uri = "code=";
	qsizetype code_idx = url.indexOf(uri.c_str());
	if (code_idx == -1)
		return;

	if (!url.startsWith(baseUrl))
		return;

	code_idx += (int)uri.size();

	qsizetype next_idx = url.indexOf("&", code_idx);
	if (next_idx != -1)
		code = url.mid(code_idx, next_idx - code_idx);
	else
		code = url.right(url.size() - code_idx);

	accept();
}

int OAuth::OBSBrowserLogin::exec()
{
	code.clear();
	lastError.clear();

	obs_frontend_browser_params params = {0};
	params.url = url.c_str();
	params.popup_whitelist_urls = (const char **)popupWhitelistUrls.Get();
	params.enable_cookie = true;

	cefWidget.reset((QWidget *)obs_frontend_get_browser_widget(&params));
	if (cefWidget.isNull()) {
		lastError = "No CEF widget generated";
		return QDialog::Rejected;
	}

	/* Only method to connect QCefWidget signal without requiring obs-browser
	 * headers */
	connect(cefWidget.get(), SIGNAL(titleChanged(const QString &)), this,
		SLOT(setWindowTitle(const QString &)));
	connect(cefWidget.get(), SIGNAL(urlChanged(const QString &)), this,
		SLOT(UrlChanged(const QString &)));

	QVBoxLayout *layout = (QVBoxLayout *)this->layout();
	layout->insertWidget(0, cefWidget.get());

	int ret = QDialog::exec();

	cefWidget.reset();
	return ret;
}
