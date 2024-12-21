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
	explicit AuthListener(QObject *parent = 0);
	quint16 GetPort();
	void SetState(QString state);
};
