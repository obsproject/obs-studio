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
#pragma once
#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>

#ifdef __OBJC__
#import <AVFoundation/AVFoundation.h>
#else
typedef void AVAudioUnitComponentManager;
typedef void AVAudioUnitComponent;
typedef void NSArray;
typedef void NSPredicate;
typedef void NSString;
#endif

struct au_descriptor {
    AudioComponent component;
    AudioComponentDescription desc;
    char name[64];
    char vendor[64];
    char version[64];
    bool is_v3;
    OSType type;
    OSType subtype;
    OSType manufacturer;
    char uid[32];
};

struct au_list {
    struct au_descriptor *items;
    int count;
};

#ifdef __cplusplus
extern "C" {
#endif

    struct au_list au_scan_all_effects(void);
    void au_free_list(struct au_list *list);
    const struct au_descriptor *au_find_by_uid(const char *uid);

#ifdef __cplusplus
}
#endif
