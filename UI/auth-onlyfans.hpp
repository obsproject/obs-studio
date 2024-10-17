/*!==========================================================================
* \file
* - Program:       obs
* - File:          auth-onlyfans.hpp
* - Created:       01/21/2024
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
#ifndef __AUTH_ONLYFANS_H_4D3D1F41_CB31_4F41_9EE9_EABE7244DDE3__
#define __AUTH_ONLYFANS_H_4D3D1F41_CB31_4F41_9EE9_EABE7244DDE3__
//-------------------------------------------------------------------------//
#include <QDialog>
#include <QTimer>
#include <string>
#include <memory>

#include <json11.hpp>
//-------------------------------------------------------------------------//
#include "auth-oauth-onlyfans.hpp"
//-------------------------------------------------------------------------//
class BrowserDock;
//-------------------------------------------------------------------------//
class OnlyfansAuth final : public OAuthOnlyfansStreamKey {
	typedef OAuthOnlyfansStreamKey base_class;

	Q_OBJECT

	bool uiLoaded = false;

	std::string name;
	std::string uuid;

	QCefWidget *cefWidget = nullptr;
	QTimer updaterTimer;

public:
	/**
	 * Makes a login.
	 * @param parent [in] - A parent widget.
	 * @param service_name [in] - A name of service.
	 * @return A pointer on auth.
	 */
	static std::shared_ptr<Auth> Login(QWidget *parent, const std::string &service_name);

public:
	/**
	 * Constructor.
	 * @param d
	 */
	explicit OnlyfansAuth(const Def &d);

	/**
	 * Constructor.
	 * @param d
	 * @param token [in] - A client token.
	 * @param refresh_token [in] - A client refresh token.
	 */
	explicit OnlyfansAuth(const Def &d, const std::string &token, const std::string &refresh_token);

	/**
	 * Destructor.
	 * @throw None.
	 */
	virtual ~OnlyfansAuth() noexcept;

	// Override methods
private:
	virtual bool RetryLogin() override;

	virtual void SaveInternal() override;
	virtual bool LoadInternal() override;

	virtual void LoadUI() override;

private:
	bool MakeApiRequest(const char *path, json11::Json &json_out);
	bool GetChannelInfo();

private slots:
	void updateSettingsUIPanes();
};
//-------------------------------------------------------------------------//
#endif // __AUTH_ONLYFANS_H_4D3D1F41_CB31_4F41_9EE9_EABE7244DDE3__
