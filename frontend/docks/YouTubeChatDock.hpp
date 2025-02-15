#pragma once

#ifdef BROWSER_AVAILABLE
#include "BrowserDock.hpp"

class YoutubeChatDock : public BrowserDock {
	Q_OBJECT

private:
	bool isLoggedIn;

public:
	YoutubeChatDock(const QString &title) : BrowserDock(title) {}

	inline void SetWidget(QCefWidget *widget_)
	{
		BrowserDock::SetWidget(widget_);
		QWidget::connect(cefWidget.get(), &QCefWidget::urlChanged, this, &YoutubeChatDock::YoutubeCookieCheck);
	}
private slots:
	void YoutubeCookieCheck();
};
#endif
