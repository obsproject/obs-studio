/*!==========================================================================
* \file
* - Program:       obs
* - File:          auth-oauth-onlyfans.hpp
* - Created:       05/11/2024
* - Author:        Vitaly Bulganin
* - Description:
* - Comments:
*
-----------------------------------------------------------------------------
*
* - History:
*
===========================================================================*/
#pragma once
//-------------------------------------------------------------------------//
#pragma once
//-------------------------------------------------------------------------//
#ifndef __AUTH_OAUTH_ONLYFANS_H_EA467C00_A5FF_466A_BB88_396D5E20DDB3__
#define __AUTH_OAUTH_ONLYFANS_H_EA467C00_A5FF_466A_BB88_396D5E20DDB3__
//-------------------------------------------------------------------------//
#include <QDialog>
#include <string>
#include <memory>
//-------------------------------------------------------------------------//
#include "auth-base.hpp"
#include "auth-oauth.hpp"
//-------------------------------------------------------------------------//
class QCefWidget;
//-------------------------------------------------------------------------//
class OAuthOnlyfansLogin : public QDialog {
	Q_OBJECT

	//!< Keeps a pointer on CEF widget.
	QCefWidget *cefWidget = nullptr;

	//!< Keeps Onlyfans base url.
	const std::string of_base_url;

	//!< Keeps a token.
	QString token;
	//!< Keeps a refresh token.
	QString refresh_token;

	//!< Keeps a flag.
	bool fail = false;

public:
	/**
	 * Constructor.
	 * @param parent [in] - A pointer on parent widget.
	 * @param url [in] - A target url.
	 * @param base_url [in] - Base url.
	 */
	explicit OAuthOnlyfansLogin(QWidget *parent, const std::string &url, const std::string &base_url);

	/**
	 * Destructor.
	 */
	~OAuthOnlyfansLogin() override = default;

	//!< Gets current token.
	auto GetToken() const -> QString { return this->token; }

	//!< Gets refresh token.
	auto GetRefreshToken() const -> QString { return this->refresh_token; }

	//!< true, if it's fail, otherwise false.
	auto LoadFail() const -> bool { return this->fail; }

	// Override methods.
public:
	virtual auto exec() -> int override;
	virtual auto reject() -> void override;
	virtual auto accept() -> void override;

public slots:
	void urlChanged(const QString &url);
};
//-------------------------------------------------------------------------//
class OAuthOnlyfansStreamKey : public OAuthStreamKey {
	Q_OBJECT

public:
	/**
	 * Constructor.
	 * @param d
	 */
	explicit OAuthOnlyfansStreamKey(const Def &d) : OAuthStreamKey(d) {}

	/**
	 * Gets a new token.
	 * @param url [in] - A target URL to get a token.
	 * @param refresh_token [in] - A refresh token.
	 * @return true, if token received, otherwise false.
	 */
	auto GetToken(const char *url, const std::string &refresh_token = "") -> bool;

	/**
	 * Gets a new token.
	 * @param url [in] - A target URL to get a token.
	 * @param refresh_token [in] - A refresh token.
	 * @return true, if token received, otherwise false.
	 */
	auto GetToken(const std::string &url, const std::string &refresh_token = "") -> bool;

private:
	auto GetTokenInternal(const char *url, const std::string &auth_code) -> bool;

	// Override methods.
public:
	virtual auto OnStreamConfig() -> void override;
};
//-------------------------------------------------------------------------//
#endif // __AUTH_OAUTH_ONLYFANS_H_EA467C00_A5FF_466A_BB88_396D5E20DDB3__
