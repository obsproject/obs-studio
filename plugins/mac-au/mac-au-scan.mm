/*****************************************************************************
Copyright (C) 2025 by pkv@obsproject.com

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
*****************************************************************************/
#import "mac-au-scan.h"
#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreFoundation/CoreFoundation.h>
#import <stdlib.h>
#import <string.h>

static void get_name(AudioComponent comp, char out[256])
{
    CFStringRef cfName = NULL;

    if (AudioComponentCopyName(comp, &cfName) != noErr || !cfName) {
        out[0] = 0;
        return;
    }

    Boolean ok = CFStringGetCString(cfName, out, 256, kCFStringEncodingUTF8);
    if (!ok)
        out[0] = 0;

    CFRelease(cfName);
}

static int au_name_compare(const void *a, const void *b)
{
    const struct au_descriptor *da = (const struct au_descriptor *) a;
    const struct au_descriptor *db = (const struct au_descriptor *) b;

    return strcasecmp(da->name, db->name);
}

struct au_list au_scan_all_effects(void)
{
    struct au_list result = {0};

    AVAudioUnitComponentManager *manager = [AVAudioUnitComponentManager sharedAudioUnitComponentManager];
    NSPredicate *predicate = [NSPredicate predicateWithFormat:@"typeName CONTAINS %@", @"Effect"];
    NSArray<AVAudioUnitComponent *> *components = [manager componentsMatchingPredicate:predicate];

    for (AVAudioUnitComponent *comp in components) {
        AudioComponentDescription desc = comp.audioComponentDescription;

        desc.componentFlags = 0;
        desc.componentFlagsMask = 0;

        AudioComponent c = AudioComponentFindNext(NULL, &desc);
        if (!c)
            continue;

        AudioComponentDescription realDesc;
        OSStatus err = AudioComponentGetDescription(c, &realDesc);
        if (err != noErr)
            continue;

        result.items =
            (struct au_descriptor *) realloc(result.items, sizeof(struct au_descriptor) * (result.count + 1));
        struct au_descriptor *d = &result.items[result.count];

        d->component = c;
        d->desc = realDesc;
        d->is_v3 = (realDesc.componentFlags & kAudioComponentFlag_IsV3AudioUnit) != 0;
        strlcpy(d->name, [[comp name] UTF8String], sizeof(d->name));
        strlcpy(d->vendor, [[comp manufacturerName] UTF8String], sizeof(d->vendor));
        strlcpy(d->version, [[comp versionString] UTF8String], sizeof(d->version));
        d->type = d->desc.componentType;
        d->subtype = d->desc.componentSubType;
        d->manufacturer = d->desc.componentManufacturer;
        snprintf(d->uid, sizeof(d->uid), "%08X-%08X-%08X", (unsigned) d->type, (unsigned) d->subtype,
                 (unsigned) d->manufacturer);

        result.count++;
    }

    if (result.count > 1)
        qsort(result.items, result.count, sizeof(struct au_descriptor), au_name_compare);

    return result;
}

void au_free_list(struct au_list *list)
{
    if (!list || !list->items)
        return;

    free(list->items);
    list->items = NULL;
    list->count = 0;
}

extern struct au_list g_au_list;
const struct au_descriptor *au_find_by_uid(const char *uid)
{
    if (!uid || !g_au_list.items)
        return NULL;

    for (int i = 0; i < g_au_list.count; i++) {
        const struct au_descriptor *d = &g_au_list.items[i];
        if (strcmp(d->uid, uid) == 0)
            return d;
    }

    return NULL;
}
