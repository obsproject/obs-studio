//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivstmidilearn.h
// Created by  : Steinberg, 11/2018
// Description : VST MIDI Learn
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
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** MIDI Learn interface: Vst::IMidiLearn
\ingroup vstIPlug vst3612
- [plug imp]
- [extends IEditController]
- [released: 3.6.12]
- [optional]

If this interface is implemented by the edit controller, the host will call this method whenever
there is live MIDI-CC input for the plug-in. This way, the plug-in can change its MIDI-CC parameter
mapping and notify the host using the IComponentHandler::restartComponent with the
kMidiCCAssignmentChanged flag.
Use this if you want to implement custom MIDI-Learn functionality in your plug-in.

\code{.cpp}
//------------------------------------------------
// in MyController class declaration
class MyController : public Vst::EditController, public Vst::IMidiLearn
{
    // ...
    //--- IMidiLearn ---------------------------------
    tresult PLUGIN_API onLiveMIDIControllerInput (int32 busIndex, int16 channel,
                                                  CtrlNumber midiCC) SMTG_OVERRIDE;
    // ...

    OBJ_METHODS (MyController, Vst::EditController)
    DEFINE_INTERFACES
        // ...
        DEF_INTERFACE (Vst::IMidiLearn)
    END_DEFINE_INTERFACES (Vst::EditController)
    //...
}

//------------------------------------------------
// in mycontroller.cpp
#include "pluginterfaces/vst/ivstmidilearn.h

namespace Steinberg {
    namespace Vst {
        DEF_CLASS_IID (IMidiLearn)
    }
}

//------------------------------------------------------------------------
tresult PLUGIN_API MyController::onLiveMIDIControllerInput (int32 busIndex,
                            int16 channel, CtrlNumber midiCC)
{
    // if we are not in doMIDILearn (triggered by a UI button for example)
    // or wrong channel then return
    if (!doMIDILearn || busIndex != 0 || channel != 0 || midiLearnParamID == InvalidParamID)
        return kResultFalse;

    // adapt our internal MIDICC -> parameterID mapping
    midiCCMapping[midiCC] = midiLearnParamID;

    // new mapping then inform the host that our MIDI assignment has changed
    if (auto componentHandler = getComponentHandler ())
    {
        componentHandler->restartComponent (kMidiCCAssignmentChanged);
    }
    return kResultTrue;
}
\endcode
*/
class IMidiLearn : public FUnknown
{
public:
	/** Called on live input MIDI-CC change associated to a given bus index and MIDI channel.
	 * \note [UI-thread & (Initialized | Connected)] */
	virtual tresult PLUGIN_API onLiveMIDIControllerInput (int32 busIndex /*in*/,
	                                                      int16 channel /*in*/,
	                                                      CtrlNumber midiCC /*in*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IMidiLearn, 0x6B2449CC, 0x419740B5, 0xAB3C79DA, 0xC5FE5C86)

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
