#pragma once

#include <QObject>

class QTcpServer;

class AuthListener : public QObject {
	Q_OBJECT

	QTcpServer *server;
	QString state;

signals:
	void ok(const QString &code);
	void fail();

protected:
	void NewConnection();

public:
	// port 0 asks the OS for an ephemeral port. YouTube relies on that.
	// X passes XOAuthRedirectPort because the app registration is fixed.
	static constexpr quint16 EphemeralPort = 0;
	explicit AuthListener(QObject *parent = nullptr, quint16 port = EphemeralPort);
	quint16 GetPort();
	void SetState(QString state);
};
