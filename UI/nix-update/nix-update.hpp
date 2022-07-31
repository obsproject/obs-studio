#pragma once

#include <QThread>
#include <QString>

class WhatsNewInfoThread : public QThread {
	Q_OBJECT

	virtual void run() override;

signals:
	void Result(const QString &text);

public:
	inline WhatsNewInfoThread() {}
};

class WhatsNewBrowserInitThread : public QThread {
	Q_OBJECT

	QString url;

	virtual void run() override;

signals:
	void Result(const QString &url);

public:
	inline WhatsNewBrowserInitThread(const QString &url_) : url(url_) {}
};
