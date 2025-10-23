//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivstparameterfunctionname.h
// Created by  : Steinberg, 03/2020
// Description : VST Parameter Function Name Interface
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

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {
namespace FunctionNameType {
//--------------------------------------------------------------------
	const CString kCompGainReduction			= "Comp:GainReduction"; /**  */
	const CString kCompGainReductionMax			= "Comp:GainReductionMax";
	const CString kCompGainReductionPeakHold	= "Comp:GainReductionPeakHold";
	const CString kCompResetGainReductionMax	= "Comp:ResetGainReductionMax";

    const CString kLowLatencyMode = "LowLatencyMode"; /** Useful for live situation where low
														 latency is required:
														 0 means LowLatency disable,
														 1 means LowLatency enable */
    const CString kDryWetMix = "DryWetMix"; /** Allowing to mix the original (Dry) Signal with the processed one (Wet):
											0.0 means Dry Signal only,
											0.5 means 50% Dry Signal + 50% Wet Signal,
											1.0 means Wet Signal only */
    const CString kRandomize = "Randomize"; /**	Allow to assign some randomized values to some
                                               parameters in a controlled way*/

	/// Panner Type
	const CString kPanPosCenterX = "PanPosCenterX";	///< Gravity point X-axis [0, 1]=>[L-R] (for stereo: middle between left and right)
	const CString kPanPosCenterY = "PanPosCenterY";	///< Gravity point Y-axis [0, 1]=>[Front-Rear]
	const CString kPanPosCenterZ = "PanPosCenterZ";	///< Gravity point Z-axis [0, 1]=>[Bottom-Top]


} // FunctionNameType

//------------------------------------------------------------------------
/** Edit controller component interface extension: Vst::IParameterFunctionName
\ingroup vstIPlug vst370
- [plug imp]
- [extends IEditController]
- [released: 3.7.0]
- [optional]

This interface allows the host to get a parameter associated to a specific meaning (a functionName)
for a given unit. The host can use this information, for example, for drawing a Gain Reduction meter
in its own UI. In order to get the plain value of this parameter, the host should use the
IEditController::normalizedParamToPlain. The host can automatically map parameters to dedicated UI
controls, such as the wet-dry mix knob or Randomize button.

\section IParameterFunctionNameExample Example

\code{.cpp}
//------------------------------------------------------------------------
// here an example of how a VST3 plug-in could support this IParameterFunctionName interface.
// we need to define somewhere the iids:

// in MyController class declaration
class MyController : public Vst::EditController, public Vst::IParameterFunctionName
{
    // ...
    tresult PLUGIN_API getParameterIDFromFunctionName (UnitID unitID, FIDString functionName,
                                                       Vst::ParamID& paramID) override;
    // ...

    OBJ_METHODS (MyController, Vst::EditController)
    DEFINE_INTERFACES
        // ...
        DEF_INTERFACE (Vst::IParameterFunctionName)
    END_DEFINE_INTERFACES (Vst::EditController)
    DELEGATE_REFCOUNT (Vst::EditController)
    // ...
}

#include "ivstparameterfunctionname.h"
namespace Steinberg {
namespace Vst {
    DEF_CLASS_IID (IParameterFunctionName)
}
}

//------------------------------------------------------------------------
tresult PLUGIN_API MyController::getParameterIDFromFunctionName (UnitID unitID, FIDString
functionName, Vst::ParamID& paramID)
{
    using namespace Vst;

    paramID = kNoParamId;

    if (unitID == kRootUnitId && FIDStringsEqual (functionName, kCompGainReduction))
        paramID = kMyGainReductionId;

    return (paramID != kNoParamId) ? kResultOk : kResultFalse;
}

//--- a host implementation example: --------------------
// ...
FUnknownPtr<Vst::IParameterFunctionName> functionName (mEditController->getIEditController ());
if (functionName)
{
    Vst::ParamID paramID;
    if (functionName->getParameterIDFromFunctionName (kRootUnitId,
                       Vst::FunctionNameType::kCompGainReduction, paramID) == kResultTrue)
    {
        // paramID could be cached for performance issue
        ParamValue norm = mEditController->getIEditController ()->getParamNormalized (paramID);
        ParamValue plain = mEditController->getIEditController ()->normalizedParamToPlain (paramID, norm);
        // plain is something like -6 (-6dB)
    }
}
\endcode
*/
class IParameterFunctionName : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** Gets for the given unitID the associated paramID to a function Name.
	 * Returns kResultFalse when no found parameter (paramID is set to kNoParamId in this case).
	 * \note [UI-thread & (Initialized | Connected)] */
	virtual tresult PLUGIN_API getParameterIDFromFunctionName (UnitID unitID /*in*/,
	                                                           FIDString functionName /*in*/,
	                                                           ParamID& paramID /*inout*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IParameterFunctionName, 0x6D21E1DC, 0x91199D4B, 0xA2A02FEF, 0x6C1AE55C)

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpop.h"
//------------------------------------------------------------------------
