//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : SDK GUI Interfaces
// Filename    : pluginterfaces/gui/iplugviewcontentscalesupport.h
// Created by  : Steinberg, 06/2016
// Description : Plug-in User Interface Scaling
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

//------------------------------------------------------------------------
namespace Steinberg {

//------------------------------------------------------------------------
/** Plug-in view content scale support
\ingroup pluginGUI vstIPlug vst366
- [plug impl]
- [extends IPlugView]
- [released: 3.6.6]
- [optional]

This interface communicates the content scale factor from the host to the plug-in view on
systems where plug-ins cannot get this information directly like Microsoft Windows.

The host calls setContentScaleFactor directly before or after the plug-in view is attached and when
the scale factor changes while the view is attached (system change or window moved to another screen
with different scaling settings).

The host may call setContentScaleFactor in a different context, for example: scaling the plug-in
editor for better readability.

When a plug-in handles this (by returning kResultTrue), it needs to scale the width and height of
its view by the scale factor and inform the host via a IPlugFrame::resizeView (). The host will then
call IPlugView::onSize ().

Note that the host is allowed to call setContentScaleFactor() at any time the IPlugView is valid.
If this happens before the IPlugFrame object is set on your view, make sure that when the host calls
IPlugView::getSize() afterwards you return the size of your view for that new scale factor.

It is recommended to implement this interface on Microsoft Windows to let the host know that the
plug-in is able to render in different scalings.
*/
class IPlugViewContentScaleSupport : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** \ingroup smtgtypedef */
	typedef float ScaleFactor;

	/** Set the Content Scale Factor
	* @param factor the scale factor requested by the host
	* @return kResultTrue when a plug-in handles this
	* \note [UI-thread] */
	virtual tresult PLUGIN_API setContentScaleFactor (ScaleFactor factor /*in*/) = 0;
//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IPlugViewContentScaleSupport, 0x65ED9690, 0x8AC44525, 0x8AADEF7A, 0x72EA703F)

//------------------------------------------------------------------------
} // namespace Steinberg

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpop.h"
//------------------------------------------------------------------------
