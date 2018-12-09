#include "auth-base.hpp"
#include "window-basic-main.hpp"

#ifdef BROWSER_AVAILABLE
#include "auth-twitch.hpp"
#endif

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
#ifdef BROWSER_AVAILABLE
		auth = new TwitchAuth;
#endif
	}

	main->auth.reset(auth);
	if (auth) {
		if (auth->LoadInternal()) {
			auth->LoadUI();
			return true;
		}
	}
	return false;
}

void Auth::Save()
{
	OBSBasic *main = OBSBasic::Get();
	Auth *auth = main->auth.get();
	if (!auth) {
		if (config_has_user_value(main->Config(), "Auth", "Type")) {
			config_remove_value(main->Config(), "Auth", "Type");
			config_save_safe(main->Config(), "tmp", nullptr);
		}
		return;
	}

	config_set_string(main->Config(), "Auth", "Type", auth->typeName());
	auth->SaveInternal();
	config_save_safe(main->Config(), "tmp", nullptr);
}
