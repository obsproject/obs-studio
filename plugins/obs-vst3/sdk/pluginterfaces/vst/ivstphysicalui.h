//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivstphysicalui.h
// Created by  : Steinberg, 06/2018
// Description : VST Physical User Interface support
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/ivstnoteexpression.h"

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpush.h"
//------------------------------------------------------------------------

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** \ingroup vst3typedef */
/**@{*/
/** Physical UI Type */
typedef uint32 PhysicalUITypeID;
/**@}*/

//------------------------------------------------------------------------
/** PhysicalUITypeIDs describes the type of Physical UI (PUI) which could be associated to a note
expression.
\see PhysicalUIMap
*/
enum PhysicalUITypeIDs
{
	/** absolute X position when touching keys of PUIs. Range [0=left, 0.5=middle, 1=right] */
	kPUIXMovement = 0,
	/** absolute Y position when touching keys of PUIs. Range [0=bottom/near, 0.5=center, 1=top/far] */
	kPUIYMovement,
	/** pressing a key down on keys of PUIs. Range [0=No Pressure, 1=Full Pressure] */
	kPUIPressure,

	kPUITypeCount, ///< count of current defined PUIs

	kInvalidPUITypeID = 0xFFFFFFFF ///< indicates an invalid or not initialized PUI type
};

//------------------------------------------------------------------------
/** PhysicalUIMap describes a mapping of a noteExpression Type to a Physical UI Type.
It is used in PhysicalUIMapList.
\see PhysicalUIMapList 
*/
struct PhysicalUIMap
{
	/** This represents the physical UI. /see PhysicalUITypeIDs, this is set by the caller of
	 * getPhysicalUIMapping */
	PhysicalUITypeID physicalUITypeID;

	/** This represents the associated noteExpression TypeID to the given physicalUITypeID. This
	 * will be filled by the plug-in in the call getPhysicalUIMapping, set it to kInvalidTypeID if
	 * no Note Expression is associated to the given PUI. */
	NoteExpressionTypeID noteExpressionTypeID;
};

//------------------------------------------------------------------------
/** PhysicalUIMapList describes a list of PhysicalUIMap
\see INoteExpressionPhysicalUIMapping
*/
struct PhysicalUIMapList
{
	/** Count of entries in the map array, set by the caller of getPhysicalUIMapping. */
	uint32 count;

	/** Pointer to a list of PhysicalUIMap containing count entries. */
	PhysicalUIMap* map;
};

//------------------------------------------------------------------------
/** Extended plug-in interface IEditController for note expression event support: Vst::INoteExpressionPhysicalUIMapping
\ingroup vstIPlug vst3611
- [plug imp]
- [extends IEditController]
- [released: 3.6.11]
- [optional]

With this plug-in interface, the host can retrieve the preferred physical mapping associated to note
expression supported by the plug-in.
When the mapping changes (for example when switching presets) the plug-in needs
to inform the host about it via \ref IComponentHandler::restartComponent (kNoteExpressionChanged).

\section INoteExpressionPhysicalUIMappingExample Example

\code{.cpp}
//------------------------------------------------------------------------
// here an example of how a VST3 plug-in could support this INoteExpressionPhysicalUIMapping interface.
// we need to define somewhere the iids:

//in MyController class declaration
class MyController : public Vst::EditController, public Vst::INoteExpressionPhysicalUIMapping
{
	// ...
	//--- INoteExpressionPhysicalUIMapping ---------------------------------
	tresult PLUGIN_API getPhysicalUIMapping (int32 busIndex, int16 channel, PhysicalUIMapList& list) SMTG_OVERRIDE;
	// ...

	OBJ_METHODS (MyController, Vst::EditController)
	DEFINE_INTERFACES
		// ...
		DEF_INTERFACE (Vst::INoteExpressionPhysicalUIMapping)
	END_DEFINE_INTERFACES (Vst::EditController)
	//...
}

// In mycontroller.cpp
#include "pluginterfaces/vst/ivstnoteexpression.h"

namespace Steinberg {
	namespace Vst {
		DEF_CLASS_IID (INoteExpressionPhysicalUIMapping)
	}
}
//------------------------------------------------------------------------
tresult PLUGIN_API MyController::getPhysicalUIMapping (int32 busIndex, int16 channel, PhysicalUIMapList& list)
{
	if (busIndex == 0 && channel == 0)
	{
		for (uint32 i = 0; i < list.count; ++i)
		{
			NoteExpressionTypeID type = kInvalidTypeID;
			if (kPUIXMovement == list.map[i].physicalUITypeID)
				list.map[i].noteExpressionTypeID = kCustomStart + 1;
			else if (kPUIYMovement == list.map[i].physicalUITypeID)
				list.map[i].noteExpressionTypeID = kCustomStart + 2;
		}
		return kResultTrue;
	}
	return kResultFalse;
}
\endcode
*/
class INoteExpressionPhysicalUIMapping : public FUnknown
{
public:
	/** Fills the list of mapped [physical UI (in) - note expression (out)] for a given bus index
	 * and channel. 
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API getPhysicalUIMapping (int32 busIndex /*in*/, int16 channel /*in*/,
	                                                 PhysicalUIMapList& list /*inout*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (INoteExpressionPhysicalUIMapping, 0xB03078FF, 0x94D24AC8, 0x90CCD303, 0xD4133324)

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpop.h"
//------------------------------------------------------------------------
