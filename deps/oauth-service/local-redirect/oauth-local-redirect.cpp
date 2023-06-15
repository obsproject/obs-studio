// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "oauth-local-redirect.hpp"

#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTcpSocket>
#include <QUuid>

constexpr const char *REDIRECT_URI_TEMPLATE = "http://127.0.0.1:%1";

constexpr const char *QUERY_TEMPLATE =
	"?response_type=code&client_id=%1&redirect_uri=%2&scope=%3";

constexpr const char *RESPONSE_HEADER =
	"HTTP/1.0 200 OK\n"
	"Connection: close\n"
	"Content-Type: text/html; charset=UTF-8\n"
	"Server: %1\n"
	"\n"
	"<html><head><title>%1"
	"</title></head>";

constexpr const char *RESPONSE_LOGO_TEMPLATE =
	"<center>"
	"<img src=\"%1\" alt=\"%2\" class=\"center\"  height=\"60\" width=\"60\">"
	"</center>";
constexpr const char *RESPONSE_TEMPLATE =
	"<center><p style=\"font-family:verdana; font-size:13pt\">%1</p></center>";

OAuth::LocalRedirect::LocalRedirect(const QString &baseAuthUrl_,
				    const QString &clientId_,
				    const QString &scope_, bool useState_,
				    QWidget *parent)
	: QMessageBox(QMessageBox::NoIcon, "placeholder", "placeholder",
		      QMessageBox::NoButton, parent),
	  baseAuthUrl(baseAuthUrl_),
	  clientId(clientId_),
	  scope(scope_),
	  useState(useState_)
{
	setWindowFlag(Qt::WindowCloseButtonHint, false);
	setTextFormat(Qt::RichText);

	reOpenUrl = addButton("placeholder", QMessageBox::HelpRole);
	QPushButton *cancel = addButton(QMessageBox::Cancel);
	cancel->setText(tr("Cancel"));

	/* Prevent the re-open URL button to close the dialog  */
	reOpenUrl->disconnect(SIGNAL(clicked(bool)));

	connect(reOpenUrl, &QPushButton::clicked, this,
		&OAuth::LocalRedirect::OpenUrl);
	connect(&server, &QTcpServer::newConnection, this,
		&OAuth::LocalRedirect::NewConnection);
}

void OAuth::LocalRedirect::NewConnection()
{
	if (!server.hasPendingConnections())
		return;

	QTcpSocket *socket = server.nextPendingConnection();
	connect(socket, &QTcpSocket::disconnected, socket,
		&QTcpSocket::deleteLater);
	socket->waitForReadyRead();

	QByteArray buffer;
	while (socket->bytesAvailable() > 0)
		buffer.append(socket->readAll());

	socket->write(QString(RESPONSE_HEADER)
			      .arg(ServerPageTitle())
			      .toUtf8()
			      .constData());

	QString bufferStr = QString::fromLatin1(buffer);

	QRegularExpressionMatch regexState =
		QRegularExpression("(&|\\?)state=(?<state>[^&]+)")
			.match(bufferStr);
	QRegularExpressionMatch regexCode =
		QRegularExpression("(&|\\?)code=(?<code>[^&]+)")
			.match(bufferStr);

	if (useState) {
		if (!regexState.hasMatch()) {
			lastError = "No 'state' in server redirect";
		} else if (state != regexState.captured("state")) {
			lastError = "State and returned state mismatch";
		}
	}

	if (!regexCode.hasMatch()) {
		lastError = "No 'code' in server redirect";
	} else {
		code = regexCode.captured("code");
	}

	QString responseTemplate;
	if (!ServerResponsePageLogoUrl().isEmpty())
		responseTemplate = QString(RESPONSE_LOGO_TEMPLATE)
					   .arg(ServerResponsePageLogoUrl())
					   .arg(ServerResponsePageLogoAlt());

	responseTemplate += RESPONSE_TEMPLATE;

	if (code.isEmpty())
		lastError = "'code' was found empty";

	socket->write(responseTemplate
			      .arg(lastError.isEmpty()
					   ? ServerResponseSuccessText()
					   : ServerResponseFailureText())
			      .toUtf8()
			      .constData());

	socket->flush();
	socket->close();

	if (!lastError.isEmpty())
		reject();
	else
		accept();
}

int OAuth::LocalRedirect::exec()
{
	redirectUri.clear();
	code.clear();

	lastError.clear();

	setWindowTitle(DialogTitle());
	setText(DialogText());
	reOpenUrl->setText(ReOpenUrlButtonText());

	if (!server.listen(QHostAddress::LocalHost, 0)) {
		lastError = "Server could not start";
		return QDialog::Rejected;
	}

	DebugLog(QString("Server started at port %1").arg(server.serverPort()));

	redirectUri = QString(REDIRECT_URI_TEMPLATE).arg(server.serverPort());

	QString urlStr = baseAuthUrl;
	urlStr += QUERY_TEMPLATE;
	urlStr = urlStr.arg(clientId).arg(redirectUri).arg(scope);

	if (useState) {
		state = QUuid::createUuid().toString(QUuid::Id128);

		urlStr += "&state=";
		urlStr += state;
	}

	url = QUrl(urlStr, QUrl::StrictMode);
	AutoOpenUrlThread autoOpenUrl(url);

	autoOpenUrl.start();
	int ret = QMessageBox::exec();

	if (ret == QMessageBox::Cancel)
		ret = QDialog::Rejected;

	autoOpenUrl.wait();
	server.close();
	url.clear();
	state.clear();
	return ret;
}
