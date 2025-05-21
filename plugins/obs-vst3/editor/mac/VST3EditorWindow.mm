/* Copyright (c) 2025 pkv <pkv@obsproject.com>
 *
 * This file uses the Steinberg VST3 SDK, which is licensed under MIT license.
 * See https://github.com/steinbergmedia/vst3sdk for details.
 *
 * This file and all modifications by pkv <pkv@obsproject.com> are licensed under
 * the GNU General Public License, version 3 or later, to comply with the SDK license.
 */

#import <Cocoa/Cocoa.h>
#include "VST3EditorWindow.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include "pluginterfaces/base/funknown.h"

#define UNUSED_PARAMETER(x) (void)(x)

@interface VST3EditorWindowDelegate : NSObject <NSWindowDelegate>
@property (nonatomic, assign) Steinberg::IPlugView *vst3View;
@property (nonatomic, assign) VST3EditorWindow *windowObj;
@end

@implementation VST3EditorWindowDelegate
- (void)windowDidResize:(NSNotification *)notification
{
    NSWindow *window = (NSWindow *) notification.object;
    NSSize size = [window.contentView frame].size;
    if (self.vst3View) {
        Steinberg::ViewRect vr(0, 0, size.width, size.height);
        self.vst3View->onSize(&vr);
    }
}
- (BOOL)windowShouldClose:(NSWindow *)sender
{
    if (self.windowObj) {
        self.windowObj->setClosedState(true);
        self.windowObj->close();
        return NO;
    }
    return YES;
}
@end

void fail(const char *msg)
{
    fprintf(stderr, "VST3 Editor Fatal Error: %s\n", msg);
}

//------------------------------------------------------------

class PlugFrameImpl : public Steinberg::IPlugFrame
{
      public:
    explicit PlugFrameImpl(NSWindow *window) : _window(window) {}

    Steinberg::tresult PLUGIN_API resizeView(Steinberg::IPlugView *, Steinberg::ViewRect *newSize) override
    {
        if (_window && newSize) {
            [_window setContentSize:NSMakeSize(newSize->getWidth(), newSize->getHeight())];
            NSRect newFrame =
                [_window frameRectForContentRect:NSMakeRect(0, 0, newSize->getWidth(), newSize->getHeight())];
            [_window setFrame:newFrame display:YES animate:NO];
            return Steinberg::kResultTrue;
        }
        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid, void **obj) override
    {
        if (Steinberg::FUnknownPrivate::iidEqual(_iid, Steinberg::IPlugFrame::iid)) {
            *obj = this;
            return Steinberg::kResultOk;
        }
        *obj = nullptr;
        return Steinberg::kNoInterface;
    }

    uint32_t PLUGIN_API addRef() override
    {
        return 1;
    }
    uint32_t PLUGIN_API release() override
    {
        return 1;
    }

      private:
    NSWindow *_window;
};

//------------------------------------------------------------

class VST3EditorWindow::Impl
{
      public:
    Impl(Steinberg::IPlugView *view, const std::string &title, VST3EditorWindow *owner) :
        _view(view),
        _window(nil),
        _frame(nullptr),
        _title(title),
        _delegate(nil),
        _owner(owner),
        _resizeable(view->canResize() == Steinberg::kResultTrue),
        _wasClosed(false)
    {}

    ~Impl()
    {
        if (_window) {
            _window.delegate = nil;
            [_window orderOut:nil];
            [_window close];
            _window = nil;
        }
        if (_frame) {
            delete _frame;
            _frame = nullptr;
        }
        _view = nullptr;
    }

    bool create(int width, int height)
    {
        NSUInteger styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
        if (_resizeable)
            styleMask |= NSWindowStyleMaskResizable;

        NSRect contentRect = NSMakeRect(0, 0, width, height);
        _window = [[NSPanel alloc] initWithContentRect:contentRect styleMask:styleMask backing:NSBackingStoreBuffered
                                                 defer:YES];

        _delegate = [[VST3EditorWindowDelegate alloc] init];
        _delegate.vst3View = _view;
        _delegate.windowObj = _owner;
        _window.delegate = _delegate;

        _window.releasedWhenClosed = NO;
        [_window center];
        [_window setStyleMask:styleMask];
        [_window setTitle:[[NSString alloc] initWithUTF8String:_title.c_str()]];
        [_window setLevel:NSFloatingWindowLevel];
        [_window setHidesOnDeactivate:NO];
        [_window setIsVisible:YES];
        [_window
            setCollectionBehavior:(NSWindowCollectionBehaviorFullScreenAuxiliary |
                                   NSWindowCollectionBehaviorCanJoinAllSpaces | NSWindowCollectionBehaviorTransient)];
        [_window setOpaque:NO];
        [_window setHasShadow:YES];
        [_window setExcludedFromWindowsMenu:YES];
        [_window setAutorecalculatesKeyViewLoop:NO];

        _frame = new PlugFrameImpl(_window);
        NSView *contentView = [_window contentView];

        if (_view) {
            _view->setFrame(_frame);
            if (_view->attached((__bridge void *) contentView, Steinberg::kPlatformTypeNSView) !=
                Steinberg::kResultOk) {
                _view->setFrame(nullptr);
                fail("VST3: Plugin attachment to window failed!");
                return false;
            }

            // Set plugin view size
            Steinberg::ViewRect rect;
            Steinberg::ViewRect vr(0, 0, width, height);
            if (_view->getSize(&rect) == Steinberg::kResultOk &&
                (rect.getWidth() != width || rect.getHeight() != height))
                _view->onSize(&rect);
            else
                _view->onSize(&vr);

            // DPI scaling
            Steinberg::FUnknownPtr<Steinberg::IPlugViewContentScaleSupport> scaleSupport(_view);
            if (scaleSupport) {
                CGFloat dpiScale = [contentView.window backingScaleFactor];
                scaleSupport->setContentScaleFactor(dpiScale);
            }
        }

        return true;
    }

    void show()
    {
        if (_window) {
            [_window makeKeyAndOrderFront:nil];
            [NSApp activateIgnoringOtherApps:YES];

            NSView *contentView = [_window contentView];
            Steinberg::FUnknownPtr<Steinberg::IPlugViewContentScaleSupport> scaleSupport(_view);
            if (scaleSupport && contentView) {
                CGFloat dpiScale = [contentView.window backingScaleFactor];
                scaleSupport->setContentScaleFactor(dpiScale);
            }

            _wasClosed = false;
        }
    }

    void hide()
    {
        if (_window)
            [_window orderOut:nil];
    }

    void setClosedState(bool closed)
    {
        _wasClosed = closed;
    }
    bool getClosedState() const
    {
        return _wasClosed;
    }

      private:
    Steinberg::IPlugView *_view;
    NSWindow *_window;
    PlugFrameImpl *_frame;
    std::string _title;
    VST3EditorWindowDelegate *_delegate;
    VST3EditorWindow *_owner;
    bool _resizeable;
    bool _wasClosed;
    bool _attached = false;
};

//------------------------------------------------------------

VST3EditorWindow::VST3EditorWindow(Steinberg::IPlugView *view, const std::string &title) :
    _impl(new Impl(view, title, this))
{}

VST3EditorWindow::~VST3EditorWindow()
{
    delete _impl;
}

bool VST3EditorWindow::create(int width, int height)
{
    return _impl->create(width, height);
}

void VST3EditorWindow::show()
{
    _impl->show();
}

void VST3EditorWindow::close()
{
    _impl->hide();
}

bool VST3EditorWindow::getClosedState()
{
    return _impl->getClosedState();
}

void VST3EditorWindow::setClosedState(bool closed)
{
    _impl->setClosedState(closed);
}
