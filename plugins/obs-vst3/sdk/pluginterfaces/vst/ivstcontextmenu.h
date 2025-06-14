//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivstcontextmenu.h
// Created by  : Steinberg, 10/2010
// Description : VST Context Menu Interfaces
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
#include "pluginterfaces/vst/vsttypes.h"

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpush.h"
//------------------------------------------------------------------------

namespace Steinberg {
class IPlugView;

namespace Vst {
class IContextMenu;

//------------------------------------------------------------------------
/** Extended host callback interface Vst::IComponentHandler3 for an edit controller.
\ingroup vstIHost vst350
- [host imp]
- [extends IComponentHandler]
- [released: 3.5.0]
- [optional]

A plug-in can ask the host to create a context menu for a given exported parameter ID or a generic
context menu.\n

The host may pre-fill this context menu with specific items regarding the parameter ID like "Show
automation for parameter", "MIDI learn" etc...\n

The plug-in can use the context menu in two ways :
- add its own items to the menu via the IContextMenu interface and call IContextMenu::popup(..) to
create the pop-up. See the \ref IContextMenuExample.
- extract the host menu items and add them to a context menu created by the plug-in.

\b Note: You can and should use this even if you do not add your own items to the menu as this is
considered to be a big user value.

\sa IContextMenu
\sa IContextMenuTarget

\section IContextMenuExample Examples
- For example, Cubase adds its owned entries in the context menu opened with right-click on an
exported parameter when the plug-in uses createContextMenu. \image html "contextmenuexample.png" \n
- Adding plug-in specific items to the context menu:

\code{.cpp}
//------------------------------------------------------------------------
class PluginContextMenuTarget : public IContextMenuTarget, public FObject
{
public:
    PluginContextMenuTarget () {}

    tresult PLUGIN_API executeMenuItem (int32 tag) override
    {
        // this will be called if the user has executed one of the menu items of the plug-in.
        // It will not be called for items of the host.
        switch (tag)
        {
            case 1: break;
            case 2: break;
        }
        return kResultTrue;
    }

    OBJ_METHODS(PluginContextMenuTarget, FObject)
    DEFINE_INTERFACES
        DEF_INTERFACE (IContextMenuTarget)
    END_DEFINE_INTERFACES (FObject)
    REFCOUNT_METHODS(FObject)
};

// The following is the code to create the context menu
void popupContextMenu (IComponentHandler* componentHandler, IPlugView* view, const ParamID* paramID,
UCoord x, UCoord y)
{
    if (componentHandler == 0 || view == 0)
        return;
    FUnknownPtr<IComponentHandler3> handler (componentHandler);
    if (handler == 0)
        return;
    if (IContextMenu* menu = handler->createContextMenu (view, paramID))
    {
        // here you can add your entries (optional)
        PluginContextMenuTarget* target = new PluginContextMenuTarget ();
        


        IContextMenu::Item item = {0};
        UString128 ("My Item 1").copyTo (item.name, 128);
        item.tag = 1;
        menu->addItem (item, target);

        UString128 ("My Item 2").copyTo (item.name, 128);
        item.tag = 2;
        menu->addItem (item, target);
        target->release ();
        //--end of adding new entries
        


        // here the the context menu will be pop-up (and it waits a user interaction)
        menu->popup (x, y);
        menu->release ();
    }
}
\endcode
*/
class IComponentHandler3 : public FUnknown
{
public:
	/** Creates a host context menu for a plug-in:
	 *   - If paramID is zero, the host may create a generic context menu.
	 *   - The IPlugView object must be valid.
	 *   - The return IContextMenu object needs to be released afterwards by the plug-in.
	 * 
	 * \note [UI-thread & (Initialized | Connected) & plugView] */
	virtual IContextMenu* PLUGIN_API createContextMenu (IPlugView* plugView /*in*/,
	                                                    const ParamID* paramID /*in*/) = 0;
//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IComponentHandler3, 0x69F11617, 0xD26B400D, 0xA4B6B964, 0x7B6EBBAB)
	
//------------------------------------------------------------------------
/** Context Menu Item Target interface: Vst::IContextMenuTarget
\ingroup vstIHost vstIPlug vst350
- [host imp]
- [plug imp]
- [released: 3.5.0]
- [optional]

A receiver of a menu item should implement this interface, which will be called after the user has selected
this menu item.

\see IComponentHandler3 for more information.
*/
class IContextMenuTarget : public FUnknown
{
public:
	/** Called when an menu item was executed.
	 * \note [UI-thread & (Initialized | Connected) & plugView] */
	virtual tresult PLUGIN_API executeMenuItem (int32 tag /*in*/) = 0;
	//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IContextMenuTarget, 0x3CDF2E75, 0x85D34144, 0xBF86D36B, 0xD7C4894D)

//------------------------------------------------------------------------
/** IContextMenuItem is an entry element of the context menu. */
struct IContextMenuItem
{
	String128 name;									///< Name of the item
	int32 tag;										///< Identifier tag of the item
	int32 flags;									///< Flags of the item

	enum Flags {
		kIsSeparator	= 1 << 0,					///< Item is a separator
		kIsDisabled		= 1 << 1,					///< Item is disabled
		kIsChecked		= 1 << 2,					///< Item is checked
		kIsGroupStart	= 1 << 3 | kIsDisabled,		///< Item is a group start (like sub folder)
		kIsGroupEnd		= 1 << 4 | kIsSeparator,	///< Item is a group end
	};
};
//------------------------------------------------------------------------
/** Context Menu interface: Vst::IContextMenu
\ingroup vstIHost vst350
- [host imp]
- [create with IComponentHandler3::createContextMenu(..)]
- [released: 3.5.0]
- [optional]

A context menu is composed of Item (entry). A Item is defined by a name, a tag, a flag
and a associated target (called when this item will be selected/executed). 
With IContextMenu the plug-in can retrieve a Item, add a Item, remove a Item and pop-up the menu.

\see IComponentHandler3 for more information.
*/
class IContextMenu : public FUnknown
{
public:
	typedef IContextMenuItem Item;
	
	/** Gets the number of menu items.
	 * \note [UI-thread] */
	virtual int32 PLUGIN_API getItemCount () = 0;

	/** Gets a menu item and its target (target could be not assigned).
	 * \note [UI-thread] */
	virtual tresult PLUGIN_API getItem (int32 index /*in*/, Item& item /*out*/,
	                                    IContextMenuTarget** target /*out*/) = 0;

	/** Adds a menu item and its target.
	 * \note [UI-thread] */
	virtual tresult PLUGIN_API addItem (const Item& item /*in*/,
	                                    IContextMenuTarget* target /*in*/) = 0;

	/** Removes a menu item.
	 * \note [UI-thread] */
	virtual tresult PLUGIN_API removeItem (const Item& item /*in*/,
	                                       IContextMenuTarget* target /*in*/) = 0;

	/** Pop-ups the menu. Coordinates are relative to the top-left position of the plug-ins view.
	 * \note [UI-thread] */
	virtual tresult PLUGIN_API popup (UCoord x /*in*/, UCoord y /*in*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IContextMenu, 0x2E93C863, 0x0C9C4588, 0x97DBECF5, 0xAD17817D)

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpop.h"
//------------------------------------------------------------------------
