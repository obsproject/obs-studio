#import "OBSSparkle.hpp"
#import "OBSUpdateDelegate.h"

OBSSparkle::OBSSparkle(const char *branch, QAction *checkForUpdatesAction)
{
    @autoreleasepool {
        updaterDelegate = [[OBSUpdateDelegate alloc] init];
        updaterDelegate.branch = [NSString stringWithUTF8String:branch];
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
