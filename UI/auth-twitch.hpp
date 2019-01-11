#pragma once

#include <QDialog>
#include <string>
#include <memory>

#include "auth-base.hpp"

class QCefWidget;

class TwitchLogin : public QDialog {
	Q_OBJECT

	QCefWidget *cefWidget = nullptr;
	QString token;
	bool fail = false;

public:
	TwitchLogin(QWidget *parent);
	~TwitchLogin();

	inline QString GetToken() const {return token;}
	inline bool LoadFail() const {return fail;}

public slots:
	void urlChanged(const QString &url);
};

class TwitchChat;

class TwitchAuth : public Auth {
	friend class TwitchLogin;

	QSharedPointer<TwitchChat> chat;
	bool uiLoaded = false;

	QSharedPointer<QAction> chatMenu;

	std::string token;

	std::string title_;
	std::string name_;
	std::string game_;
	std::string key_;
	std::string id_;

	virtual void SaveInternal() override;
	virtual bool LoadInternal() override;

public:
	TwitchAuth();

	bool GetChannelInfo();
	bool SetChannelInfo();

	inline const std::string &title() const {return title_;}
	inline const std::string &game() const {return game_;}
	inline const std::string &name() const {return name_;}
	inline const std::string &key() const {return key_;}
	inline const std::string &id() const {return id_;}

	inline void setTitle(const std::string &title) {title_ = title;}
	inline void setGame(const std::string &game) {game_ = game;}

	virtual Auth::Type type() const override {return Auth::Type::Twitch;}

	virtual void LoadUI() override;
	virtual void OnStreamConfig() override;

	static std::shared_ptr<Auth> Login(QWidget *parent);
};
