#pragma once

#include <QObject>
#include <QString>
#include <random>
#include <string>

#include "auth-oauth.hpp"

#ifdef BROWSER_AVAILABLE
#include "window-dock-browser.hpp"
#include "lineedit-autoresize.hpp"
#include <QHBoxLayout>
class YoutubeChatDock : public BrowserDock {
	Q_OBJECT

private:
	std::string apiChatId;
	LineEditAutoResize *lineEdit;
	QPushButton *sendButton;
	QHBoxLayout *chatLayout;

public:
	inline YoutubeChatDock(const QString &title) : BrowserDock(title) {}
	void SetWidget(QCefWidget *widget_);
	void SetApiChatId(const std::string &id);

private slots:
	void SendChatMessage();
	void ShowErrorMessage(const QString &error);
	void EnableChatInput();
};
#endif

inline const std::vector<Auth::Def> youtubeServices = {
	{"YouTube - RTMP", Auth::Type::OAuth_LinkedAccount, true, true},
	{"YouTube - RTMPS", Auth::Type::OAuth_LinkedAccount, true, true},
	{"YouTube - HLS", Auth::Type::OAuth_LinkedAccount, true, true}};

class YoutubeAuth : public OAuthStreamKey {
	Q_OBJECT

	bool uiLoaded = false;
	std::string section;

#ifdef BROWSER_AVAILABLE
	YoutubeChatDock *chat;
#endif

	virtual bool RetryLogin() override;
	virtual void SaveInternal() override;
	virtual bool LoadInternal() override;
	virtual void LoadUI() override;

	QString GenerateState();

public:
	YoutubeAuth(const Def &d);
	~YoutubeAuth();

	void SetChatId(const QString &chat_id, const std::string &api_chat_id);
	void ResetChat();

	static std::shared_ptr<Auth> Login(QWidget *parent,
					   const std::string &service);
};
