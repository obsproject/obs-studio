#include "mac-update.hpp"

#include <qaction.h>

#import <Cocoa/Cocoa.h>
#import <Sparkle/Sparkle.h>

@interface OBSUpdateDelegate : NSObject <SPUUpdaterDelegate> {
}
@property (copy) NSString *branch;
@property (nonatomic) SPUStandardUpdaterController *updaterController;
@end

@implementation OBSUpdateDelegate {
}

@synthesize branch;

- (nonnull NSSet<NSString *> *)allowedChannelsForUpdater:(nonnull SPUUpdater *)updater
{
    return [NSSet setWithObject:branch];
}

- (void)observeCanCheckForUpdatesWithAction:(QAction *)action
{
    [_updaterController.updater addObserver:self forKeyPath:NSStringFromSelector(@selector(canCheckForUpdates))
                                    options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew)
                                    context:(void *) action];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey, id> *)change
                       context:(void *)context
{
    if ([keyPath isEqualToString:NSStringFromSelector(@selector(canCheckForUpdates))]) {
        QAction *menuAction = (QAction *) context;
        menuAction->setEnabled(_updaterController.updater.canCheckForUpdates);
    } else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (void)dealloc
{
    @autoreleasepool {
        [_updaterController.updater removeObserver:self forKeyPath:NSStringFromSelector(@selector(canCheckForUpdates))];
    }
}

@end

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
