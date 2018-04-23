#include <stdbool.h>
#include <obs.h>
#include "tests.h"

static const char* locale_params[] = {
	"ar-SA",
	"bn-BD",
	"ca-ES",
	"cs-CZ",
	"da-DK",
	"de-DE",
	"el-GR",
	"en-US",
	"es-ES",
	"et-EE",
	"eu-ES",
	"fi-FI",
	"fil-PH",
	"fr-FR",
	"gl-ES",
	"he-IL",
	"hr-HR",
	"hu-HU",
	"it-IT",
	"ja-JP",
	"ko-KR",
	"nb-NO",
	"nl-NL",
	"pl-PL",
	"pt-BR",
	"pt-PT",
	"ro-RO",
	"ru-RU",
	"sk-SK",
	"sl-SI",
	"sr-CS",
	"sr-SP",
	"sv-SE",
	"th-TH",
	"tl-PH",
	"tr-TR",
	"uk-UA",
	"vi-VN",
	"zh-CN",
	"zh-TW",
	"zh-CN"
};

static const locale_params_sz =
	sizeof(locale_params) / sizeof(const char *);

static void startup_with_locale_should_return_true(void **state)
{
	for (int i = 0; i < locale_params_sz; ++i) {
		bool success = obs_startup(locale_params[i], NULL, NULL);
		assert_true(success);
		obs_shutdown();
	}
}

static const struct CMUnitTest startup_tests[] = {
	cmocka_unit_test(startup_with_locale_should_return_true)
};

int run_startup_tests(void)
{
	return cmocka_run_group_tests(startup_tests, NULL, NULL);
}