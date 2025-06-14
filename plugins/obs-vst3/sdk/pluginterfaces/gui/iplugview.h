//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : SDK GUI Interfaces
// Filename    : pluginterfaces/gui/iplugview.h
// Created by  : Steinberg, 12/2007
// Description : Plug-in User Interface
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/base/typesizecheck.h"

namespace Steinberg {

class IPlugFrame;

//------------------------------------------------------------------------
/*! \defgroup pluginGUI Graphical User Interface
 */

//------------------------------------------------------------------------
/** Graphical rectangle structure. Used with IPlugView.
 * \ingroup pluginGUI
 */
struct ViewRect
{
	ViewRect (int32 l = 0, int32 t = 0, int32 r = 0, int32 b = 0)
	: left (l), top (t), right (r), bottom (b)
	{
	}

	int32 left;
	int32 top;
	int32 right;
	int32 bottom;

	//--- ---------------------------------------------------------------------
	int32 getWidth () const { return right - left; }
	int32 getHeight () const { return bottom - top; }
};

SMTG_TYPE_SIZE_CHECK (ViewRect, 16, 16, 16, 16)

//------------------------------------------------------------------------
/** \defgroup platformUIType Platform UI Types
 * \ingroup pluginGUI
 * List of Platform UI types for IPlugView. This list is used to match the GUI-System between
 * the host and a plug-in in case that an OS provides multiple GUI-APIs.
 */
/**@{*/
/** The parent parameter in IPlugView::attached() is a HWND handle.
 * You should attach a child window to it. */
const FIDString kPlatformTypeHWND = "HWND"; ///< HWND handle. (Microsoft Windows)

/** The parent parameter in IPlugView::attached() is a WindowRef.
 * You should attach a HIViewRef to the content view of the window. */
const FIDString kPlatformTypeHIView = "HIView"; ///< HIViewRef. (Mac OS X)

/** The parent parameter in IPlugView::attached() is a NSView pointer.
 * You should attach a NSView to it. */
const FIDString kPlatformTypeNSView = "NSView"; ///< NSView pointer. (Mac OS X)

/** The parent parameter in IPlugView::attached() is a UIView pointer.
 * You should attach an UIView to it. */
const FIDString kPlatformTypeUIView = "UIView"; ///< UIView pointer. (iOS)

/** The parent parameter in IPlugView::attached() is a X11 Window supporting XEmbed.
 * You should attach a Window to it that supports the XEmbed extension.
 * See https://standards.freedesktop.org/xembed-spec/xembed-spec-latest.html */
const FIDString kPlatformTypeX11EmbedWindowID = "X11EmbedWindowID"; ///< X11 Window ID. (X11)

/** The parent parameter in IPlugView::attached() is a wl_surface pointer.
 * The plug-in should create a wl_surface and attach it to the parent surface as a wl_subsurface.
 * The plug-in should not connect to the system compositor to do so, but use
 * IWaylandHost::openWaylandConnection().
 * See https://wayland.freedesktop.org/docs/html */
const FIDString kPlatformTypeWaylandSurfaceID = "WaylandSurfaceID"; ///< Wayland Surface ID.
/**@}*/
//------------------------------------------------------------------------

//------------------------------------------------------------------------
/** Plug-in definition of a view.
\ingroup pluginGUI vstIPlug vst300
- [plug imp]
- [released: 3.0.0]

\par Coordinates
The coordinates utilized within the ViewRect are native to the view system of the parent type.
This implies that on macOS (kPlatformTypeNSView), the coordinates are expressed in logical 
units (independent of the screen scale factor), whereas on Windows (kPlatformTypeHWND) and 
Linux (kPlatformTypeX11EmbedWindowID), the coordinates are expressed in physical units (pixels).

\par Sizing of a view
Usually, the size of a plug-in view is fixed. But both the host and the plug-in can cause
a view to be resized:
\n
- \b Host: If IPlugView::canResize () returns kResultTrue the host will set up the window
  so that the user can resize it. While the user resizes the window,
  IPlugView::checkSizeConstraint () is called, allowing the plug-in to change the size to a valid
  a valid supported rectangle size. The host then resizes the window to this rect and has to call IPlugView::onSize ().
\n
\n
- \b Plug-in: The plug-in can call IPlugFrame::resizeView () and cause the host to resize the
  window.\n\n
  Afterwards, in the same callstack, the host has to call IPlugView::onSize () if a resize is needed (size was changed).
  Note that if the host calls IPlugView::getSize () before calling IPlugView::onSize () (if needed),
  it will get the current (old) size not the wanted one!!\n
  Here the calling sequence:\n
    - plug-in->host: IPlugFrame::resizeView (newSize)
	- host->plug-in (optional): IPlugView::getSize () returns the currentSize (not the newSize!)
	- host->plug-in: if newSize is different from the current size: IPlugView::onSize (newSize)
    - host->plug-in (optional): IPlugView::getSize () returns the newSize
\n
<b>Please only resize the platform representation of the view when IPlugView::onSize () is
called.</b>

\par Keyboard handling
The plug-in view receives keyboard events from the host. A view implementation must not handle
keyboard events by the means of platform callbacks, but let the host pass them to the view. The host
depends on a proper return value when IPlugView::onKeyDown is called, otherwise the plug-in view may
cause a malfunction of the host's key command handling.

\see IPlugFrame, \ref platformUIType
*/
class IPlugView : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** Is Platform UI Type supported
	    \param type : IDString of \ref platformUIType */
	virtual tresult PLUGIN_API isPlatformTypeSupported (FIDString type) = 0;

	/** The parent window of the view has been created, the (platform) representation of the view
		should now be created as well.
	    Note that the parent is owned by the caller and you are not allowed to alter it in any way
		other than adding your own views.
	    Note that in this call the plug-in could call a IPlugFrame::resizeView ()!
	    \param parent : platform handle of the parent window or view
	    \param type : \ref platformUIType which should be created */
	virtual tresult PLUGIN_API attached (void* parent, FIDString type) = 0;

	/** The parent window of the view is about to be destroyed.
	    You have to remove all your own views from the parent window or view. */
	virtual tresult PLUGIN_API removed () = 0;

	/** Handling of mouse wheel. */
	virtual tresult PLUGIN_API onWheel (float distance) = 0;

	/** Handling of keyboard events : Key Down.
	    \param key : unicode code of key
	    \param keyCode : virtual keycode for non ascii keys - see \ref VirtualKeyCodes in keycodes.h
	    \param modifiers : any combination of modifiers - see \ref KeyModifier in keycodes.h
	    \return kResultTrue if the key is handled, otherwise kResultFalse. \n
	            <b> Please note that kResultTrue must only be returned if the key has really been
	   handled. </b> Otherwise key command handling of the host might be blocked! */
	virtual tresult PLUGIN_API onKeyDown (char16 key, int16 keyCode, int16 modifiers) = 0;

	/** Handling of keyboard events : Key Up.
	    \param key : unicode code of key
	    \param keyCode : virtual keycode for non ascii keys - see \ref VirtualKeyCodes in keycodes.h
	    \param modifiers : any combination of KeyModifier - see \ref KeyModifier in keycodes.h
	    \return kResultTrue if the key is handled, otherwise return kResultFalse. */
	virtual tresult PLUGIN_API onKeyUp (char16 key, int16 keyCode, int16 modifiers) = 0;

	/** Returns the size of the platform representation of the view. */
	virtual tresult PLUGIN_API getSize (ViewRect* size) = 0;

	/** Resizes the platform representation of the view to the given rect. Note that if the plug-in
	 *	requests a resize (IPlugFrame::resizeView ()) onSize has to be called afterward. */
	virtual tresult PLUGIN_API onSize (ViewRect* newSize) = 0;

	/** Focus changed message. */
	virtual tresult PLUGIN_API onFocus (TBool state) = 0;

	/** Sets IPlugFrame object to allow the plug-in to inform the host about resizing. */
	virtual tresult PLUGIN_API setFrame (IPlugFrame* frame) = 0;

	/** Is view sizable by user. */
	virtual tresult PLUGIN_API canResize () = 0;

	/** On live resize this is called to check if the view can be resized to the given rect, if not
	 *	adjust the rect to the allowed size. */
	virtual tresult PLUGIN_API checkSizeConstraint (ViewRect* rect) = 0;
//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IPlugView, 0x5BC32507, 0xD06049EA, 0xA6151B52, 0x2B755B29)

//------------------------------------------------------------------------
/** Callback interface passed to IPlugView.
\ingroup pluginGUI vstIHost vst300
- [host imp]
- [released: 3.0.0]
- [mandatory]

Enables a plug-in to resize the view and cause the host to resize the window.
*/
class IPlugFrame : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** Called to inform the host about the resize of a given view.
	 * Afterwards the host has to call IPlugView::onSize (). */
	virtual tresult PLUGIN_API resizeView (IPlugView* view, ViewRect* newSize) = 0;
//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IPlugFrame, 0x367FAF01, 0xAFA94693, 0x8D4DA2A0, 0xED0882A3)

//------------------------------------------------------------------------
namespace Linux {

using TimerInterval = uint64;
using FileDescriptor = int;

//------------------------------------------------------------------------
/** Linux event handler interface
\ingroup pluginGUI vst368
- [plug imp]
- [released: 3.6.8]
\see IRunLoop
*/
class IEventHandler : public FUnknown
{
public:
	virtual void PLUGIN_API onFDIsSet (FileDescriptor fd) = 0;
//------------------------------------------------------------------------
	static const FUID iid;
};
DECLARE_CLASS_IID (IEventHandler, 0x561E65C9, 0x13A0496F, 0x813A2C35, 0x654D7983)

//------------------------------------------------------------------------
/** Linux timer handler interface
\ingroup pluginGUI vst368
- [plug imp]
- [released: 3.6.8]
\see IRunLoop
*/
class ITimerHandler : public FUnknown
{
public:
	virtual void PLUGIN_API onTimer () = 0;
//------------------------------------------------------------------------
	static const FUID iid;
};
DECLARE_CLASS_IID (ITimerHandler, 0x10BDD94F, 0x41424774, 0x821FAD8F, 0xECA72CA9)

//------------------------------------------------------------------------
/** Linux host run loop interface
\ingroup pluginGUI vst368
- [host imp]
- [extends IPlugFrame]
- [released: 3.6.8]

On Linux the host has to provide this interface to the plug-in as there's no global event run loop
defined as on other platforms. 

This can be done by IPlugFrame and the context which is passed to the plug-in as an argument 
in the method IPlugFactory3::setHostContext. This way the plug-in can get a runloop even if 
it does not have an editor.

A plug-in can register an event handler for a file descriptor. The host has to call the event
handler when the file descriptor is marked readable.

A plug-in also can register a timer which will be called repeatedly until it is unregistered.
*/
class IRunLoop : public FUnknown
{
public:
	virtual tresult PLUGIN_API registerEventHandler (IEventHandler* handler, FileDescriptor fd) = 0;
	virtual tresult PLUGIN_API unregisterEventHandler (IEventHandler* handler) = 0;

	virtual tresult PLUGIN_API registerTimer (ITimerHandler* handler,
											  TimerInterval milliseconds) = 0;
	virtual tresult PLUGIN_API unregisterTimer (ITimerHandler* handler) = 0;
//------------------------------------------------------------------------
	static const FUID iid;
};
DECLARE_CLASS_IID (IRunLoop, 0x18C35366, 0x97764F1A, 0x9C5B8385, 0x7A871389)

//------------------------------------------------------------------------
} // namespace Linux
} // namespace Steinberg
