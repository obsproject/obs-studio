/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

#import "platform.hpp"

#import <OBSApp.hpp>

#import <util/threading.h>

#import <AVFoundation/AVFoundation.h>
#import <AppKit/AppKit.h>
#import <dlfcn.h>

using namespace std;

bool isInBundle()
{
    NSRunningApplication *app = [NSRunningApplication currentApplication];
    return [app bundleIdentifier] != nil;
}

bool GetDataFilePath(const char *data, string &output)
{
    NSURL *bundleUrl = [[NSBundle mainBundle] bundleURL];
    NSString *path = [[bundleUrl path] stringByAppendingFormat:@"/%@/%s", @"Contents/Resources", data];
    output = path.UTF8String;

    return !access(output.c_str(), R_OK);
}

void CheckIfAlreadyRunning(bool &already_running)
{
    NSString *bundleId = [[NSBundle mainBundle] bundleIdentifier];

    NSUInteger appCount = [[NSRunningApplication runningApplicationsWithBundleIdentifier:bundleId] count];

    already_running = appCount > 1;
}

string GetDefaultVideoSavePath()
{
    NSFileManager *fm = [NSFileManager defaultManager];
    NSURL *url = [fm URLForDirectory:NSMoviesDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:true
                               error:nil];

    if (!url)
        return getenv("HOME");

    return url.path.fileSystemRepresentation;
}

vector<string> GetPreferredLocales()
{
    NSArray *preferred = [NSLocale preferredLanguages];

    auto locales = GetLocaleNames();
    auto lang_to_locale = [&locales](string lang) -> string {
        string lang_match = "";

        for (const auto &locale : locales) {
            if (locale.first == lang.substr(0, locale.first.size()))
                return locale.first;

            if (!lang_match.size() && locale.first.substr(0, 2) == lang.substr(0, 2))
                lang_match = locale.first;
        }

        return lang_match;
    };

    vector<string> result;
    result.reserve(preferred.count);

    for (NSString *lang in preferred) {
        string locale = lang_to_locale(lang.UTF8String);
        if (!locale.size())
            continue;

        if (find(begin(result), end(result), locale) != end(result))
            continue;

        result.emplace_back(locale);
    }

    return result;
}

bool IsAlwaysOnTop(QWidget *window)
{
    return (window->windowFlags() & Qt::WindowStaysOnTopHint) != 0;
}

void disableColorSpaceConversion(QWidget *window)
{
    NSView *view = (__bridge NSView *) reinterpret_cast<void *>(window->winId());
    view.window.colorSpace = NSColorSpace.sRGBColorSpace;
}

void SetAlwaysOnTop(QWidget *window, bool enable)
{
    Qt::WindowFlags flags = window->windowFlags();

    if (enable) {
        NSView *view = (__bridge NSView *) reinterpret_cast<void *>(window->winId());

        [[view window] setLevel:NSScreenSaverWindowLevel];

        flags |= Qt::WindowStaysOnTopHint;
    } else {
        flags &= ~Qt::WindowStaysOnTopHint;
    }

    window->setWindowFlags(flags);
    window->show();
}

bool SetDisplayAffinitySupported(void)
{
    // Not implemented yet
    return false;
}

typedef void (*set_int_t)(int);

void EnableOSXVSync(bool enable)
{
    static bool initialized = false;
    static bool valid = false;
    static set_int_t set_debug_options = nullptr;
    static set_int_t deferred_updates = nullptr;

    if (!initialized) {
        void *quartzCore = dlopen("/System/Library/Frameworks/"
                                  "QuartzCore.framework/QuartzCore",
                                  RTLD_LAZY);
        if (quartzCore) {
            set_debug_options = (set_int_t) dlsym(quartzCore, "CGSSetDebugOptions");
            deferred_updates = (set_int_t) dlsym(quartzCore, "CGSDeferredUpdates");

            valid = set_debug_options && deferred_updates;
        }

        initialized = true;
    }

    if (valid) {
        set_debug_options(enable ? 0 : 0x08000000);
        deferred_updates(enable ? 1 : 0);
    }
}

void EnableOSXDockIcon(bool enable)
{
    if (enable)
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    else
        [NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];
}

@interface DockView : NSView {
      @private
    QIcon _icon;
}
@end

@implementation DockView

- (id)initWithIcon:(QIcon)icon
{
    self = [super init];
    _icon = icon;
    return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
    CGSize size = dirtyRect.size;

    /* Draw regular app icon */
    NSImage *appIcon = [[NSWorkspace sharedWorkspace] iconForFile:[[NSBundle mainBundle] bundlePath]];
    [appIcon drawInRect:CGRectMake(0, 0, size.width, size.height)];

    /* Draw small icon on top */
    float iconSize = 0.45;
    CGImageRef image = _icon.pixmap(size.width, size.height).toImage().toCGImage();
    CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
    CGContextDrawImage(
        context, CGRectMake(size.width * (1 - iconSize), 0, size.width * iconSize, size.height * iconSize), image);
    CGImageRelease(image);
}

@end

MacPermissionStatus CheckPermissionWithPrompt(MacPermissionType type, bool prompt_for_permission)
{
    __block MacPermissionStatus permissionResponse = kPermissionNotDetermined;

    switch (type) {
        case kAudioDeviceAccess: {
            AVAuthorizationStatus audioStatus = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];

            if (audioStatus == AVAuthorizationStatusNotDetermined && prompt_for_permission) {
                os_event_t *block_finished;
                os_event_init(&block_finished, OS_EVENT_TYPE_MANUAL);
                [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio
                                         completionHandler:^(BOOL granted __attribute((unused))) {
                                             os_event_signal(block_finished);
                                         }];
                os_event_wait(block_finished);
                os_event_destroy(block_finished);
                audioStatus = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];
            }

            permissionResponse = (MacPermissionStatus) audioStatus;

            blog(LOG_INFO, "[macOS] Permission for audio device access %s.",
                 permissionResponse == kPermissionAuthorized ? "granted" : "denied");

            break;
        }
        case kVideoDeviceAccess: {
            AVAuthorizationStatus videoStatus = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];

            if (videoStatus == AVAuthorizationStatusNotDetermined && prompt_for_permission) {
                os_event_t *block_finished;
                os_event_init(&block_finished, OS_EVENT_TYPE_MANUAL);
                [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo
                                         completionHandler:^(BOOL granted __attribute((unused))) {
                                             os_event_signal(block_finished);
                                         }];

                os_event_wait(block_finished);
                os_event_destroy(block_finished);
                videoStatus = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
            }

            permissionResponse = (MacPermissionStatus) videoStatus;

            blog(LOG_INFO, "[macOS] Permission for video device access %s.",
                 permissionResponse == kPermissionAuthorized ? "granted" : "denied");

            break;
        }
        case kScreenCapture: {
            permissionResponse = (CGPreflightScreenCaptureAccess() ? kPermissionAuthorized : kPermissionDenied);

            if (permissionResponse != kPermissionAuthorized && prompt_for_permission) {
                permissionResponse = (CGRequestScreenCaptureAccess() ? kPermissionAuthorized : kPermissionDenied);
            }

            blog(LOG_INFO, "[macOS] Permission for screen capture %s.",
                 permissionResponse == kPermissionAuthorized ? "granted" : "denied");

            break;
        }
        case kAccessibility: {
            permissionResponse = (AXIsProcessTrusted() ? kPermissionAuthorized : kPermissionDenied);

            if (permissionResponse != kPermissionAuthorized && prompt_for_permission) {
                NSDictionary *options = @{(__bridge id) kAXTrustedCheckOptionPrompt: @YES};
                permissionResponse = (AXIsProcessTrustedWithOptions((CFDictionaryRef) options) ? kPermissionAuthorized
                                                                                               : kPermissionDenied);
            }

            blog(LOG_INFO, "[macOS] Permission for accessibility %s.",
                 permissionResponse == kPermissionAuthorized ? "granted" : "denied");
            break;
        }
    }

    return permissionResponse;
}

void OpenMacOSPrivacyPreferences(const char *tab)
{
    NSURL *url = [NSURL
        URLWithString:[NSString
                          stringWithFormat:@"x-apple.systempreferences:com.apple.preference.security?Privacy_%s", tab]];
    [[NSWorkspace sharedWorkspace] openURL:url];
}

void SetMacOSDarkMode(bool dark)
{
    if (dark) {
        NSApp.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    } else {
        NSApp.appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];
    }
}

void TaskbarOverlayInit() {}

void TaskbarOverlaySetStatus(TaskbarOverlayStatus status)
{
    QIcon icon;
    if (status == TaskbarOverlayStatusActive)
        icon = QIcon::fromTheme("obs-active", QIcon(":/res/images/active_mac.png"));
    else if (status == TaskbarOverlayStatusPaused)
        icon = QIcon::fromTheme("obs-paused", QIcon(":/res/images/paused_mac.png"));

    NSDockTile *tile = [NSApp dockTile];
    [tile setContentView:[[DockView alloc] initWithIcon:icon]];
    [tile display];
}

/*
 * This custom NSApplication subclass makes the app compatible with CEF. Qt
 * also has an NSApplication subclass, but it doesn't conflict thanks to Qt
 * using arcane magic to hook into the NSApplication superclass itself if the
 * program has its own NSApplication subclass.
 */

@protocol CrAppProtocol
- (BOOL)isHandlingSendEvent;
@end

@interface OBSApplication : NSApplication <CrAppProtocol>
@property (nonatomic, getter=isHandlingSendEvent) BOOL handlingSendEvent;
@end

@implementation OBSApplication

- (void)sendEvent:(NSEvent *)event
{
    _handlingSendEvent = YES;
    [super sendEvent:event];
    _handlingSendEvent = NO;
}

@end

void InstallNSThreadLocks()
{
    [[NSThread new] start];

    if ([NSThread isMultiThreaded] != 1) {
        abort();
    }
}

void InstallNSApplicationSubclass()
{
    [OBSApplication sharedApplication];
}

bool HighContrastEnabled()
{
    return [[NSWorkspace sharedWorkspace] accessibilityDisplayShouldIncreaseContrast];
}
