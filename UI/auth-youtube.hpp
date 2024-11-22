#pragma once

#include <QObject>
#include <QString>
#include <random>
#include <string>

#include "auth-oauth.hpp"

#ifdef BROWSER_AVAILABLE
#include "window-dock-browser.hpp"
#include <QHBoxLayout>
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

inline const std::vector<Auth::Def> youtubeServices = {{"YouTube - RTMP", Auth::Type::OAuth_LinkedAccount, true, true},
						       {"YouTube - RTMPS", Auth::Type::OAuth_LinkedAccount, true, true},
						       {"YouTube - HLS", Auth::Type::OAuth_LinkedAccount, true, true}};

class YoutubeAuth : public OAuthStreamKey {
	Q_OBJECT

	bool uiLoaded = false;
	std::string section;

#ifdef BROWSER_AVAILABLE
	YoutubeChatDock *chat = nullptr;
#endif

	virtual bool RetryLogin() override;
	virtual void SaveInternal() override;
	virtual bool LoadInternal() override;
	virtual void LoadUI() override;

	QString GenerateState();

public:
	YoutubeAuth(const Def &d);
	~YoutubeAuth();

	void SetChatId(const QString &chat_id);
	void ResetChat();
	void ReloadChat();

	static std::shared_ptr<Auth> Login(QWidget *parent, const std::string &service);
};
