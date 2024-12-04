#import "OBSUpdateDelegate.h"

@implementation OBSUpdateDelegate {
}

@synthesize branch;

- (nonnull NSSet<NSString *> *)allowedChannelsForUpdater:(nonnull SPUUpdater *)updater
{
    return [NSSet setWithObject:branch];
}

- (void)observeCanCheckForUpdatesWithAction:(nonnull QAction *)action;
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
