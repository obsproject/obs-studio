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

#include "util/dstr.h"
#include "obs.h"
#include "obs-internal.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/sysctl.h>

#include <Carbon/Carbon.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDManager.h>

#import <AppKit/AppKit.h>

// MARK: macOS Bundle Management
const char *get_module_extension(void)
{
    return "";
}

void add_default_module_paths(void)
{
    NSURL *pluginURL = [[NSBundle mainBundle] builtInPlugInsURL];
    NSString *pluginModulePath = [[pluginURL path] stringByAppendingString:@"/%module%.plugin/Contents/MacOS/"];
    NSString *pluginDataPath = [[pluginURL path] stringByAppendingString:@"/%module%.plugin/Contents/Resources/"];

    obs_add_module_path(pluginModulePath.UTF8String, pluginDataPath.UTF8String);
}

char *find_libobs_data_file(const char *file)
{
    NSBundle *frameworkBundle = [NSBundle bundleWithIdentifier:@"com.obsproject.libobs"];
    NSString *libobsDataPath =
        [[[frameworkBundle bundleURL] path] stringByAppendingFormat:@"/%@/%s", @"Resources", file];
    size_t path_length = strlen(libobsDataPath.UTF8String);

    char *path = bmalloc(path_length + 1);
    snprintf(path, (path_length + 1), "%s", libobsDataPath.UTF8String);

    return path;
}

// MARK: - macOS Hardware Info Helpers

static void log_processor_name(void)
{
    char *name = NULL;
    size_t size;
    int ret;

    ret = sysctlbyname("machdep.cpu.brand_string", NULL, &size, NULL, 0);
    if (ret != 0)
        return;

    name = bmalloc(size);

    ret = sysctlbyname("machdep.cpu.brand_string", name, &size, NULL, 0);
    if (ret == 0)
        blog(LOG_INFO, "CPU Name: %s", name);

    bfree(name);
}

static void log_processor_speed(void)
{
    size_t size;
    long long freq;
    int ret;

    size = sizeof(freq);
    ret = sysctlbyname("hw.cpufrequency", &freq, &size, NULL, 0);
    if (ret == 0)
        blog(LOG_INFO, "CPU Speed: %lldMHz", freq / 1000000);
}

static void log_model_name(void)
{
    char *name = NULL;
    size_t size;
    int ret;

    ret = sysctlbyname("hw.model", NULL, &size, NULL, 0);
    if (ret != 0)
        return;

    name = bmalloc(size);

    ret = sysctlbyname("hw.model", name, &size, NULL, 0);
    if (ret == 0)
        blog(LOG_INFO, "Model Identifier: %s", name);

    bfree(name);
}

static void log_processor_cores(void)
{
    blog(LOG_INFO, "Physical Cores: %d, Logical Cores: %d", os_get_physical_cores(), os_get_logical_cores());
}

static void log_emulation_status(void)
{
    blog(LOG_INFO, "Rosetta translation used: %s", os_get_emulation_status() ? "true" : "false");
}

static void log_available_memory(void)
{
    size_t size;
    long long memory_available;
    int ret;

    size = sizeof(memory_available);
    ret = sysctlbyname("hw.memsize", &memory_available, &size, NULL, 0);
    if (ret == 0)
        blog(LOG_INFO, "Physical Memory: %lldMB Total", memory_available / 1024 / 1024);
}

static void log_os(void)
{
    NSProcessInfo *pi = [NSProcessInfo processInfo];
    blog(LOG_INFO, "OS Name: macOS");
    blog(LOG_INFO, "OS Version: %s", [[pi operatingSystemVersionString] UTF8String]);
}

static void log_kernel_version(void)
{
    char kernel_version[1024];
    size_t size = sizeof(kernel_version);
    int ret;

    ret = sysctlbyname("kern.osrelease", kernel_version, &size, NULL, 0);
    if (ret == 0)
        blog(LOG_INFO, "Kernel Version: %s", kernel_version);
}

void log_system_info(void)
{
    log_processor_name();
    log_processor_speed();
    log_processor_cores();
    log_available_memory();
    log_model_name();
    log_os();
    log_emulation_status();
    log_kernel_version();
}

// MARK: - Type Conversion Utilities

static bool dstr_from_cfstring(struct dstr *str, CFStringRef ref)
{
    CFIndex length = CFStringGetLength(ref);
    CFIndex max_size = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
    assert(max_size > 0);

    dstr_reserve(str, max_size);

    if (!CFStringGetCString(ref, str->array, max_size, kCFStringEncodingUTF8))
        return false;

    str->len = strlen(str->array);
    return true;
}

// MARK: - Graphics Thread Wrappers

void *obs_graphics_thread_autorelease(void *param)
{
    @autoreleasepool {
        return obs_graphics_thread(param);
    }
}

bool obs_graphics_thread_loop_autorelease(struct obs_graphics_context *context)
{
    @autoreleasepool {
        return obs_graphics_thread_loop(context);
    }
}

// MARK: - macOS Hotkey Management

typedef struct obs_key_code {
    int code;
    bool is_valid;
} obs_key_code_t;

typedef struct macOS_glyph_desc {
    UniChar glyph;
    char *desc;
    bool is_glyph;
    bool is_valid;
} macOS_glyph_desc_t;

typedef struct obs_key_desc {
    char *desc;
    bool is_valid;
} obs_key_desc_t;

static int INVALID_KEY = 0xFF;

/* clang-format off */
static const obs_key_code_t virtual_keys[OBS_KEY_LAST_VALUE] = {
    [OBS_KEY_A]            = {.code = kVK_ANSI_A,              .is_valid = true},
    [OBS_KEY_B]            = {.code = kVK_ANSI_B,              .is_valid = true},
    [OBS_KEY_C]            = {.code = kVK_ANSI_C,              .is_valid = true},
    [OBS_KEY_D]            = {.code = kVK_ANSI_D,              .is_valid = true},
    [OBS_KEY_E]            = {.code = kVK_ANSI_E,              .is_valid = true},
    [OBS_KEY_F]            = {.code = kVK_ANSI_F,              .is_valid = true},
    [OBS_KEY_G]            = {.code = kVK_ANSI_G,              .is_valid = true},
    [OBS_KEY_H]            = {.code = kVK_ANSI_H,              .is_valid = true},
    [OBS_KEY_I]            = {.code = kVK_ANSI_I,              .is_valid = true},
    [OBS_KEY_J]            = {.code = kVK_ANSI_J,              .is_valid = true},
    [OBS_KEY_K]            = {.code = kVK_ANSI_K,              .is_valid = true},
    [OBS_KEY_L]            = {.code = kVK_ANSI_L,              .is_valid = true},
    [OBS_KEY_M]            = {.code = kVK_ANSI_M,              .is_valid = true},
    [OBS_KEY_N]            = {.code = kVK_ANSI_N,              .is_valid = true},
    [OBS_KEY_O]            = {.code = kVK_ANSI_O,              .is_valid = true},
    [OBS_KEY_P]            = {.code = kVK_ANSI_P,              .is_valid = true},
    [OBS_KEY_Q]            = {.code = kVK_ANSI_Q,              .is_valid = true},
    [OBS_KEY_R]            = {.code = kVK_ANSI_R,              .is_valid = true},
    [OBS_KEY_S]            = {.code = kVK_ANSI_S,              .is_valid = true},
    [OBS_KEY_T]            = {.code = kVK_ANSI_T,              .is_valid = true},
    [OBS_KEY_U]            = {.code = kVK_ANSI_U,              .is_valid = true},
    [OBS_KEY_V]            = {.code = kVK_ANSI_V,              .is_valid = true},
    [OBS_KEY_W]            = {.code = kVK_ANSI_W,              .is_valid = true},
    [OBS_KEY_X]            = {.code = kVK_ANSI_X,              .is_valid = true},
    [OBS_KEY_Y]            = {.code = kVK_ANSI_Y,              .is_valid = true},
    [OBS_KEY_Z]            = {.code = kVK_ANSI_Z,              .is_valid = true},
    [OBS_KEY_1]            = {.code = kVK_ANSI_1,              .is_valid = true},
    [OBS_KEY_2]            = {.code = kVK_ANSI_2,              .is_valid = true},
    [OBS_KEY_3]            = {.code = kVK_ANSI_3,              .is_valid = true},
    [OBS_KEY_4]            = {.code = kVK_ANSI_4,              .is_valid = true},
    [OBS_KEY_5]            = {.code = kVK_ANSI_5,              .is_valid = true},
    [OBS_KEY_6]            = {.code = kVK_ANSI_6,              .is_valid = true},
    [OBS_KEY_7]            = {.code = kVK_ANSI_7,              .is_valid = true},
    [OBS_KEY_8]            = {.code = kVK_ANSI_8,              .is_valid = true},
    [OBS_KEY_9]            = {.code = kVK_ANSI_9,              .is_valid = true},
    [OBS_KEY_0]            = {.code = kVK_ANSI_0,              .is_valid = true},
    [OBS_KEY_RETURN]       = {.code = kVK_Return,              .is_valid = true},
    [OBS_KEY_ESCAPE]       = {.code = kVK_Escape,              .is_valid = true},
    [OBS_KEY_BACKSPACE]    = {.code = kVK_Delete,              .is_valid = true},
    [OBS_KEY_TAB]          = {.code = kVK_Tab,                 .is_valid = true},
    [OBS_KEY_SPACE]        = {.code = kVK_Space,               .is_valid = true},
    [OBS_KEY_MINUS]        = {.code = kVK_ANSI_Minus,          .is_valid = true},
    [OBS_KEY_EQUAL]        = {.code = kVK_ANSI_Equal,          .is_valid = true},
    [OBS_KEY_BRACKETLEFT]  = {.code = kVK_ANSI_LeftBracket,    .is_valid = true},
    [OBS_KEY_BRACKETRIGHT] = {.code = kVK_ANSI_RightBracket,   .is_valid = true},
    [OBS_KEY_BACKSLASH]    = {.code = kVK_ANSI_Backslash,      .is_valid = true},
    [OBS_KEY_SEMICOLON]    = {.code = kVK_ANSI_Semicolon,      .is_valid = true},
    [OBS_KEY_QUOTE]        = {.code = kVK_ANSI_Quote,          .is_valid = true},
    [OBS_KEY_DEAD_GRAVE]   = {.code = kVK_ANSI_Grave,          .is_valid = true},
    [OBS_KEY_COMMA]        = {.code = kVK_ANSI_Comma,          .is_valid = true},
    [OBS_KEY_PERIOD]       = {.code = kVK_ANSI_Period,         .is_valid = true},
    [OBS_KEY_SLASH]        = {.code = kVK_ANSI_Slash,          .is_valid = true},
    [OBS_KEY_CAPSLOCK]     = {.code = kVK_CapsLock,            .is_valid = true},
    [OBS_KEY_SECTION]      = {.code = kVK_ISO_Section,         .is_valid = true},
    [OBS_KEY_F1]           = {.code = kVK_F1,                  .is_valid = true},
    [OBS_KEY_F2]           = {.code = kVK_F2,                  .is_valid = true},
    [OBS_KEY_F3]           = {.code = kVK_F3,                  .is_valid = true},
    [OBS_KEY_F4]           = {.code = kVK_F4,                  .is_valid = true},
    [OBS_KEY_F5]           = {.code = kVK_F5,                  .is_valid = true},
    [OBS_KEY_F6]           = {.code = kVK_F6,                  .is_valid = true},
    [OBS_KEY_F7]           = {.code = kVK_F7,                  .is_valid = true},
    [OBS_KEY_F8]           = {.code = kVK_F8,                  .is_valid = true},
    [OBS_KEY_F9]           = {.code = kVK_F9,                  .is_valid = true},
    [OBS_KEY_F10]          = {.code = kVK_F10,                 .is_valid = true},
    [OBS_KEY_F11]          = {.code = kVK_F11,                 .is_valid = true},
    [OBS_KEY_F12]          = {.code = kVK_F12,                 .is_valid = true},
    [OBS_KEY_HELP]         = {.code = kVK_Help,                .is_valid = true},
    [OBS_KEY_HOME]         = {.code = kVK_Home,                .is_valid = true},
    [OBS_KEY_PAGEUP]       = {.code = kVK_PageUp,              .is_valid = true},
    [OBS_KEY_DELETE]       = {.code = kVK_ForwardDelete,       .is_valid = true},
    [OBS_KEY_END]          = {.code = kVK_End,                 .is_valid = true},
    [OBS_KEY_PAGEDOWN]     = {.code = kVK_PageDown,            .is_valid = true},
    [OBS_KEY_RIGHT]        = {.code = kVK_RightArrow,          .is_valid = true},
    [OBS_KEY_LEFT]         = {.code = kVK_LeftArrow,           .is_valid = true},
    [OBS_KEY_DOWN]         = {.code = kVK_DownArrow,           .is_valid = true},
    [OBS_KEY_UP]           = {.code = kVK_UpArrow,             .is_valid = true},
    [OBS_KEY_CLEAR]        = {.code = kVK_ANSI_KeypadClear,    .is_valid = true},
    [OBS_KEY_NUMSLASH]     = {.code = kVK_ANSI_KeypadDivide,   .is_valid = true},
    [OBS_KEY_NUMASTERISK]  = {.code = kVK_ANSI_KeypadMultiply, .is_valid = true},
    [OBS_KEY_NUMMINUS]     = {.code = kVK_ANSI_KeypadMinus,    .is_valid = true},
    [OBS_KEY_NUMPLUS]      = {.code = kVK_ANSI_KeypadPlus,     .is_valid = true},
    [OBS_KEY_ENTER]        = {.code = kVK_ANSI_KeypadEnter,    .is_valid = true},
    [OBS_KEY_NUM1]         = {.code = kVK_ANSI_Keypad1,        .is_valid = true},
    [OBS_KEY_NUM2]         = {.code = kVK_ANSI_Keypad2,        .is_valid = true},
    [OBS_KEY_NUM3]         = {.code = kVK_ANSI_Keypad3,        .is_valid = true},
    [OBS_KEY_NUM4]         = {.code = kVK_ANSI_Keypad4,        .is_valid = true},
    [OBS_KEY_NUM5]         = {.code = kVK_ANSI_Keypad5,        .is_valid = true},
    [OBS_KEY_NUM6]         = {.code = kVK_ANSI_Keypad6,        .is_valid = true},
    [OBS_KEY_NUM7]         = {.code = kVK_ANSI_Keypad7,        .is_valid = true},
    [OBS_KEY_NUM8]         = {.code = kVK_ANSI_Keypad8,        .is_valid = true},
    [OBS_KEY_NUM9]         = {.code = kVK_ANSI_Keypad9,        .is_valid = true},
    [OBS_KEY_NUM0]         = {.code = kVK_ANSI_Keypad0,        .is_valid = true},
    [OBS_KEY_NUMPERIOD]    = {.code = kVK_ANSI_KeypadDecimal,  .is_valid = true},
    [OBS_KEY_NUMEQUAL]     = {.code = kVK_ANSI_KeypadEquals,   .is_valid = true},
    [OBS_KEY_F13]          = {.code = kVK_F13,                 .is_valid = true},
    [OBS_KEY_F14]          = {.code = kVK_F14,                 .is_valid = true},
    [OBS_KEY_F15]          = {.code = kVK_F15,                 .is_valid = true},
    [OBS_KEY_F16]          = {.code = kVK_F16,                 .is_valid = true},
    [OBS_KEY_F17]          = {.code = kVK_F17,                 .is_valid = true},
    [OBS_KEY_F18]          = {.code = kVK_F18,                 .is_valid = true},
    [OBS_KEY_F19]          = {.code = kVK_F19,                 .is_valid = true},
    [OBS_KEY_F20]          = {.code = kVK_F20,                 .is_valid = true},
    [OBS_KEY_CONTROL]      = {.code = kVK_Control,             .is_valid = true},
    [OBS_KEY_SHIFT]        = {.code = kVK_Shift,               .is_valid = true},
    [OBS_KEY_ALT]          = {.code = kVK_Option,              .is_valid = true},
    [OBS_KEY_META]         = {.code = kVK_Command,             .is_valid = true},
};

static const obs_key_desc_t key_descriptions[OBS_KEY_LAST_VALUE] = {
    [OBS_KEY_SPACE]       = {.desc = "Space",      .is_valid = true},
    [OBS_KEY_NUMEQUAL]    = {.desc = "= (Keypad)", .is_valid = true},
    [OBS_KEY_NUMASTERISK] = {.desc = "* (Keypad)", .is_valid = true},
    [OBS_KEY_NUMPLUS]     = {.desc = "+ (Keypad)", .is_valid = true},
    [OBS_KEY_NUMMINUS]    = {.desc = "- (Keypad)", .is_valid = true},
    [OBS_KEY_NUMPERIOD]   = {.desc = ". (Keypad)", .is_valid = true},
    [OBS_KEY_NUMSLASH]    = {.desc = "/ (Keypad)", .is_valid = true},
    [OBS_KEY_NUM0]        = {.desc = "0 (Keypad)", .is_valid = true},
    [OBS_KEY_NUM1]        = {.desc = "1 (Keypad)", .is_valid = true},
    [OBS_KEY_NUM2]        = {.desc = "2 (Keypad)", .is_valid = true},
    [OBS_KEY_NUM3]        = {.desc = "3 (Keypad)", .is_valid = true},
    [OBS_KEY_NUM4]        = {.desc = "4 (Keypad)", .is_valid = true},
    [OBS_KEY_NUM5]        = {.desc = "5 (Keypad)", .is_valid = true},
    [OBS_KEY_NUM6]        = {.desc = "6 (Keypad)", .is_valid = true},
    [OBS_KEY_NUM7]        = {.desc = "7 (Keypad)", .is_valid = true},
    [OBS_KEY_NUM8]        = {.desc = "8 (Keypad)", .is_valid = true},
    [OBS_KEY_NUM9]        = {.desc = "9 (Keypad)", .is_valid = true},
    [OBS_KEY_MOUSE1]      = {.desc = "Mouse 1",    .is_valid = true},
    [OBS_KEY_MOUSE2]      = {.desc = "Mouse 2",    .is_valid = true},
    [OBS_KEY_MOUSE3]      = {.desc = "Mouse 3",    .is_valid = true},
    [OBS_KEY_MOUSE4]      = {.desc = "Mouse 4",    .is_valid = true},
    [OBS_KEY_MOUSE5]      = {.desc = "Mouse 5",    .is_valid = true},
    [OBS_KEY_MOUSE6]      = {.desc = "Mouse 6",    .is_valid = true},
    [OBS_KEY_MOUSE7]      = {.desc = "Mouse 7",    .is_valid = true},
    [OBS_KEY_MOUSE8]      = {.desc = "Mouse 8",    .is_valid = true},
    [OBS_KEY_MOUSE9]      = {.desc = "Mouse 9",    .is_valid = true},
    [OBS_KEY_MOUSE10]     = {.desc = "Mouse 10",   .is_valid = true},
    [OBS_KEY_MOUSE11]     = {.desc = "Mouse 11",   .is_valid = true},
    [OBS_KEY_MOUSE12]     = {.desc = "Mouse 12",   .is_valid = true},
    [OBS_KEY_MOUSE13]     = {.desc = "Mouse 13",   .is_valid = true},
    [OBS_KEY_MOUSE14]     = {.desc = "Mouse 14",   .is_valid = true},
    [OBS_KEY_MOUSE15]     = {.desc = "Mouse 15",   .is_valid = true},
    [OBS_KEY_MOUSE16]     = {.desc = "Mouse 16",   .is_valid = true},
    [OBS_KEY_MOUSE17]     = {.desc = "Mouse 17",   .is_valid = true},
    [OBS_KEY_MOUSE18]     = {.desc = "Mouse 18",   .is_valid = true},
    [OBS_KEY_MOUSE19]     = {.desc = "Mouse 19",   .is_valid = true},
    [OBS_KEY_MOUSE20]     = {.desc = "Mouse 20",   .is_valid = true},
    [OBS_KEY_MOUSE21]     = {.desc = "Mouse 21",   .is_valid = true},
    [OBS_KEY_MOUSE22]     = {.desc = "Mouse 22",   .is_valid = true},
    [OBS_KEY_MOUSE23]     = {.desc = "Mouse 23",   .is_valid = true},
    [OBS_KEY_MOUSE24]     = {.desc = "Mouse 24",   .is_valid = true},
    [OBS_KEY_MOUSE25]     = {.desc = "Mouse 25",   .is_valid = true},
    [OBS_KEY_MOUSE26]     = {.desc = "Mouse 26",   .is_valid = true},
    [OBS_KEY_MOUSE27]     = {.desc = "Mouse 27",   .is_valid = true},
    [OBS_KEY_MOUSE28]     = {.desc = "Mouse 28",   .is_valid = true},
    [OBS_KEY_MOUSE29]     = {.desc = "Mouse 29",   .is_valid = true},
};

static const macOS_glyph_desc_t key_glyphs[(keyCodeMask >> 8)] = {
    [kVK_Return]           = {.glyph = 0x21A9, .is_glyph = true, .is_valid = true},
    [kVK_Escape]           = {.glyph = 0x238B, .is_glyph = true, .is_valid = true},
    [kVK_Delete]           = {.glyph = 0x232B, .is_glyph = true, .is_valid = true},
    [kVK_Tab]              = {.glyph = 0x21e5, .is_glyph = true, .is_valid = true},
    [kVK_CapsLock]         = {.glyph = 0x21EA, .is_glyph = true, .is_valid = true},
    [kVK_ANSI_KeypadClear] = {.glyph = 0x2327, .is_glyph = true, .is_valid = true},
    [kVK_ANSI_KeypadEnter] = {.glyph = 0x2305, .is_glyph = true, .is_valid = true},
    [kVK_Help]             = {.glyph = 0x003F, .is_glyph = true, .is_valid = true},
    [kVK_Home]             = {.glyph = 0x2196, .is_glyph = true, .is_valid = true},
    [kVK_PageUp]           = {.glyph = 0x21de, .is_glyph = true, .is_valid = true},
    [kVK_ForwardDelete]    = {.glyph = 0x2326, .is_glyph = true, .is_valid = true},
    [kVK_End]              = {.glyph = 0x2198, .is_glyph = true, .is_valid = true},
    [kVK_PageDown]         = {.glyph = 0x21df, .is_glyph = true, .is_valid = true},
    [kVK_Control]          = {.glyph = kControlUnicode, .is_glyph = true, .is_valid = true},
    [kVK_Shift]            = {.glyph = kShiftUnicode, .is_glyph = true, .is_valid = true},
    [kVK_Option]           = {.glyph = kOptionUnicode, .is_glyph = true, .is_valid = true},
    [kVK_Command]          = {.glyph = kCommandUnicode, .is_glyph = true, .is_valid = true},
    [kVK_RightControl]     = {.glyph = kControlUnicode, .is_glyph = true, .is_valid = true},
    [kVK_RightShift]       = {.glyph = kShiftUnicode, .is_glyph = true, .is_valid = true},
    [kVK_RightOption]      = {.glyph = kOptionUnicode, .is_glyph = true, .is_valid = true},
    [kVK_F1]               = {.desc = "F1", .is_valid = true},
    [kVK_F2]               = {.desc = "F2", .is_valid = true},
    [kVK_F3]               = {.desc = "F3", .is_valid = true},
    [kVK_F4]               = {.desc = "F4", .is_valid = true},
    [kVK_F5]               = {.desc = "F5", .is_valid = true},
    [kVK_F6]               = {.desc = "F6", .is_valid = true},
    [kVK_F7]               = {.desc = "F7", .is_valid = true},
    [kVK_F8]               = {.desc = "F8", .is_valid = true},
    [kVK_F9]               = {.desc = "F9", .is_valid = true},
    [kVK_F10]              = {.desc = "F10", .is_valid = true},
    [kVK_F11]              = {.desc = "F11", .is_valid = true},
    [kVK_F12]              = {.desc = "F12", .is_valid = true},
    [kVK_F13]              = {.desc = "F13", .is_valid = true},
    [kVK_F14]              = {.desc = "F14", .is_valid = true},
    [kVK_F15]              = {.desc = "F15", .is_valid = true},
    [kVK_F16]              = {.desc = "F16", .is_valid = true},
    [kVK_F17]              = {.desc = "F17", .is_valid = true},
    [kVK_F18]              = {.desc = "F18", .is_valid = true},
    [kVK_F19]              = {.desc = "F19", .is_valid = true},
    [kVK_F20]              = {.desc = "F20", .is_valid = true}
};

/* clang-format on */

struct obs_hotkeys_platform {
    volatile long refs;
    CFMachPortRef eventTap;
    bool is_key_down[OBS_KEY_LAST_VALUE];
    TISInputSourceRef tis;
    CFDataRef layout_data;
    UCKeyboardLayout *layout;
};

// MARK: macOS Hotkey Implementation
#define OBS_COCOA_MODIFIER_SIZE (int) 7

static char string_control[OBS_COCOA_MODIFIER_SIZE];
static char string_option[OBS_COCOA_MODIFIER_SIZE];
static char string_shift[OBS_COCOA_MODIFIER_SIZE];
static char string_command[OBS_COCOA_MODIFIER_SIZE];
static dispatch_once_t onceToken;

static void hotkeys_retain(obs_hotkeys_platform_t *platform)
{
    os_atomic_inc_long(&platform->refs);
}

static void hotkeys_release(obs_hotkeys_platform_t *platform)
{
    if (os_atomic_dec_long(&platform->refs) == -1) {
        if (platform->tis) {
            CFRelease(platform->tis);
            platform->tis = NULL;
        }

        if (platform->layout_data) {
            CFRelease(platform->layout_data);
            platform->layout_data = NULL;
        }

        if (platform->eventTap) {
            CGEventTapEnable(platform->eventTap, false);
            CFRelease(platform->eventTap);
            platform->eventTap = NULL;
        }

        bfree(platform);
    }
}

static bool obs_key_to_localized_string(obs_key_t key, struct dstr *str)
{
    if (key < OBS_KEY_LAST_VALUE && !key_descriptions[key].is_valid) {
        return false;
    }

    dstr_copy(str, obs_get_hotkey_translation(key, key_descriptions[key].desc));
    return true;
}

static bool key_code_to_string(int code, struct dstr *str)
{
    if (code < INVALID_KEY) {
        macOS_glyph_desc_t glyph = key_glyphs[code];

        if (glyph.is_valid && glyph.is_glyph && glyph.glyph > 0) {
            dstr_from_wcs(str, (wchar_t[]) {glyph.glyph, 0});
        } else if (glyph.is_valid && glyph.desc) {
            dstr_copy(str, glyph.desc);
        } else {
            return false;
        }
    }

    return true;
}

static bool log_layout_name(TISInputSourceRef tis)
{
    struct dstr layout_name = {0};
    CFStringRef sid = (CFStringRef) TISGetInputSourceProperty(tis, kTISPropertyInputSourceID);

    if (!sid) {
        blog(LOG_ERROR, "hotkeys-cocoa: Unable to get input source ID");
        return false;
    }

    if (!dstr_from_cfstring(&layout_name, sid)) {
        blog(LOG_ERROR, "hotkeys-cocoa: Unable to convert input source ID");
        dstr_free(&layout_name);
        return false;
    }

    blog(LOG_INFO, "hotkeys-cocoa: Using keyboard layout '%s'", layout_name.array);

    dstr_free(&layout_name);
    return true;
}

// MARK: macOS Hotkey CoreFoundation Callbacks

static CGEventRef KeyboardEventProc(CGEventTapProxy proxy __unused, CGEventType type, CGEventRef event, void *userInfo)
{
    obs_hotkeys_platform_t *platform = userInfo;

    const CGEventFlags flags = CGEventGetFlags(event);
    platform->is_key_down[OBS_KEY_SHIFT] = !!(flags & kCGEventFlagMaskShift);
    platform->is_key_down[OBS_KEY_ALT] = !!(flags & kCGEventFlagMaskAlternate);
    platform->is_key_down[OBS_KEY_META] = !!(flags & kCGEventFlagMaskCommand);
    platform->is_key_down[OBS_KEY_CONTROL] = !!(flags & kCGEventFlagMaskControl);

    switch (type) {
        case kCGEventKeyDown: {
            const int64_t keycode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
            platform->is_key_down[obs_key_from_virtual_key(keycode)] = true;
            break;
        }
        case kCGEventKeyUp: {
            const int64_t keycode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
            platform->is_key_down[obs_key_from_virtual_key(keycode)] = false;
            break;
        }
        case kCGEventFlagsChanged: {
            break;
        }
        case kCGEventTapDisabledByTimeout: {
            blog(LOG_DEBUG, "[hotkeys-cocoa]: Hotkey event tap disabled by timeout. Reenabling...");
            CGEventTapEnable(platform->eventTap, true);
            break;
        }
        default: {
            blog(LOG_WARNING, "[hotkeys-cocoa]: Received unexpected event with code '%d'", type);
        }
    }
    return event;
}

static void InputMethodChangedProc(CFNotificationCenterRef center __unused, void *observer,
                                   CFNotificationName name __unused, const void *object __unused,
                                   CFDictionaryRef userInfo __unused)
{
    struct obs_core_hotkeys *hotkeys = observer;
    obs_hotkeys_platform_t *platform = hotkeys->platform_context;

    pthread_mutex_lock(&hotkeys->mutex);

    if (platform->layout_data) {
        CFRelease(platform->layout_data);
    }

    platform->tis = TISCopyCurrentKeyboardLayoutInputSource();
    platform->layout_data = (CFDataRef) TISGetInputSourceProperty(platform->tis, kTISPropertyUnicodeKeyLayoutData);

    if (!platform->layout_data) {
        blog(LOG_ERROR, "hotkeys-cocoa: Failed to retrieve keyboard layout data");
        hotkeys->platform_context = NULL;
        pthread_mutex_unlock(&hotkeys->mutex);
        hotkeys_release(platform);
        return;
    }

    CFRetain(platform->layout_data);
    platform->layout = (UCKeyboardLayout *) CFDataGetBytePtr(platform->layout_data);
}

// MARK: macOS Hotkey API Implementation

bool obs_hotkeys_platform_init(struct obs_core_hotkeys *hotkeys)
{
    CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(), hotkeys, InputMethodChangedProc,
                                    kTISNotifySelectedKeyboardInputSourceChanged, NULL,
                                    CFNotificationSuspensionBehaviorDeliverImmediately);

    obs_hotkeys_platform_t *platform = bzalloc(sizeof(obs_hotkeys_platform_t));

    const bool has_event_access = CGPreflightListenEventAccess();
    if (has_event_access) {
        platform->eventTap = CGEventTapCreate(kCGHIDEventTap, kCGHeadInsertEventTap, kCGEventTapOptionListenOnly,
                                              CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp) |
                                                  CGEventMaskBit(kCGEventFlagsChanged),
                                              KeyboardEventProc, platform);
        if (!platform->eventTap) {
            blog(LOG_WARNING, "[hotkeys-cocoa]: Couldn't create hotkey event tap.");
            hotkeys_release(platform);
            platform = NULL;
            return false;
        }
        CFRunLoopSourceRef source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, platform->eventTap, 0);
        CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopCommonModes);
        CFRelease(source);

        CGEventTapEnable(platform->eventTap, true);
    } else {
        blog(LOG_WARNING, "[hotkeys-cocoa]: No event permissions, could not add global hotkeys.");
    }

    platform->tis = TISCopyCurrentKeyboardLayoutInputSource();
    platform->layout_data = (CFDataRef) TISGetInputSourceProperty(platform->tis, kTISPropertyUnicodeKeyLayoutData);

    if (!platform->layout_data) {
        blog(LOG_ERROR, "hotkeys-cocoa: Failed to retrieve keyboard layout data");
        hotkeys_release(platform);
        platform = NULL;
        return false;
    }

    CFRetain(platform->layout_data);
    platform->layout = (UCKeyboardLayout *) CFDataGetBytePtr(platform->layout_data);

    obs_hotkeys_platform_t *currentPlatform;

    pthread_mutex_lock(&hotkeys->mutex);

    currentPlatform = hotkeys->platform_context;

    if (platform && currentPlatform && platform->layout_data == currentPlatform->layout_data) {
        pthread_mutex_unlock(&hotkeys->mutex);
        hotkeys_release(platform);
        return true;
    }

    hotkeys->platform_context = platform;

    log_layout_name(platform->tis);

    pthread_mutex_unlock(&hotkeys->mutex);

    calldata_t parameters = {0};
    signal_handler_signal(hotkeys->signals, "hotkey_layout_change", &parameters);

    if (currentPlatform) {
        hotkeys_release(currentPlatform);
    }

    bool hasPlatformContext = hotkeys->platform_context != NULL;

    return hasPlatformContext;
}

void obs_hotkeys_platform_free(struct obs_core_hotkeys *hotkeys)
{
    CFNotificationCenterRemoveEveryObserver(CFNotificationCenterGetDistributedCenter(), hotkeys);

    hotkeys_release(hotkeys->platform_context);
}

int obs_key_to_virtual_key(obs_key_t key)
{
    if (virtual_keys[key].is_valid) {
        return virtual_keys[key].code;
    } else {
        return INVALID_KEY;
    }
}

obs_key_t obs_key_from_virtual_key(int keyCode)
{
    for (size_t i = 0; i < OBS_KEY_LAST_VALUE; i++) {
        if (virtual_keys[i].is_valid && virtual_keys[i].code == keyCode) {
            return (obs_key_t) i;
        }
    }

    return OBS_KEY_NONE;
}

bool obs_hotkeys_platform_is_pressed(obs_hotkeys_platform_t *platform, obs_key_t key)
{
    if (key >= OBS_KEY_MOUSE1 && key <= OBS_KEY_MOUSE29) {
        int button = key - 1;

        NSUInteger buttons = [NSEvent pressedMouseButtons];

        if ((buttons & (1 << button)) != 0) {
            return true;
        } else {
            return false;
        }
    }

    if (!platform) {
        return false;
    }

    if (key >= OBS_KEY_LAST_VALUE) {
        return false;
    }

    return platform->is_key_down[key];
}

static void unichar_to_utf8(const UniChar *character, char *buffer)
{
    CFStringRef string = CFStringCreateWithCharactersNoCopy(NULL, character, 2, kCFAllocatorNull);

    if (!string) {
        blog(LOG_ERROR, "hotkey-cocoa: Unable to create CFStringRef with UniChar character");
        return;
    }

    if (!CFStringGetCString(string, buffer, OBS_COCOA_MODIFIER_SIZE, kCFStringEncodingUTF8)) {
        blog(LOG_ERROR, "hotkey-cocoa: Error while populating buffer with glyph %d (0x%x)", character[0], character[0]);
    }

    CFRelease(string);
}

void obs_key_combination_to_str(obs_key_combination_t key, struct dstr *str)
{
    struct dstr keyString = {0};

    if (key.key != OBS_KEY_NONE) {
        obs_key_to_str(key.key, &keyString);
    }

    dispatch_once(&onceToken, ^{
        const UniChar controlCharacter[] = {kControlUnicode, 0};
        const UniChar optionCharacter[] = {kOptionUnicode, 0};
        const UniChar shiftCharacter[] = {kShiftUnicode, 0};
        const UniChar commandCharacter[] = {kCommandUnicode, 0};

        unichar_to_utf8(controlCharacter, string_control);
        unichar_to_utf8(optionCharacter, string_option);
        unichar_to_utf8(shiftCharacter, string_shift);
        unichar_to_utf8(commandCharacter, string_command);
    });

    const char *modifier_control = (key.modifiers & INTERACT_CONTROL_KEY) ? string_control : "";
    const char *modifier_option = (key.modifiers & INTERACT_ALT_KEY) ? string_option : "";
    const char *modifier_shift = (key.modifiers & INTERACT_SHIFT_KEY) ? string_shift : "";
    const char *modifier_command = (key.modifiers & INTERACT_COMMAND_KEY) ? string_command : "";
    const char *modifier_key = keyString.len ? keyString.array : "";

    dstr_printf(str, "%s%s%s%s%s", modifier_control, modifier_option, modifier_shift, modifier_command, modifier_key);

    dstr_free(&keyString);
}

void obs_key_to_str(obs_key_t key, struct dstr *str)
{
    const UniCharCount maxLength = 16;
    UniChar buffer[16];

    if (obs_key_to_localized_string(key, str)) {
        return;
    }

    int code = obs_key_to_virtual_key(key);
    if (key_code_to_string(code, str)) {
        return;
    }

    if (code == INVALID_KEY) {
        const char *keyName = obs_key_to_name(key);
        blog(LOG_ERROR, "hotkey-cocoa: Got invalid key while translating '%d' (%s)", key, keyName);
        dstr_copy(str, keyName);
        return;
    }

    struct obs_hotkeys_platform *platform = NULL;

    if (obs) {
        pthread_mutex_lock(&obs->hotkeys.mutex);
        platform = obs->hotkeys.platform_context;
        hotkeys_retain(platform);
        pthread_mutex_unlock(&obs->hotkeys.mutex);
    }

    if (!platform) {
        const char *keyName = obs_key_to_name(key);
        blog(LOG_ERROR, "hotkey-cocoa: Could not get hotkey platform while translating '%d' (%s)", key, keyName);
        dstr_copy(str, keyName);
        return;
    }

    UInt32 deadKeyState = 0;
    UniCharCount length = 0;

    OSStatus err = UCKeyTranslate(platform->layout, code, kUCKeyActionDown, ((alphaLock >> 8) & 0xFF), LMGetKbdType(),
                                  kUCKeyTranslateNoDeadKeysBit, &deadKeyState, maxLength, &length, buffer);

    if (err == noErr && length <= 0 && deadKeyState) {
        err = UCKeyTranslate(platform->layout, kVK_Space, kUCKeyActionDown, ((alphaLock >> 8) & 0xFF), LMGetKbdType(),
                             kUCKeyTranslateNoDeadKeysBit, &deadKeyState, maxLength, &length, buffer);
    }

    hotkeys_release(platform);

    if (err != noErr) {
        const char *keyName = obs_key_to_name(key);
        blog(LOG_ERROR, "hotkey-cocoa: Error while translating '%d' (0x%x, %s) to string: %d", key, code, keyName, err);
        dstr_copy(str, keyName);
        return;
    }

    if (length == 0) {
        const char *keyName = obs_key_to_name(key);
        blog(LOG_ERROR, "hotkey-cocoa: Got zero length string while translating '%d' (0x%x, %s) to string", key, code,
             keyName);
        dstr_copy(str, keyName);
        return;
    }

    CFStringRef string = CFStringCreateWithCharactersNoCopy(NULL, buffer, length, kCFAllocatorNull);

    if (!string) {
        const char *keyName = obs_key_to_name(key);
        blog(LOG_ERROR, "hotkey-cocoa: Could not create CFStringRef while translating '%d' (0x%x, %s) to string", key,
             code, keyName);
        dstr_copy(str, keyName);
        return;
    }

    if (!dstr_from_cfstring(str, string)) {
        const char *keyName = obs_key_to_name(key);
        blog(LOG_ERROR, "hotkey-cocoa: Could not translate CFStringRef to CString while translating '%d' (0x%x, %s)",
             key, code, keyName);
        CFRelease(string);
        dstr_copy(str, keyName);
        return;
    }

    CFRelease(string);
    return;
}
