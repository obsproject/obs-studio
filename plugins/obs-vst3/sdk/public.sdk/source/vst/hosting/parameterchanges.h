//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/hosting/parameterchanges.h
// Created by  : Steinberg, 03/05/2008.
// Description : VST 3 parameter changes implementation
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/ivstparameterchanges.h"
#include <vector>

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Implementation's example of IParamValueQueue - not threadsave!.
\ingroup hostingBase
*/
class ParameterValueQueue : public IParamValueQueue
{
public:
	//------------------------------------------------------------------------
	ParameterValueQueue (ParamID paramID);
	virtual ~ParameterValueQueue ();

	ParamID PLUGIN_API getParameterId () SMTG_OVERRIDE { return paramID; }
	int32 PLUGIN_API getPointCount () SMTG_OVERRIDE;
	tresult PLUGIN_API getPoint (int32 index, int32& sampleOffset, ParamValue& value) SMTG_OVERRIDE;
	tresult PLUGIN_API addPoint (int32 sampleOffset, ParamValue value, int32& index) SMTG_OVERRIDE;

	void setParamID (ParamID pID) {paramID = pID;}
	void clear ();
	//------------------------------------------------------------------------
	DECLARE_FUNKNOWN_METHODS
protected:
	ParamID paramID;

	struct ParameterQueueValue
	{
		ParameterQueueValue (ParamValue value, int32 sampleOffset) : value (value), sampleOffset (sampleOffset) {}
		ParamValue value;
		int32 sampleOffset;
	};
	std::vector<ParameterQueueValue> values;
};

//------------------------------------------------------------------------
/** Implementation's example of IParameterChanges - not threadsave!.
\ingroup hostingBase
*/
class ParameterChanges : public IParameterChanges
{
public:
	//------------------------------------------------------------------------
	ParameterChanges (int32 maxParameters = 0);
	virtual ~ParameterChanges ();

	void clearQueue ();
	void setMaxParameters (int32 maxParameters);

	//---IParameterChanges-----------------------------
	int32 PLUGIN_API getParameterCount () SMTG_OVERRIDE;
	IParamValueQueue* PLUGIN_API getParameterData (int32 index) SMTG_OVERRIDE;
	IParamValueQueue* PLUGIN_API addParameterData (const ParamID& pid, int32& index) SMTG_OVERRIDE;
	
	//------------------------------------------------------------------------
	DECLARE_FUNKNOWN_METHODS
protected:
	std::vector<IPtr<ParameterValueQueue>> queues;
	int32 usedQueueCount {0};
};


//------------------------------------------------------------------------
/** Ring buffer for transferring parameter changes from a writer to a read thread .
\ingroup hostingBase
*/
class ParameterChangeTransfer
{
public:
	//------------------------------------------------------------------------
	ParameterChangeTransfer (int32 maxParameters = 0);
	virtual ~ParameterChangeTransfer ();

	void setMaxParameters (int32 maxParameters);

	void addChange (ParamID pid, ParamValue value, int32 sampleOffset);
	bool getNextChange (ParamID& pid, ParamValue& value, int32& sampleOffset);

	void transferChangesTo (ParameterChanges& dest);
	void transferChangesFrom (ParameterChanges& source);

	void removeChanges () { writeIndex = readIndex; }

	//------------------------------------------------------------------------
protected:
	struct ParameterChange
	{
		ParamID id;
		ParamValue value;
		int32 sampleOffset;
	};
	int32 size;
	ParameterChange* changes;

	volatile int32 readIndex;
	volatile int32 writeIndex;
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
