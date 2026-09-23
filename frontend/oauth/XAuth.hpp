#pragma once

#include "OAuth.hpp"

#include <QString>

inline constexpr quint16 XOAuthRedirectPort = 42813;

inline const Auth::Def xServiceDef = {"X", Auth::Type::OAuth_LinkedAccount, true, true};

class XAuth : public OAuthStreamKey {
	Q_OBJECT

	bool ExchangeCode(const QString &code, const QString &redirectUri, const QString &verifier);
	QString GenerateState() const;

protected:
	QString username;
	bool RefreshToken();

	virtual bool RetryLogin() override;
	virtual void SaveInternal() override;
	virtual bool LoadInternal() override;

public:
	explicit XAuth(const Def &d);

	QString Username() const { return username; }

	static QString RedirectUri();
	static std::shared_ptr<Auth> Login(QWidget *parent, const std::string &service);
};
