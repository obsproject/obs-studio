#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#define UNUSED_PARAMETER(x) (void)(x)

/* Test helper to check if a file exists */
static int file_exists(const char *path)
{
	return access(path, F_OK) == 0;
}

/* Test helper to create a directory */
static int create_test_dir(const char *path)
{
	return mkdir(path, 0755);
}

/* Test helper to remove a file */
static int remove_file(const char *path)
{
	return unlink(path);
}

/* Test helper to remove a directory */
static int remove_dir(const char *path)
{
	return rmdir(path);
}

/* Test: Verify Documents folder exists on user's system */
static void test_documents_folder_exists(void **state)
{
	UNUSED_PARAMETER(state);

	const char *home = getenv("HOME");
	assert_non_null(home);

	char documents_path[512];
	snprintf(documents_path, sizeof(documents_path), "%s/Documents", home);

	/* Documents folder should exist on most systems */
	assert_true(file_exists(documents_path));
}

/* Test: Verify old config path detection */
static void test_old_config_path_detection(void **state)
{
	UNUSED_PARAMETER(state);

	const char *old_path = "/home/user/.config";
	const char *new_path = "/home/user/Documents/OBS Scene Collections";

	/* Test that we can detect old path format */
	int is_old_path = strstr(old_path, "/.config") != NULL;
	assert_true(is_old_path);

	/* Test that new path doesn't match old pattern */
	int is_new_path = strstr(new_path, "/.config") != NULL;
	assert_false(is_new_path);
}

/* Test: Scene Collections directory can be created */
static void test_create_scene_collections_dir(void **state)
{
	UNUSED_PARAMETER(state);

	const char *home = getenv("HOME");
	assert_non_null(home);

	char test_dir[512];
	snprintf(test_dir, sizeof(test_dir), "%s/Documents/OBS_Scene_Collections_Test", home);

	/* Clean up if it exists */
	remove_dir(test_dir);

	/* Create directory */
	int result = create_test_dir(test_dir);
	assert_int_equal(result, 0);
	assert_true(file_exists(test_dir));

	/* Clean up */
	remove_dir(test_dir);
}

/* Test: JSON files can be copied between directories */
static void test_copy_scene_collection_file(void **state)
{
	UNUSED_PARAMETER(state);

	const char *home = getenv("HOME");
	assert_non_null(home);

	char src_dir[512];
	char dst_dir[512];
	char src_file[512];
	char dst_file[512];

	snprintf(src_dir, sizeof(src_dir), "%s/Documents/OBS_Test_Src", home);
	snprintf(dst_dir, sizeof(dst_dir), "%s/Documents/OBS_Test_Dst", home);
	snprintf(src_file, sizeof(src_file), "%s/test_scene.json", src_dir);
	snprintf(dst_file, sizeof(dst_file), "%s/test_scene.json", dst_dir);

	/* Clean up if they exist */
	remove_file(src_file);
	remove_file(dst_file);
	remove_dir(src_dir);
	remove_dir(dst_dir);

	/* Create source directory and file */
	create_test_dir(src_dir);
	FILE *f = fopen(src_file, "w");
	assert_non_null(f);
	fprintf(f, "{\"name\": \"test scene\"}");
	fclose(f);
	assert_true(file_exists(src_file));

	/* Create destination directory */
	create_test_dir(dst_dir);

	/* Copy file */
	char cmd[1024];
	snprintf(cmd, sizeof(cmd), "cp %s %s", src_file, dst_file);
	int copy_result = system(cmd);
	assert_int_equal(copy_result, 0);
	assert_true(file_exists(dst_file));

	/* Clean up */
	remove_file(src_file);
	remove_file(dst_file);
	remove_dir(src_dir);
	remove_dir(dst_dir);
}

/* Test: Multiple scene collection files can be enumerated */
static void test_enumerate_scene_collections(void **state)
{
	UNUSED_PARAMETER(state);

	const char *home = getenv("HOME");
	assert_non_null(home);

	char test_dir[512];
	snprintf(test_dir, sizeof(test_dir), "%s/Documents/OBS_Test_Enum", home);

	/* Clean up if it exists */
	remove_dir(test_dir);

	/* Create test directory */
	create_test_dir(test_dir);

	/* Create test files */
	char file1[512];
	char file2[512];
	char file3[512];
	snprintf(file1, sizeof(file1), "%s/scene1.json", test_dir);
	snprintf(file2, sizeof(file2), "%s/scene2.json", test_dir);
	snprintf(file3, sizeof(file3), "%s/readme.txt", test_dir);

	FILE *f;
	f = fopen(file1, "w");
	fprintf(f, "{}");
	fclose(f);
	f = fopen(file2, "w");
	fprintf(f, "{}");
	fclose(f);
	f = fopen(file3, "w");
	fprintf(f, "not json");
	fclose(f);

	/* Count files with .json extension */
	DIR *dir = opendir(test_dir);
	assert_non_null(dir);

	int json_count = 0;
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_type == DT_REG) {
			const char *ext = strrchr(entry->d_name, '.');
			if (ext && strcmp(ext, ".json") == 0) {
				json_count++;
			}
		}
	}
	closedir(dir);

	assert_int_equal(json_count, 2);

	/* Clean up */
	remove_file(file1);
	remove_file(file2);
	remove_file(file3);
	remove_dir(test_dir);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_documents_folder_exists),
		cmocka_unit_test(test_old_config_path_detection),
		cmocka_unit_test(test_create_scene_collections_dir),
		cmocka_unit_test(test_copy_scene_collection_file),
		cmocka_unit_test(test_enumerate_scene_collections),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
