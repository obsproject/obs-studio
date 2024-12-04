#pragma once

#include <QThread>

bool FetchAndVerifyFile(const char *name, const char *file, const char *url, std::string *out,
			const std::vector<std::string> &extraHeaders = std::vector<std::string>());

class WhatsNewInfoThread : public QThread {
	Q_OBJECT

	virtual void run() override;

signals:
	void Result(const QString &text);

public:
	inline WhatsNewInfoThread() {}
};
