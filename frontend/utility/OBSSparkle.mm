#import "OBSSparkle.hpp"
#import "OBSUpdateDelegate.h"

#import <util/platform.h>

OBSSparkle::OBSSparkle(const char *branch, QAction *checkForUpdatesAction)
{
    @autoreleasepool {
        updaterDelegate = [[OBSUpdateDelegate alloc] init];
        updaterDelegate.branch = [NSString stringWithUTF8String:branch];

        if (os_get_emulation_status()) {
            updaterDelegate.feedUrl = @"https://obsproject.com/osx_update/updates_arm64_v2.xml";
        } else {
            updaterDelegate.feedUrl = nil;
        }

        updaterDelegate.updaterController =
            [[SPUStandardUpdaterController alloc] initWithStartingUpdater:YES updaterDelegate:updaterDelegate
                                                       userDriverDelegate:nil];
        [updaterDelegate observeCanCheckForUpdatesWithAction:checkForUpdatesAction];
    }
}

void OBSSparkle::setBranch(const char *branch)
{
    updaterDelegate.branch = [NSString stringWithUTF8String:branch];
}

void OBSSparkle::checkForUpdates(bool manualCheck)
{
    @autoreleasepool {
        if (manualCheck) {
            [updaterDelegate.updaterController checkForUpdates:nil];
        } else {
            [updaterDelegate.updaterController.updater checkForUpdatesInBackground];
        }
    }
}
