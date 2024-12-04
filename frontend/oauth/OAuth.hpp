#pragma once

#include "Auth.hpp"

class OAuth : public Auth {
	Q_OBJECT

public:
	inline OAuth(const Def &d) : Auth(d) {}

	typedef std::function<std::shared_ptr<Auth>(QWidget *, const std::string &service_name)> login_cb;
	typedef std::function<void()> delete_cookies_cb;

	static std::shared_ptr<Auth> Login(QWidget *parent, const std::string &service);
	static void DeleteCookies(const std::string &service);

	static void RegisterOAuth(const Def &d, create_cb create, login_cb login, delete_cookies_cb delete_cookies);

protected:
	std::string refresh_token;
	std::string token;
	bool implicit = false;
	uint64_t expire_time = 0;
	int currentScopeVer = 0;

	virtual void SaveInternal() override;
	virtual bool LoadInternal() override;

	virtual bool RetryLogin() = 0;
	bool TokenExpired();
	bool GetToken(const char *url, const std::string &client_id, int scope_ver,
		      const std::string &auth_code = std::string(), bool retry = false);
	bool GetToken(const char *url, const std::string &client_id, const std::string &secret,
		      const std::string &redirect_uri, int scope_ver, const std::string &auth_code, bool retry);

private:
	bool GetTokenInternal(const char *url, const std::string &client_id, const std::string &secret,
			      const std::string &redirect_uri, int scope_ver, const std::string &auth_code, bool retry);
};

class OAuthStreamKey : public OAuth {
	Q_OBJECT

protected:
	std::string key_;

public:
	inline OAuthStreamKey(const Def &d) : OAuth(d) {}

	inline const std::string &key() const { return key_; }

	virtual void OnStreamConfig() override;
};
