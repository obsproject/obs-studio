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
#import "mac-au-plugin.h"
#import <AudioToolbox/AUAudioUnit.h>
#import <CoreFoundation/CoreFoundation.h>

@interface AUPluginObjC : NSObject <NSWindowDelegate>
@property (strong, nonatomic) NSPanel *window;
@property (strong, nonatomic) NSView *editorView;
@property (strong, nonatomic) NSViewController *viewController;
@property (assign, nonatomic) struct au_plugin *plugin;

@property (assign, nonatomic) BOOL resizingFromHost;
@property (assign, nonatomic) BOOL resizingFromPlugin;
@property (assign, nonatomic) BOOL hasSeenPluginResize;
@property (assign, nonatomic) BOOL pluginCanResizeFromHost;

// New methods
- (void)startObservingPluginResize;
- (void)pluginViewDidResize:(NSNotification *)n;

@end

@implementation AUPluginObjC
// we intercept the close call in order to sync correctly UI w/ the SHow/Hide button in Properties
- (BOOL)windowShouldClose:(NSPanel *)sender
{
    [sender orderOut:nil];
    if (self.plugin) {
        self.plugin->editor_is_visible = false;
    }
    return NO;
}

// The resizing logic gave me headaches. AUv3 and Apple AUv2 resizing works directly. But for other AUv2 the host window and
// the AU view would resize independently. I noticed though that for these AUv2, Logic Pro blocks the resizing of both the
// container and the AU view. Reaper however allows direct resizing of these AUv2 GUI with an auto-adjustment of the host
// container, but if the resizing is initiated by the host container, the AU view doesn't budge ... It gave me the idea to
// combine both approaches of Logic Pro + Reaper and so I blocked resizing of the host container while allowing it if
// initiated by the AU view.
- (void)windowDidResize:(NSNotification *)notification
{
    if (!self.window || !self.editorView || !self.pluginCanResizeFromHost || self.resizingFromPlugin)
        return;

    self.resizingFromHost = YES;
    NSView *parent = self.window.contentView;
    self.editorView.frame = parent.bounds;
    self.resizingFromHost = NO;
}

- (void)startObservingPluginResize
{
    if (!self.editorView)
        return;

    self.hasSeenPluginResize = NO;
    [self.editorView setPostsFrameChangedNotifications:YES];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(pluginViewDidResize:)
                                                 name:NSViewFrameDidChangeNotification
                                               object:self.editorView];
}

- (void)pluginViewDidResize:(NSNotification *)n
{
    if (self.resizingFromHost)
        return;

    if (!self.hasSeenPluginResize) {
        self.hasSeenPluginResize = YES;
        return;
    }

    self.resizingFromPlugin = YES;

    NSView *view = self.editorView;
    NSWindow *window = self.window;

    if (!view || !window) {
        self.resizingFromPlugin = NO;
        return;
    }

    NSRect contentRect = NSMakeRect(0, 0, view.frame.size.width, view.frame.size.height);
    NSRect newFrame = [window frameRectForContentRect:contentRect];
    NSRect oldFrame = window.frame;
    newFrame.origin.x = oldFrame.origin.x;
    newFrame.origin.y = oldFrame.origin.y + oldFrame.size.height - newFrame.size.height;
    // this is where we tell the host container to resize
    [window setFrame:newFrame display:YES animate:NO];

    self.resizingFromPlugin = NO;
}

@end

bool scan_buses(const struct au_descriptor *desc, struct au_plugin *p, double sample_rate, uint32_t channels)
{
    NSError *err = nil;
    AVAudioFormat *format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:sample_rate
                                                                           channels:(AVAudioChannelCount) channels];

    p->num_in_audio_buses = (int) p->au.inputBusses.count;
    p->num_out_audio_buses = (int) p->au.outputBusses.count;
    p->num_enabled_in_audio_buses = 0;
    p->num_enabled_out_audio_buses = 0;
    p->sidechain_num_channels = 0;

    NSLog(@"[AU filter] AU %s has %d input buses, %d output buses", desc->name, p->num_in_audio_buses,
          p->num_out_audio_buses);

    // Main input Bus
    if (p->num_in_audio_buses > 0) {
        AUAudioUnitBus *inBus = p->au.inputBusses[0];
        if (![inBus setFormat:format error:&err]) {
            NSLog(@"[AU filter] AU %s cannot set %u-ch main input format: %@", desc->name, channels, err);
            return false;
        }
        inBus.enabled = YES;
        p->num_enabled_in_audio_buses++;
    }

    // Main output bus
    if (p->num_out_audio_buses > 0) {
        AUAudioUnitBus *outBus = p->au.outputBusses[0];
        if (![outBus setFormat:format error:&err]) {
            NSLog(@"[AU filter] AU %s cannot set %u-ch main output format: %@", desc->name, channels, err);
            return false;
        }
        outBus.enabled = YES;
        p->num_enabled_out_audio_buses++;

        if ([outBus respondsToSelector:@selector(supportedChannelLayoutTags)]) {
            NSArray *layouts = outBus.supportedChannelLayoutTags;
            NSMutableString *s = [NSMutableString stringWithString:@"["];
            for (NSUInteger i = 0; i < layouts.count; i++) {
                unsigned int tag = [layouts[i] unsignedIntValue];
                [s appendFormat:@"0x%08X", tag];

                if (i + 1 < layouts.count)
                    [s appendString:@","];
            }
            [s appendString:@"]"];
            NSLog(@"[AU filter] AU %s main output layouts = %s", desc->name, [s UTF8String]);
        }
    }

    // sidechain bus: we support only mono, stereo or same number of channels as obs
    if (p->num_in_audio_buses > 1) {
        AUAudioUnitBus *scBus = p->au.inputBusses[1];
        NSArray<NSNumber *> *candidates = @[@1, @2, @(channels)];
        uint32_t sc = 0;

        for (NSNumber *n in candidates) {
            uint32_t nCh = n.unsignedIntValue;
            AVAudioFormat *scfmt = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:sample_rate
                                                                                  channels:(AVAudioChannelCount) nCh];
            if ([scBus setFormat:scfmt error:nil]) {
                sc = nCh;
                break;
            }
        }

        if (sc > 0) {
            scBus.enabled = YES;
            p->sidechain_num_channels = sc;
            p->num_enabled_in_audio_buses++;
            NSLog(@"[AU filter] AU %s sidechain enabled (%u ch)", desc->name, sc);
        } else {
            scBus.enabled = NO;
            NSLog(@"[AU filter] AU %s sidechain bus present but no matching channel count", desc->name);
        }
    }

    return true;
}

struct au_plugin *au_plugin_create(const struct au_descriptor *desc, double sample_rate, uint32_t frames,
                                   uint32_t channels)
{
    if (!desc)
        return NULL;

    struct au_plugin *p = (struct au_plugin *) calloc(1, sizeof(*p));
    if (!p)
        return NULL;

    AUPluginObjC *objc = [[AUPluginObjC alloc] init];
    objc.plugin = p;
    p->objc = (__bridge_retained void *) objc;

    strlcpy(p->uid, desc->uid, sizeof(p->uid));
    strlcpy(p->name, desc->name, sizeof(p->name));

    NSString *nameStr = [NSString stringWithUTF8String:desc->name];
    NSString *vendorStr = [NSString stringWithUTF8String:desc->vendor];
    p->title = [NSString stringWithFormat:@"%@ – %@", nameStr, vendorStr];
    strlcpy(p->vendor, desc->vendor, sizeof(p->vendor));
    p->sample_rate = sample_rate;
    p->channels = channels;
    p->max_frames = frames;

    @autoreleasepool {
        __block BOOL done = NO;
        __block AVAudioUnit *created = nil;

        [AVAudioUnit instantiateWithComponentDescription:desc->desc options:0
                                       completionHandler:^(AVAudioUnit *av, NSError *err) {
                                           if (av && !err) {
                                               created = av;
                                           }
                                           done = YES;
                                       }];

        while (!done)
            [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]];

        if (!created) {
            free(p);
            return NULL;
        }

        p->av_unit = created;
        p->au = created.AUAudioUnit;
        p->has_view = p->au.providesUserInterface;
        if (!p->has_view)
            NSLog(@"[AU filter] '%s'has no GUI", p->name);
    }

    if (!scan_buses(desc, p, sample_rate, channels)) {
        CFRelease((CFTypeRef) p->objc);
        free(p);
        return NULL;
    }

    NSError *err = nil;
    if (![p->au allocateRenderResourcesAndReturnError:&err]) {
        NSLog(@"[AU filter] allocateRenderResources failed for '%s': %@", p->name, err);
        CFRelease((CFTypeRef) p->objc);
        free(p);
        return NULL;
    }

    return p;
}

void au_plugin_destroy(struct au_plugin *p)
{
    if (!p)
        return;

    if (p->objc) {
        CFRelease((CFTypeRef) p->objc);
        p->objc = NULL;
    }

    if (p->au) {
        [p->au deallocateRenderResources];
        p->au = nil;
    }

    p->av_unit = nil;

    free(p);
}

// main processing function; the incoming audio data is in float *const *channels; the processed audio will be copied into
// the input buffers. This is what will be retrieved on obs side in the output deque.
void au_plugin_process(struct au_plugin *p, float *const *channels, float *const *sc_channels, bool sidechain_enabled,
                       uint32_t num_frames, uint32_t num_channels)
{
    if (!p || !p->au)
        return;

    if (num_channels == 0 || num_channels > 32)
        return;

    struct {
        AudioBufferList list;
        AudioBuffer extraBuffers[31];
    } bufList;

    AudioBufferList *abl = &bufList.list;
    abl->mNumberBuffers = num_channels;

    for (uint32_t ch = 0; ch < num_channels; ch++) {
        abl->mBuffers[ch].mNumberChannels = 1;
        abl->mBuffers[ch].mDataByteSize = num_frames * sizeof(float);
        abl->mBuffers[ch].mData = (void *) channels[ch];
    }

    AudioTimeStamp ts;
    memset(&ts, 0, sizeof(ts));
    ts.mFlags = kAudioTimeStampSampleTimeValid;
    ts.mSampleTime = p->sample_time;

    AURenderPullInputBlock pullInput = ^AUAudioUnitStatus(AudioUnitRenderActionFlags *actionFlags,
                                                          const AudioTimeStamp *timestamp, AUAudioFrameCount frameCount,
                                                          NSInteger inputBusNumber, AudioBufferList *inputData) {
        (void) actionFlags;
        (void) timestamp;

        // main input bus
        if (inputBusNumber == 0) {
            inputData->mNumberBuffers = num_channels;

            for (uint32_t ch = 0; ch < num_channels; ch++) {
                inputData->mBuffers[ch].mNumberChannels = 1;
                inputData->mBuffers[ch].mDataByteSize = frameCount * sizeof(float);
                inputData->mBuffers[ch].mData = (void *) channels[ch];
            }
            return noErr;
        }

        // sidechain bus
        if (inputBusNumber == 1) {
            if (!sidechain_enabled || p->sidechain_num_channels == 0 || sc_channels == NULL) {
                return kAudioUnitErr_NoConnection;
            }

            inputData->mNumberBuffers = p->sidechain_num_channels;

            for (int ch = 0; ch < p->sidechain_num_channels; ch++) {
                inputData->mBuffers[ch].mNumberChannels = 1;
                inputData->mBuffers[ch].mDataByteSize = frameCount * sizeof(float);
                inputData->mBuffers[ch].mData = (void *) sc_channels[ch];
            }

            return noErr;
        }
        return kAudioUnitErr_NoConnection;
    };

    AURenderBlock renderBlock = p->au.renderBlock;
    if (!renderBlock)
        return;

    AudioUnitRenderActionFlags flags = 0;

    renderBlock(&flags, &ts, (AUAudioFrameCount) num_frames, 0, abl, pullInput);

    p->sample_time += num_frames;
}

CFDataRef au_plugin_save_state(struct au_plugin *p)
{
    if (!p || !p->au)
        return NULL;

    NSDictionary *state = p->au.fullStateForDocument;
    if (!state)
        return NULL;

    NSError *error = nil;
    NSData *data = [NSPropertyListSerialization dataWithPropertyList:state format:NSPropertyListBinaryFormat_v1_0
                                                             options:0
                                                               error:&error];
    if (!data || error)
        return NULL;

    return CFDataCreate(kCFAllocatorDefault, (const UInt8 *) [data bytes], (CFIndex)[data length]);
}

void au_plugin_load_state(struct au_plugin *p, CFDataRef data)
{
    if (!p || !p->au || !data)
        return;

    NSError *error = nil;

    NSDictionary *state = [NSPropertyListSerialization propertyListWithData:(__bridge NSData *) data
                                                                    options:NSPropertyListImmutable
                                                                     format:nil
                                                                      error:&error];
    if (!state || error)
        return;

    p->au.fullStateForDocument = state;
}

static NSWindow *FindOBSFiltersWindow(void)
{
    for (NSWindow *w in NSApp.windows) {
        if (![w isKindOfClass:[NSPanel class]] && w.isVisible && w.canBecomeMainWindow)
            return w;
    }
    return NSApp.mainWindow;
}

static NSPanel *au_create_editor_window(AUPluginObjC *objc)
{
    const CGFloat W = 640.0;
    const CGFloat H = 480.0;

    // try positioning the AU GUI to the left of Properties
    NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
    NSWindow *anchor = FindOBSFiltersWindow();
    NSScreen *screen = anchor.screen ?: [NSScreen mainScreen];
    NSRect S = screen.visibleFrame;
    NSRect A = anchor.frame;

    NSRect AUFrame = [NSWindow frameRectForContentRect:NSMakeRect(0, 0, W, H) styleMask:style];
    CGFloat Ex = A.origin.x - AUFrame.size.width - 20;
    CGFloat Ey = A.origin.y + (A.size.height / 2.0) - (AUFrame.size.height / 2.0);

    if (Ex < S.origin.x + 20)
        Ex = S.origin.x + 20;

    if (Ey < S.origin.y + 20)
        Ey = S.origin.y + 20;

    if (Ey + AUFrame.size.height > S.origin.y + S.size.height - 20)
        Ey = S.origin.y + S.size.height - AUFrame.size.height - 20;

    // create AU panel
    NSRect contentRect = NSMakeRect(Ex, Ey, W, H);
    NSPanel *panel = [[NSPanel alloc] initWithContentRect:contentRect styleMask:style backing:NSBackingStoreBuffered
                                                    defer:YES];

    panel.delegate = objc;
    panel.title = objc.plugin->title;
    panel.floatingPanel = YES;
    panel.collectionBehavior = NSWindowCollectionBehaviorFullScreenAuxiliary |
                               NSWindowCollectionBehaviorCanJoinAllSpaces;
    panel.becomesKeyOnlyIfNeeded = NO;
    panel.hidesOnDeactivate = NO;

    return panel;
}

void au_plugin_show_editor(struct au_plugin *p)
{
    if (!p || !p->au || !p->objc || !p->has_view)
        return;

    AUPluginObjC *objc = (__bridge AUPluginObjC *) p->objc;
    if (!objc.window) {
        objc.window = au_create_editor_window(objc);
        if (!objc.window)
            return;
    }

    NSPanel *win = objc.window;

    if (!objc.viewController) {
        __block AUViewControllerBase *vc = nil;
        __block BOOL done = NO;

        [p->au requestViewControllerWithCompletionHandler:^(AUViewControllerBase *viewController) {
            vc = viewController;
            done = YES;
        }];

        while (!done) {
            [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]];
        }

        if (!vc) {
            NSLog(@"[AU filter] AU '%s' returned NULL viewController", p->name);
            return;
        }

        objc.viewController = vc;
    }

    if (!objc.editorView) {
        CGSize size = CGSizeZero;

        if ([objc.viewController respondsToSelector:@selector(preferredContentSize)])
            size = objc.viewController.preferredContentSize;

        if (size.width <= 0.0 || size.height <= 0.0)
            size = objc.viewController.view.frame.size;

        if (size.width < 120.0)
            size.width = 120.0;
        if (size.height < 120.0)
            size.height = 120.0;

        [win setContentSize:size];
    }

    NSView *parent = win.contentView;
    NSView *view = objc.viewController.view;

    if (objc.editorView && objc.editorView != view) {
        [objc.editorView removeFromSuperview];
        objc.editorView = nil;
    }

    if (view.superview != parent) {
        view.frame = parent.bounds;
        view.autoresizingMask = 0;
        [parent addSubview:view];
    }

    objc.editorView = view;
    [objc startObservingPluginResize];

    // block host window resizing for AUv2 except Apple's
    BOOL is_v3 = p->is_v3;
    BOOL is_apple = (strstr(p->vendor, "Apple") != NULL);
    objc.pluginCanResizeFromHost = is_v3 || is_apple;

    NSUInteger mask = win.styleMask;
    if (objc.pluginCanResizeFromHost)
        mask |= NSWindowStyleMaskResizable;
    else
        mask &= ~NSWindowStyleMaskResizable;

    [win setStyleMask:mask];

    [NSApp activateIgnoringOtherApps:YES];
    [win makeKeyAndOrderFront:nil];

    p->editor_is_visible = true;
}

void au_plugin_hide_editor(struct au_plugin *p)
{
    if (!p || !p->objc)
        return;

    AUPluginObjC *objc = (__bridge AUPluginObjC *) p->objc;
    NSPanel *win = objc.window;
    p->editor_is_visible = false;

    if (!win)
        return;

    if (win.isVisible)
        [win orderOut:nil];
}
