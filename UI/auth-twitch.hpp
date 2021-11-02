#pragma once

#include <QDialog>
#include <QTimer>
#include <string>
#include <memory>

#include <json11.hpp>
#include "auth-oauth.hpp"

class BrowserDock;

class TwitchAuth : public OAuthStreamKey {
	Q_OBJECT

	friend class TwitchLogin;

	QSharedPointer<BrowserDock> chat;
	QSharedPointer<BrowserDock> info;
	QSharedPointer<BrowserDock> stat;
	QSharedPointer<BrowserDock> feed;
	QSharedPointer<QAction> chatMenu;
	QSharedPointer<QAction> infoMenu;
	QSharedPointer<QAction> statMenu;
	QSharedPointer<QAction> feedMenu;
	bool uiLoaded = false;

	std::string name;

	virtual bool RetryLogin() override;

	virtual void SaveInternal() override;
	virtual bool LoadInternal() override;

	bool MakeApiRequest(const char *path, json11::Json &json_out);
	bool GetChannelInfo();

	virtual void LoadUI() override;

public:
	TwitchAuth(const Def &d);

	static std::shared_ptr<Auth> Login(QWidget *parent,
					   const std::string &service_name);

	QTimer uiLoadTimer;

public slots:
	void TryLoadSecondaryUIPanes();
	void LoadSecondaryUIPanes();
};
