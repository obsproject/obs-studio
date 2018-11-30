#pragma once

#include <QDialog>
#include <string>

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

class TwitchAuth : public Auth {
	friend class TwitchLogin;

	std::string token;

	std::string title_;
	std::string game_;
	std::string key_;
	std::string id_;

	virtual void SaveInternal() override;
	virtual bool LoadInternal() override;

public:
	inline explicit TwitchAuth() {}

	bool GetChannelInfo(std::string &error);
	bool SetChannelInfo(std::string &error);

	inline const std::string &title() const {return title_;}
	inline const std::string &game() const {return game_;}
	inline const std::string &key() const {return key_;}
	inline const std::string &id() const {return id_;}

	inline void setTitle(const std::string &title) {title_ = title;}
	inline void setGame(const std::string &game) {game_ = game;}

	virtual Auth::Type type() const override {return Auth::Type::Twitch;}
	virtual Auth *Clone() const override;

	virtual void LoadUI() override;

	static Auth *Login(QWidget *parent);
};
