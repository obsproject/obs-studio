/******************************************************************************
    Copyright (C) 2013 by Ruwen Hahn <palana@stunned.de>

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

#include "util/platform.h"
#include "util/dstr.h"
#include "obs.h"
#include "obs-internal.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/sysctl.h>

// support both foo.so and libfoo.so for now
static const char *plugin_patterns[] = {
	OBS_INSTALL_PREFIX "obs-plugins/%s.so",
	OBS_INSTALL_PREFIX "obs-plugins/lib%s.so",
	"../obs-plugins/%s.so",
	"../obs-plugins/lib%s.so"
};

static const int plugin_patterns_size =
	sizeof(plugin_patterns)/sizeof(plugin_patterns[0]);

char *find_plugin(const char *plugin)
{
	struct dstr path;
	dstr_init(&path);
	for(int i = 0; i < plugin_patterns_size; i++) {
		dstr_printf(&path, plugin_patterns[i], plugin);
		if(!access(path.array, F_OK))
			break;
	}

	return path.array;
}

char *find_libobs_data_file(const char *file)
{
	struct dstr path;
	dstr_init_copy(&path, OBS_INSTALL_DATA_PATH "/libobs/");
	dstr_cat(&path, file);
	return path.array;
}

char *obs_find_plugin_file(const char *file)
{
	struct dstr path;
	dstr_init_copy(&path, OBS_INSTALL_DATA_PATH "/obs-plugins/");
	dstr_cat(&path, file);
	return path.array;
}

static void log_processor_name(void)
{
	char   *name = NULL;
	size_t size;
	int    ret;

	ret = sysctlbyname("machdep.cpu.brand_string", NULL, &size, NULL, 0);
	if (ret != 0)
		return;

	name = malloc(size);

	ret = sysctlbyname("machdep.cpu.brand_string", name, &size, NULL, 0);
	if (ret == 0)
		blog(LOG_INFO, "CPU Name: %s", name);

	free(name);
}

static void log_processor_speed(void)
{
	size_t    size;
	long long freq;
	int       ret;

	size = sizeof(freq);
	ret = sysctlbyname("hw.cpufrequency", &freq, &size, NULL, 0);
	if (ret == 0)
		blog(LOG_INFO, "CPU Speed: %lldMHz", freq / 1000000);
}

static void log_processor_cores(void)
{
	size_t size;
	int    physical_cores = 0, logical_cores = 0;
	int    ret;

	size = sizeof(physical_cores);
	ret = sysctlbyname("machdep.cpu.core_count", &physical_cores,
			&size, NULL, 0);
	if (ret != 0)
		return;

	ret = sysctlbyname("machdep.cpu.thread_count", &logical_cores,
			&size, NULL, 0);
	if (ret != 0)
		return;

	blog(LOG_INFO, "Physical Cores: %d, Logical Cores: %d",
			physical_cores, logical_cores);
}

static void log_available_memory(void)
{
	size_t    size;
	long long memory_available;
	int       ret;

	size = sizeof(memory_available);
	ret = sysctlbyname("hw.memsize", &memory_available, &size, NULL, 0);
	if (ret == 0)
		blog(LOG_INFO, "Physical Memory: %lldMB Total",
				memory_available / 1024 / 1024);
}

static void log_kernel_version(void)
{
	char   kernel_version[1024];
	size_t size = sizeof(kernel_version);
	int    ret;

	ret = sysctlbyname("kern.osrelease", kernel_version, &size,
			NULL, 0);
	if (ret == 0)
		blog(LOG_INFO, "Kernel Version: %s", kernel_version);
}

void log_system_info(void)
{
	log_processor_name();
	log_processor_speed();
	log_processor_cores();
	log_available_memory();
	log_kernel_version();
}
