#pragma once

#include "OAuth.hpp"

inline const std::vector<Auth::Def> youtubeServices = {{"YouTube - RTMP", Auth::Type::OAuth_LinkedAccount, true, true},
						       {"YouTube - RTMPS", Auth::Type::OAuth_LinkedAccount, true, true},
						       {"YouTube - HLS", Auth::Type::OAuth_LinkedAccount, true, true}};

#ifdef BROWSER_AVAILABLE
class YoutubeChatDock;
#endif

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
