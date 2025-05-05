/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>
    Copyright (C) 2014 by Zachary Lund <admin@computerquip.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "obs-internal.h"
#include "obs-nix.h"
#include "obs-nix-platform.h"
#include "obs-nix-x11.h"

#include "util/config-file.h"

#ifdef ENABLE_WAYLAND
#include "obs-nix-wayland.h"
#endif

#if defined(__FreeBSD__)
#define _GNU_SOURCE
#endif
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#if defined(__FreeBSD__) || defined(__OpenBSD__)
#include <sys/sysctl.h>
#endif
#if !defined(__OpenBSD__)
#include <sys/sysinfo.h>
#endif
#include <sys/utsname.h>
#include <inttypes.h>

const char *get_module_extension(void)
{
	return ".so";
}

#define FLATPAK_PLUGIN_PATH "/app/plugins"

static const char *module_bin[] = {
	"../../obs-plugins/64bit",
	OBS_INSTALL_PREFIX "/" OBS_PLUGIN_DESTINATION,
	FLATPAK_PLUGIN_PATH "/" OBS_PLUGIN_DESTINATION,
};

static const char *module_data[] = {
	OBS_DATA_PATH "/obs-plugins/%module%",
	OBS_INSTALL_DATA_PATH "/obs-plugins/%module%",
	FLATPAK_PLUGIN_PATH "/share/obs/obs-plugins/%module%",
};

static const int module_patterns_size = sizeof(module_bin) / sizeof(module_bin[0]);

static const struct obs_nix_hotkeys_vtable *hotkeys_vtable = NULL;

void add_default_module_paths(void)
{
	char *module_bin_path = os_get_executable_path_ptr("../" OBS_PLUGIN_PATH);
	char *module_data_path = os_get_executable_path_ptr("../" OBS_DATA_PATH "/obs-plugins/%module%");

	if (module_bin_path && module_data_path) {
		char *abs_module_bin_path = os_get_abs_path_ptr(module_bin_path);
		char *abs_module_install_path = os_get_abs_path_ptr(OBS_INSTALL_PREFIX "/" OBS_PLUGIN_DESTINATION);

		if (abs_module_bin_path &&
		    (!abs_module_install_path || strcmp(abs_module_bin_path, abs_module_install_path) != 0)) {
			obs_add_module_path(module_bin_path, module_data_path);
		}
		bfree(abs_module_install_path);
		bfree(abs_module_bin_path);
	}

	bfree(module_bin_path);
	bfree(module_data_path);

	for (int i = 0; i < module_patterns_size; i++) {
		obs_add_module_path(module_bin[i], module_data[i]);
	}
}

/*
 *   /usr/local/share/libobs
 *   /usr/share/libobs
 */
char *find_libobs_data_file(const char *file)
{
	struct dstr output;
	dstr_init(&output);

	if (check_path(file, OBS_DATA_PATH "/libobs/", &output))
		return output.array;

	char *relative_data_path = os_get_executable_path_ptr("../" OBS_DATA_PATH "/libobs/");
	if (relative_data_path) {
		bool found = check_path(file, relative_data_path, &output);

		bfree(relative_data_path);

		if (found) {
			return output.array;
		}
	}

	if (OBS_INSTALL_PREFIX[0] != 0) {
		if (check_path(file, OBS_INSTALL_DATA_PATH "/libobs/", &output))
			return output.array;
	}

	dstr_free(&output);
	return NULL;
}

static void log_processor_cores(void)
{
	blog(LOG_INFO, "Physical Cores: %d, Logical Cores: %d", os_get_physical_cores(), os_get_logical_cores());
}

#if defined(__linux__)
static void log_processor_info(void)
{
	int physical_id = -1;
	int last_physical_id = -1;
	char *line = NULL;
	size_t linecap = 0;

	FILE *fp;
	struct dstr proc_name;
	struct dstr proc_speed;

	fp = fopen("/proc/cpuinfo", "r");
	if (!fp)
		return;

	dstr_init(&proc_name);
	dstr_init(&proc_speed);

	while (getline(&line, &linecap, fp) != -1) {
		if (!strncmp(line, "model name", 10)) {
			char *start = strchr(line, ':');
			if (!start || *(++start) == '\0')
				continue;

			dstr_copy(&proc_name, start);
			dstr_resize(&proc_name, proc_name.len - 1);
			dstr_depad(&proc_name);
		}

		if (!strncmp(line, "physical id", 11)) {
			char *start = strchr(line, ':');
			if (!start || *(++start) == '\0')
				continue;

			physical_id = atoi(start);
		}

		if (!strncmp(line, "cpu MHz", 7)) {
			char *start = strchr(line, ':');
			if (!start || *(++start) == '\0')
				continue;

			dstr_copy(&proc_speed, start);
			dstr_resize(&proc_speed, proc_speed.len - 1);
			dstr_depad(&proc_speed);
		}

		if (*line == '\n' && physical_id != last_physical_id) {
			last_physical_id = physical_id;
			blog(LOG_INFO, "CPU Name: %s", proc_name.array);
			blog(LOG_INFO, "CPU Speed: %sMHz", proc_speed.array);
		}
	}

	fclose(fp);
	dstr_free(&proc_name);
	dstr_free(&proc_speed);
	free(line);
}
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
static void log_processor_speed(void)
{
#ifndef __OpenBSD__
	char *line = NULL;
	size_t linecap = 0;
	FILE *fp;
	struct dstr proc_speed;

	fp = fopen("/var/run/dmesg.boot", "r");
	if (!fp) {
		blog(LOG_INFO, "CPU: Missing /var/run/dmesg.boot !");
		return;
	}

	dstr_init(&proc_speed);

	while (getline(&line, &linecap, fp) != -1) {
		if (!strncmp(line, "CPU: ", 5)) {
			char *start = strrchr(line, '(');
			if (!start || *(++start) == '\0')
				continue;

			size_t len = strcspn(start, "-");
			dstr_ncopy(&proc_speed, start, len);
		}
	}

	blog(LOG_INFO, "CPU Speed: %sMHz", proc_speed.array);

	fclose(fp);
	dstr_free(&proc_speed);
	free(line);
#endif
}

static void log_processor_name(void)
{
	int mib[2];
	size_t len;
	char *proc;

	mib[0] = CTL_HW;
	mib[1] = HW_MODEL;

	sysctl(mib, 2, NULL, &len, NULL, 0);
	proc = bmalloc(len);
	if (!proc)
		return;

	sysctl(mib, 2, proc, &len, NULL, 0);
	blog(LOG_INFO, "CPU Name: %s", proc);

	bfree(proc);
}

static void log_processor_info(void)
{
	log_processor_name();
	log_processor_speed();
}
#endif

static void log_memory_info(void)
{
#if defined(__OpenBSD__)
	int mib[2];
	size_t len;
	int64_t mem;

	mib[0] = CTL_HW;
	mib[1] = HW_PHYSMEM64;
	len = sizeof(mem);

	if (sysctl(mib, 2, &mem, &len, NULL, 0) >= 0)
		blog(LOG_INFO, "Physical Memory: %" PRIi64 "MB Total", mem / 1024 / 1024);
#else
	struct sysinfo info;
	if (sysinfo(&info) < 0)
		return;

	blog(LOG_INFO, "Physical Memory: %" PRIu64 "MB Total, %" PRIu64 "MB Free",
	     (uint64_t)info.totalram * info.mem_unit / 1024 / 1024,
	     ((uint64_t)info.freeram + (uint64_t)info.bufferram) * info.mem_unit / 1024 / 1024);
#endif
}

static void log_kernel_version(void)
{
	struct utsname info;
	if (uname(&info) < 0)
		return;

	blog(LOG_INFO, "Kernel Version: %s %s", info.sysname, info.release);
}

#if defined(__linux__) || defined(__FreeBSD__)
static void log_distribution_info(void)
{
	FILE *fp;
	char *line = NULL;
	size_t linecap = 0;
	struct dstr distro;
	struct dstr version;

	fp = fopen("/etc/os-release", "r");
	if (!fp) {
		blog(LOG_INFO, "Distribution: Missing /etc/os-release !");
		return;
	}

	dstr_init_copy(&distro, "Unknown");
	dstr_init_copy(&version, "Unknown");

	while (getline(&line, &linecap, fp) != -1) {
		if (!strncmp(line, "NAME", 4)) {
			char *start = strchr(line, '=');
			if (!start || *(++start) == '\0')
				continue;
			dstr_copy(&distro, start);
			dstr_resize(&distro, distro.len - 1);
		}

		if (!strncmp(line, "VERSION_ID", 10)) {
			char *start = strchr(line, '=');
			if (!start || *(++start) == '\0')
				continue;
			dstr_copy(&version, start);
			dstr_resize(&version, version.len - 1);
		}
	}

	blog(LOG_INFO, "Distribution: %s %s", distro.array, version.array);

	fclose(fp);
	dstr_free(&version);
	dstr_free(&distro);
	free(line);
}

static void log_flatpak_extensions(const char *extensions)
{
	if (!extensions)
		return;

	char **exts_list = strlist_split(extensions, ';', false);
	for (char **ext = exts_list; *ext != NULL; ext++) {
		// Log the extension name without its commit hash
		char **name = strlist_split(*ext, '=', false);
		blog(LOG_INFO, " - %s", *name);
		strlist_free(name);
	}
	strlist_free(exts_list);
}

static void log_flatpak_info(void)
{
	config_t *fp_info = NULL;

	if (config_open(&fp_info, "/.flatpak-info", CONFIG_OPEN_EXISTING) != CONFIG_SUCCESS) {
		blog(LOG_ERROR, "Unable to open .flatpak-info file");
		return;
	}

	const char *branch = config_get_string(fp_info, "Instance", "branch");
	const char *arch = config_get_string(fp_info, "Instance", "arch");
	const char *commit = config_get_string(fp_info, "Instance", "app-commit");

	const char *runtime = config_get_string(fp_info, "Application", "runtime");

	const char *app_exts = config_get_string(fp_info, "Instance", "app-extensions");
	const char *runtime_exts = config_get_string(fp_info, "Instance", "runtime-extensions");

	const char *fp_version = config_get_string(fp_info, "Instance", "flatpak-version");

	blog(LOG_INFO, "Flatpak Branch: %s", branch ? branch : "none");
	blog(LOG_INFO, "Flatpak Arch: %s", arch ? arch : "unknown");
	blog(LOG_INFO, "Flatpak Commit: %s", commit ? commit : "unknown");

	blog(LOG_INFO, "Flatpak Runtime: %s", runtime ? runtime : "none");

	if (app_exts) {
		blog(LOG_INFO, "App Extensions:");
		log_flatpak_extensions(app_exts);
	}

	if (runtime_exts) {
		blog(LOG_INFO, "Runtime Extensions:");
		log_flatpak_extensions(runtime_exts);
	}

	blog(LOG_INFO, "Flatpak Framework Version: %s", fp_version ? fp_version : "unknown");

	config_close(fp_info);
}

static void log_desktop_session_info(void)
{
	char *current_desktop = getenv("XDG_CURRENT_DESKTOP");
	char *session_desktop = getenv("XDG_SESSION_DESKTOP");
	char *session_type = getenv("XDG_SESSION_TYPE");

	if (current_desktop && session_desktop)
		blog(LOG_INFO, "Desktop Environment: %s (%s)", current_desktop, session_desktop);
	else if (current_desktop || session_desktop)
		blog(LOG_INFO, "Desktop Environment: %s", current_desktop ? current_desktop : session_desktop);

	if (session_type)
		blog(LOG_INFO, "Session Type: %s", session_type);
}
#endif

void log_system_info(void)
{
#if defined(__linux__) || defined(__FreeBSD__)
	log_processor_info();
#endif
	log_processor_cores();
	log_memory_info();
	log_kernel_version();
#if defined(__linux__) || defined(__FreeBSD__)
	if (access("/.flatpak-info", F_OK) == 0)
		log_flatpak_info();
	else
		log_distribution_info();

	log_desktop_session_info();
#endif
	if (obs_get_nix_platform() == OBS_NIX_PLATFORM_X11_EGL)
		obs_nix_x11_log_info();
}

bool obs_hotkeys_platform_init(struct obs_core_hotkeys *hotkeys)
{
	switch (obs_get_nix_platform()) {
	case OBS_NIX_PLATFORM_X11_EGL:
		hotkeys_vtable = obs_nix_x11_get_hotkeys_vtable();
		break;
#ifdef ENABLE_WAYLAND
	case OBS_NIX_PLATFORM_WAYLAND:
		hotkeys_vtable = obs_nix_wayland_get_hotkeys_vtable();
		break;
#endif
	default:
		break;
	}

	return hotkeys_vtable->init(hotkeys);
}

void obs_hotkeys_platform_free(struct obs_core_hotkeys *hotkeys)
{
	hotkeys_vtable->free(hotkeys);
	hotkeys_vtable = NULL;
}

bool obs_hotkeys_platform_is_pressed(obs_hotkeys_platform_t *context, obs_key_t key)
{
	return hotkeys_vtable->is_pressed(context, key);
}

void obs_key_to_str(obs_key_t key, struct dstr *dstr)
{
	return hotkeys_vtable->key_to_str(key, dstr);
}

obs_key_t obs_key_from_virtual_key(int sym)
{
	return hotkeys_vtable->key_from_virtual_key(sym);
}

int obs_key_to_virtual_key(obs_key_t key)
{
	return hotkeys_vtable->key_to_virtual_key(key);
}

static inline void add_combo_key(obs_key_t key, struct dstr *str)
{
	struct dstr key_str = {0};

	obs_key_to_str(key, &key_str);

	if (!dstr_is_empty(&key_str)) {
		if (!dstr_is_empty(str)) {
			dstr_cat(str, " + ");
		}
		dstr_cat_dstr(str, &key_str);
	}

	dstr_free(&key_str);
}

void obs_key_combination_to_str(obs_key_combination_t combination, struct dstr *str)
{
	if ((combination.modifiers & INTERACT_CONTROL_KEY) != 0) {
		add_combo_key(OBS_KEY_CONTROL, str);
	}
	if ((combination.modifiers & INTERACT_COMMAND_KEY) != 0) {
		add_combo_key(OBS_KEY_META, str);
	}
	if ((combination.modifiers & INTERACT_ALT_KEY) != 0) {
		add_combo_key(OBS_KEY_ALT, str);
	}
	if ((combination.modifiers & INTERACT_SHIFT_KEY) != 0) {
		add_combo_key(OBS_KEY_SHIFT, str);
	}
	if (combination.key != OBS_KEY_NONE) {
		add_combo_key(combination.key, str);
	}
}
