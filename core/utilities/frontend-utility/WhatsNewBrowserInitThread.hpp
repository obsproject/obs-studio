#pragma once

#include <QThread>

class WhatsNewBrowserInitThread : public QThread {
	Q_OBJECT

	QString url;

	virtual void run() override;

signals:
	void Result(const QString &url);

public:
	inline WhatsNewBrowserInitThread(const QString &url_) : url(url_) {}
};
