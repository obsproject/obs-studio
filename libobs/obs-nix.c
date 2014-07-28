/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>
    Copyright (C) 2014 by Zachary Lund <admin@computerquip.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include "util/dstr.h"
#include "obs-internal.h"

const char *get_module_extension(void)
{
	return ".so";
}

#ifdef __LP64__
#define BIT_STRING "64bit"
#else
#define BIT_STRING "32bit"
#endif

static const char *module_bin[] = {
	"../../obs-plugins/" BIT_STRING,
	OBS_INSTALL_PREFIX "lib/obs-plugins",
};

static const char *module_data[] = {
	OBS_DATA_PATH "/obs-plugins/%module%",
	OBS_INSTALL_DATA_PATH "/obs-plugins/%module%",
};

static const int module_patterns_size =
	sizeof(module_bin)/sizeof(module_bin[0]);

void add_default_module_paths(void)
{
	for (int i = 0; i < module_patterns_size; i++)
		obs_add_module_path(module_bin[i], module_data[i]);
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

	if (OBS_INSTALL_PREFIX [0] != 0) {
		if (check_path(file, OBS_INSTALL_DATA_PATH "/libobs/",
					&output))
			return output.array;
	}

	dstr_free(&output);
	return NULL;
}

static void log_processor_info(void)
{
	FILE *fp;
	int physical_id = -1;
	int last_physical_id = -1;
	char *line = NULL;
	size_t linecap = 0;
	struct dstr processor;

	blog(LOG_INFO, "Processor: %lu logical cores",
	     sysconf(_SC_NPROCESSORS_ONLN));

	fp = fopen("/proc/cpuinfo", "r");
	if (!fp)
		return;

	dstr_init(&processor);

	while (getline(&line, &linecap, fp) != -1) {
		if (!strncmp(line, "model name", 10)) {
			char *start = strchr(line, ':');
			if (!start || *(++start) == '\0')
				continue;
			dstr_copy(&processor, start);
			dstr_resize(&processor, processor.len - 1);
			dstr_depad(&processor);
		}

		if (!strncmp(line, "physical id", 11)) {
			char *start = strchr(line, ':');
			if (!start || *(++start) == '\0')
				continue;
			physical_id = atoi(start);
		}

		if (*line == '\n' && physical_id != last_physical_id) {
			last_physical_id = physical_id;
			blog(LOG_INFO, "Processor: %s", processor.array);
		}
	}

	fclose(fp);
	dstr_free(&processor);
	free(line);
}

static void log_memory_info(void)
{
	struct sysinfo info;
	if (sysinfo(&info) < 0)
		return;

	blog(LOG_INFO, "Physical Memory: %luMB Total",
		info.totalram / 1024 / 1024);
}

static void log_kernel_version(void)
{
	struct utsname info;
	if (uname(&info) < 0)
		return;

	blog(LOG_INFO, "Kernel Version: %s %s", info.sysname, info.release);
}

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

void log_system_info(void)
{
	log_processor_info();
	log_memory_info();
	log_kernel_version();
	log_distribution_info();
}
