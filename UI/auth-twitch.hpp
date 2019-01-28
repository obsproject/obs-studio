#pragma once

#include <QDialog>
#include <QTimer>
#include <string>
#include <memory>

#include "auth-oauth.hpp"

class TwitchWidget;

class TwitchAuth : public OAuthStreamKey {
	Q_OBJECT

	friend class TwitchLogin;

	QSharedPointer<TwitchWidget> chat;
	QSharedPointer<TwitchWidget> info;
	QSharedPointer<QAction> chatMenu;
	QSharedPointer<QAction> infoMenu;
	bool uiLoaded = false;

	std::string name;
	int currentScopeVer = 0;

	bool TokenExpired();
	bool RetryLogin();
	bool GetToken(const std::string &auth_code = std::string(),
			bool retry = false);

	virtual void SaveInternal() override;
	virtual bool LoadInternal() override;

	bool GetChannelInfo();

	virtual void LoadUI() override;
	virtual void OnStreamConfig() override;

public:
	TwitchAuth(const Def &d);

	static std::shared_ptr<Auth> Login(QWidget *parent);

	QTimer uiLoadTimer;

public slots:
	void TryLoadSecondaryUIPanes();
	void LoadSecondaryUIPanes();
};
