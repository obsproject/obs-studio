/******************************************************************************
    Copyright (C) 2026 by FiniteSingularity <finitesingularityttv@gmail.com>

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

#import "obs-core-modules.h"

#import <obs-internal.h>
#import <obs.h>
#import <util/dstr.h>
#import <util/platform.h>

#import <Foundation/Foundation.h>

extern bool find_core_module(struct obs_runtime_module_info *info, obs_find_module_callback2_t callback, void *data);

void load_core_modules(obs_find_module_callback2_t callback, void *data)
{
    NSURL *pluginURL = [[NSBundle mainBundle] builtInPlugInsURL];
    NSString *pluginBasePath = [pluginURL path];

    for (unsigned int i = 0; i < obs_core_modules_count; i++) {
        const char *name = obs_core_modules[i];
        NSString *moduleName = [NSString stringWithUTF8String:name];

        NSString *binPath =
            [NSString stringWithFormat:@"%@/%@.plugin/Contents/MacOS/%@", pluginBasePath, moduleName, moduleName];

        NSString *dataPath =
            [NSString stringWithFormat:@"%@/%@.plugin/Contents/Resources/", pluginBasePath, moduleName];

        if (![[NSFileManager defaultManager] fileExistsAtPath:binPath]) {
            blog(LOG_ERROR, "Core Module %s required but missing!", name);
            return;
        }

        struct obs_runtime_module_info module_info = {
            .path_info = {.binary = [binPath UTF8String], .data = [dataPath UTF8String]},
            .type = MODULE_TYPE_CORE,
            .name = name
        };

        if (!find_core_module(&module_info, callback, data)) {
            blog(LOG_ERROR, "Failed to load core module %s", name);
            return;
        }
    }
}
