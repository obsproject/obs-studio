#include "AuthListener.hpp"

#include <OBSApp.hpp>

#include <qt-wrappers.hpp>

#include <QRegularExpression>
#include <QTcpServer>
#include <QTcpSocket>

#include "moc_AuthListener.cpp"

#define LOGO_URL "https://obsproject.com/assets/images/new_icon_small-r.png"

static const QString serverResponseHeader = QStringLiteral("HTTP/1.0 200 OK\n"
							   "Connection: close\n"
							   "Content-Type: text/html; charset=UTF-8\n"
							   "Server: OBS Studio\n"
							   "\n"
							   "<html><head><title>OBS Studio"
							   "</title></head>");

static const QString responseTemplate = "<center>"
					"<img src=\"" LOGO_URL
					"\" alt=\"OBS\" class=\"center\"  height=\"60\" width=\"60\">"
					"</center>"
					"<center><p style=\"font-family:verdana; font-size:13pt\">%1</p></center>";

AuthListener::AuthListener(QObject *parent) : QObject(parent)
{
	server = new QTcpServer(this);
	connect(server, &QTcpServer::newConnection, this, &AuthListener::NewConnection);
	if (!server->listen(QHostAddress::LocalHost, 0)) {
		blog(LOG_DEBUG, "Server could not start");
		emit fail();
	} else {
		blog(LOG_DEBUG, "Server started at port %d", server->serverPort());
	}
}

quint16 AuthListener::GetPort()
{
	return server ? server->serverPort() : 0;
}

void AuthListener::SetState(QString state)
{
	this->state = state;
}

void AuthListener::NewConnection()
{
	QTcpSocket *socket = server->nextPendingConnection();
	if (socket) {
		connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
		connect(socket, &QTcpSocket::readyRead, socket, [&, socket]() {
			QByteArray buffer;
			while (socket->bytesAvailable() > 0) {
				buffer.append(socket->readAll());
			}
			socket->write(QT_TO_UTF8(serverResponseHeader));
			QString redirect = QString::fromLatin1(buffer);
			blog(LOG_DEBUG, "redirect: %s", QT_TO_UTF8(redirect));

			QRegularExpression re_state("(&|\\?)state=(?<state>[^&]+)");
			QRegularExpression re_code("(&|\\?)code=(?<code>[^&]+)");

			QRegularExpressionMatch match = re_state.match(redirect);

			QString code;

			if (match.hasMatch()) {
				if (state == match.captured("state")) {
					match = re_code.match(redirect);
					if (!match.hasMatch())
						blog(LOG_DEBUG, "no 'code' "
								"in server "
								"redirect");

					code = match.captured("code");
				} else {
					blog(LOG_WARNING, "state mismatch "
							  "while handling "
							  "redirect");
				}
			} else {
				blog(LOG_DEBUG, "no 'state' in "
						"server redirect");
			}

			if (code.isEmpty()) {
				auto data = responseTemplate.arg(QTStr("YouTube.Auth.NoCode"));
				socket->write(QT_TO_UTF8(data));
				emit fail();
			} else {
				auto data = responseTemplate.arg(QTStr("YouTube.Auth.Ok"));
				socket->write(QT_TO_UTF8(data));
				emit ok(code);
			}
			socket->flush();
			socket->close();
		});
	} else {
		emit fail();
	}
}
