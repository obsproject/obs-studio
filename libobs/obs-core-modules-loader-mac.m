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
