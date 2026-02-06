/******************************************************************************
    Copyright (C) 2025-2026 pkv <pkv@obsproject.com>
    This file is part of obs-vst3.
    It uses the Steinberg VST3 SDK, which is licensed under MIT license.
    See https://github.com/steinbergmedia/vst3sdk for details.
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
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
    explicit PlugFrameImpl(NSWindow *window) : window_(window) {}

    Steinberg::tresult PLUGIN_API resizeView(Steinberg::IPlugView *, Steinberg::ViewRect *newSize) override
    {
        if (window_ && newSize) {
            [window_ setContentSize:NSMakeSize(newSize->getWidth(), newSize->getHeight())];
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
    NSWindow *window_;
};

//------------------------------------------------------------

class VST3EditorWindow::Impl
{
      public:
    Impl(Steinberg::IPlugView *view, const std::string &title, VST3EditorWindow *owner) :
        view_(view),
        window_(nil),
        frame_(nullptr),
        title_(title),
        delegate_(nil),
        owner_(owner),
        resizeable_(view->canResize() == Steinberg::kResultTrue),
        wasClosed_(false)
    {}

    ~Impl()
    {
        if (window_) {
            window_.delegate = nil;
            [window_ orderOut:nil];
            [window_ close];
            window_ = nil;
        }
        if (frame_) {
            delete frame_;
            frame_ = nullptr;
        }
        view_ = nullptr;
    }

    bool create(int width, int height)
    {
        NSUInteger styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
        if (resizeable_)
            styleMask |= NSWindowStyleMaskResizable;

        NSRect contentRect = NSMakeRect(0, 0, width, height);
        window_ = [[NSPanel alloc] initWithContentRect:contentRect styleMask:styleMask backing:NSBackingStoreBuffered
                                                 defer:YES];

        delegate_ = [[VST3EditorWindowDelegate alloc] init];
        delegate_.vst3View = view_;
        delegate_.windowObj = owner_;
        window_.delegate = delegate_;

        window_.releasedWhenClosed = NO;
        [window_ center];
        [window_ setStyleMask:styleMask];
        [window_ setTitle:[[NSString alloc] initWithUTF8String:title_.c_str()]];
        [window_ setLevel:NSFloatingWindowLevel];
        [window_ setHidesOnDeactivate:NO];
        [window_ setIsVisible:YES];
        [window_
            setCollectionBehavior:(NSWindowCollectionBehaviorFullScreenAuxiliary |
                                   NSWindowCollectionBehaviorCanJoinAllSpaces | NSWindowCollectionBehaviorTransient)];
        [window_ setOpaque:NO];
        [window_ setHasShadow:YES];
        [window_ setExcludedFromWindowsMenu:YES];
        [window_ setAutorecalculatesKeyViewLoop:NO];

        frame_ = new PlugFrameImpl(window_);
        NSView *contentView = [window_ contentView];

        if (view_) {
            view_->setFrame(frame_);
            if (view_->attached((__bridge void *) contentView, Steinberg::kPlatformTypeNSView) !=
                Steinberg::kResultOk) {
                view_->setFrame(nullptr);
                fail("VST3: Plugin attachment to window failed!");
                return false;
            }

            // Set plugin view size
            Steinberg::ViewRect rect;
            Steinberg::ViewRect vr(0, 0, width, height);
            if (view_->getSize(&rect) == Steinberg::kResultOk &&
                (rect.getWidth() != width || rect.getHeight() != height))
                view_->onSize(&rect);
            else
                view_->onSize(&vr);

            // DPI scaling
            Steinberg::FUnknownPtr<Steinberg::IPlugViewContentScaleSupport> scaleSupport(view_);
            if (scaleSupport) {
                CGFloat dpiScale = [contentView.window backingScaleFactor];
                scaleSupport->setContentScaleFactor(dpiScale);
            }
        }
        [window_ makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];

        return true;
    }

    void show()
    {
        if (window_) {
            [window_ makeKeyAndOrderFront:nil];
            [NSApp activateIgnoringOtherApps:YES];

            NSView *contentView = [window_ contentView];
            Steinberg::FUnknownPtr<Steinberg::IPlugViewContentScaleSupport> scaleSupport(view_);
            if (scaleSupport && contentView) {
                CGFloat dpiScale = [contentView.window backingScaleFactor];
                scaleSupport->setContentScaleFactor(dpiScale);
            }

            wasClosed_ = false;
        }
    }

    void hide()
    {
        if (window_)
            [window_ orderOut:nil];
    }

    void setClosedState(bool closed)
    {
        wasClosed_ = closed;
    }
    bool getClosedState() const
    {
        return wasClosed_;
    }

      private:
    Steinberg::IPlugView *view_;
    NSWindow *window_;
    PlugFrameImpl *frame_;
    std::string title_;
    VST3EditorWindowDelegate *delegate_;
    VST3EditorWindow *owner_;
    bool resizeable_;
    bool wasClosed_;
};

//------------------------------------------------------------

VST3EditorWindow::VST3EditorWindow(Steinberg::IPlugView *view, const std::string &title) :
    impl_(new Impl(view, title, this))
{}

VST3EditorWindow::~VST3EditorWindow()
{
    delete impl_;
}

bool VST3EditorWindow::create(int width, int height)
{
    return impl_->create(width, height);
}

void VST3EditorWindow::show()
{
    impl_->show();
}

void VST3EditorWindow::close()
{
    impl_->hide();
}

bool VST3EditorWindow::getClosedState()
{
    return impl_->getClosedState();
}

void VST3EditorWindow::setClosedState(bool closed)
{
    impl_->setClosedState(closed);
}
