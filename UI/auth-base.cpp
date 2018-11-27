#include "auth-base.hpp"
#include "window-basic-main.hpp"
#include "auth-twitch.hpp"

const char *Auth::typeName()
{
	if (type() == Auth::Type::Twitch)
		return "Twitch";
	return nullptr;
}

bool Auth::Load()
{
	OBSBasic *main = OBSBasic::Get();
	const char *typeStr = config_get_string(main->Config(), "Auth", "Type");
	Auth *auth = nullptr;

	if (astrcmpi(typeStr, "Twitch") == 0) {
		auth = new TwitchAuth;
	}

	main->auth.reset(auth);
	if (auth)
		return auth->LoadInternal();
	return false;
}

void Auth::Save()
{
	OBSBasic *main = OBSBasic::Get();
	Auth *auth = main->auth.get();

	if (!auth) {
		config_set_string(main->Config(), "Auth", "Type", "None");
		return;
	}

	config_set_string(main->Config(), "Auth", "Type", auth->typeName());
	auth->SaveInternal();
}
