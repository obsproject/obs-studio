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
#include "mac-au-scan.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __OBJC__
#import <AVFoundation/AVFoundation.h>
#import <CoreAudioKit/CoreAudioKit.h>
#import <Cocoa/Cocoa.h>
#else
typedef void AVAudioUnit;
typedef void AUAudioUnit;
typedef void AUViewControllerBase;
typedef void NSWindow;
#endif

struct au_data;

struct au_plugin {
    char uid[32];
    char name[64];
    NSString *title;
    char vendor[64];

    OSType type;
    OSType subtype;
    OSType manufacturer;
    bool is_v3;

    AVAudioUnit *av_unit;
    AUAudioUnit *au;
    void *objc;

    bool has_view;
    bool editor_is_visible;

    uint32_t max_frames;
    double sample_rate;
    uint32_t channels;
    double sample_time;

    int num_in_audio_buses;
    int num_out_audio_buses;
    int num_enabled_in_audio_buses;
    int num_enabled_out_audio_buses;
    int sidechain_num_channels;
};

#ifdef __cplusplus
extern "C" {
#endif

    struct au_plugin *au_plugin_create(const struct au_descriptor *desc, double sr, uint32_t frames, uint32_t channels);
    void au_plugin_destroy(struct au_plugin *p);
    void au_plugin_process(struct au_plugin *p, float *const *channels, float *const *sc_channels,
                           bool sidechain_enabled, uint32_t num_frames, uint32_t num_channels);

    CFDataRef au_plugin_save_state(struct au_plugin *p);
    void au_plugin_load_state(struct au_plugin *p, CFDataRef data);

    void au_plugin_show_editor(struct au_plugin *p);
    void au_plugin_hide_editor(struct au_plugin *p);

#ifdef __cplusplus
}
#endif
