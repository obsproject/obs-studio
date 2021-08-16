#pragma once

#include <QObject>
#include <QString>
#include <random>
#include <string>

#include "auth-oauth.hpp"

#ifdef BROWSER_AVAILABLE
class BrowserDock;
#endif

inline const std::vector<Auth::Def> youtubeServices = {
	{"YouTube - RTMP", Auth::Type::OAuth_LinkedAccount, true},
	{"YouTube - RTMPS", Auth::Type::OAuth_LinkedAccount, true},
	{"YouTube - HLS", Auth::Type::OAuth_LinkedAccount, true}};

class YoutubeAuth : public OAuthStreamKey {
	Q_OBJECT

	bool uiLoaded = false;
	std::mt19937 randomSeed;
	std::string section;

#ifdef BROWSER_AVAILABLE
	QSharedPointer<BrowserDock> chat;
	QSharedPointer<QAction> chatMenu;
#endif

	virtual bool RetryLogin() override;
	virtual void SaveInternal() override;
	virtual bool LoadInternal() override;
	virtual void LoadUI() override;

	QString GenerateState();

public:
	YoutubeAuth(const Def &d);

	void SetChatId(QString &chat_id);
	void ResetChat();

	static std::shared_ptr<Auth> Login(QWidget *parent,
					   const std::string &service);
};
