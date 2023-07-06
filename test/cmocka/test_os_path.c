#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <util/platform.h>

struct testcase {
	const char *path;
	const char *ext;
};

static void run_testcases(struct testcase *testcases)
{
	for (size_t i = 0; testcases[i].path; i++) {
		const char *path = testcases[i].path;

		const char *ext = os_get_path_extension(path);

		printf("path: '%s' ext: '%s'\n", path, ext);
		if (testcases[i].ext)
			assert_string_equal(ext, testcases[i].ext);
		else
			assert_ptr_equal(ext, testcases[i].ext);
	}
}

static void os_get_path_extension_test(void **state)
{
	UNUSED_PARAMETER(state);

	static struct testcase testcases[] = {
		{"/home/user/a.txt", ".txt"},
		{"C:\\Users\\user\\Documents\\video.mp4", ".mp4"},
		{"./\\", NULL},
		{".\\/", NULL},
		{"/.\\", NULL},
		{"/\\.", "."},
		{"\\/.", "."},
		{"\\./", NULL},
		{"", NULL},
		{NULL, NULL}};

	run_testcases(testcases);
}

int main()
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(os_get_path_extension_test),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
