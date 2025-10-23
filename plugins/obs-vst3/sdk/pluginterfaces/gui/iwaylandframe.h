//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/gui/iwaylandframe.h
// Created by  : Steinberg, 10/2025
//				 Originally written and contributed to VST SDK by PreSonus Software Ltd.
// Description : Wayland Support.
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

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpush.h"
//------------------------------------------------------------------------

struct wl_display;
struct wl_surface;
struct xdg_surface;
struct xdg_toplevel;

namespace Steinberg {
struct ViewRect;

/** \defgroup waylandFrame Wayland Frame
 *
 * The following interfaces allow querying information about the host plug-in frame when running in
 * a Wayland session.
 *
 * A native Wayland host application acts as both a Wayland client and a Wayland compositor.
 * The host application connects to the system compositor and creates application windows etc. using
 * this compositor connection.\n
 * A plug-in does not connect to the system compositor, but connects to the host application by
 * calling IWaylandHost::openWaylandConnection().\n
 * The IWaylandHost interface can be created via IHostApplication::createInstance. As the interface
 * may be required early, the host should pass IHostApplication to the plug-in using
 * IPluginFactory3::setHostContext.\n
 * When opening a plug-in window, the host calls IPlugView::attached() with the parent pointer set
 * to the wl_surface of the parent frame (with an unknown surface role).
 * The plug-in creates a wl_surface and must assign the wl_subsurface role using the given parent
 * pointer.\n
 * The plug-in is responsible for resizing the subsurface accordingly.\n
 * In order to create additional windows (dialogs, menus, tooltips etc.), the plug-in can use the
 * IWaylandFrame interface, which is implemented by the host's IPlugFrame object.\n
 * The plug-in can use IWaylandFrame::getParentSurface() to query an xdg_surface, which can in turn
 * be used as a parent in xdg_surface_get_popup. Likewise, the plug-in can use
 * IWaylandFrame::getParentToplevel() to query an xdg_toplevel, which can be used in
 * xdg_toplevel_set_parent.
 */

//------------------------------------------------------------------------
/** IWaylandHost: Wayland host interface
 Implemented as a singleton in the host application.
 Created via IHostApplication::createInstance.

\ingroup waylandFrame pluginGUI vst380
- [host imp]
- [released: 3.8.0]
*/
class IWaylandHost : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** Open a Wayland connection to the host.
	 * \note [UI-thread & Initialized] */
	virtual wl_display* PLUGIN_API openWaylandConnection () = 0;

	/** Close a previously created connection.
	 * \note [UI-thread & Initialized] */
	virtual tresult PLUGIN_API closeWaylandConnection (wl_display* display) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IWaylandHost, 0x5E9582EE, 0x86594652, 0xB213678E, 0x7F1A705E)

//------------------------------------------------------------------------
/** IWaylandFrame interface
Interface to query additional information about the host plug-in frame in a Wayland session.
To be implemented by the VST3 IPlugFrame class.

\ingroup waylandFrame pluginGUI vst380
- [host imp]
- [released: 3.8.0]
\see IPlugFrame
*/
class IWaylandFrame : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** Get the parent Wayland surface.
	 * The plug-in must not change the state of the parent surface.
	 * \note [UI-thread & plugView] */
	virtual wl_surface* PLUGIN_API getWaylandSurface (wl_display* display) = 0;

	/** Get the parent XDG surface for creating popup windows.
	 * If the parent surface is not an xdg_surface,
	 *   this returns the first xdg_surface that can be found in the surface hierarchy,
	 *   starting the search with the parent surface.
	 * The plug-in must not change the state of the parent surface.
	 * The size and position of the parent surface, relative to the top left corner of
	 *   the plug-in surface, is returned in parentSize.
	 * \note [UI-thread & plugView] */
	virtual xdg_surface* PLUGIN_API getParentSurface (ViewRect& parentSize,
	                                                  wl_display* display) = 0;

	/** Get the XDG toplevel surface containing the plug-in frame.
	 * The plug-in must not change the state of the returned xdg_toplevel.
	 * \note [UI-thread & plugView] */
	virtual xdg_toplevel* PLUGIN_API getParentToplevel (wl_display* display) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IWaylandFrame, 0x809FAEC6, 0x231C4FFA, 0x98ED046C, 0x6E9E2003)

//------------------------------------------------------------------------
} // namespace Steinberg

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpop.h"
//------------------------------------------------------------------------
