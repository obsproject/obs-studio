#pragma once

#include "OAuth.hpp"

#include <json11.hpp>

#include <QTimer>

class TwitchAuth : public OAuthStreamKey {
	Q_OBJECT

	bool uiLoaded = false;

	std::string name;
	std::string uuid;

	virtual bool RetryLogin() override;

	virtual void SaveInternal() override;
	virtual bool LoadInternal() override;

	bool MakeApiRequest(const char *path, json11::Json &json_out);
	bool GetChannelInfo();

	virtual void LoadUI() override;

public:
	TwitchAuth(const Def &d);
	~TwitchAuth();

	static std::shared_ptr<Auth> Login(QWidget *parent, const std::string &service_name);

	QTimer uiLoadTimer;

public slots:
	void TryLoadSecondaryUIPanes();
	void LoadSecondaryUIPanes();
};
