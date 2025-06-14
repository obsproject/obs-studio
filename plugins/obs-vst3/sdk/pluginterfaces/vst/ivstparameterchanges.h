//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivstparameterchanges.h
// Created by  : Steinberg, 09/2005
// Description : VST Parameter Change Interfaces
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

//----------------------------------------------------------------------
namespace Steinberg {
namespace Vst {
//----------------------------------------------------------------------
/** Queue of changes for a specific parameter: Vst::IParamValueQueue
\ingroup vstIHost vst300
- [host imp]
- [released: 3.0.0]
- [mandatory]

The change queue can be interpreted as segment of an automation curve. For each
processing block, a segment with the size of the block is transmitted to the processor.
The curve is expressed as sampling points of a linear approximation of
the original automation curve. If the original already is a linear curve, it can
be transmitted precisely. A non-linear curve has to be converted to a linear
approximation by the host. Every point of the value queue defines a linear
section of the curve as a straight line from the previous point of a block to
the new one. So the plug-in can calculate the value of the curve for any sample
position in the block.

<b>Implicit Points:</b> \n
In each processing block, the section of the curve for each parameter is transmitted.
In order to reduce the amount of points, the point at block position 0 can be omitted.
- If the curve has a slope of 0 over a period of multiple blocks, only one point is
transmitted for the block where the constant curve section starts. The queue for the following
blocks will be empty as long as the curve slope is 0.
- If the curve has a constant slope other than 0 over the period of several blocks, only
the value for the last sample of the block is transmitted. In this case, the last valid point
is at block position -1. The processor can calculate the value for each sample in the block
by using a linear interpolation:

\code{.cpp}
//------------------------------------------------------------------------
double x1 = -1; // position of last point related to current buffer
double y1 = currentParameterValue; // last transmitted value

int32 pointTime = 0;
ParamValue pointValue = 0;
IParamValueQueue::getPoint (0, pointTime, pointValue);

double x2 = pointTime;
double y2 = pointValue;

double slope = (y2 - y1) / (x2 - x1);
double offset = y1 - (slope * x1);

double curveValue = (slope * bufferTime) + offset; // bufferTime is any position in buffer
\endcode

\b Jumps:
\n
A jump in the automation curve has to be transmitted as two points: one with the
old value and one with the new value at the next sample position.

\image html "automation.jpg"

See \ref IParameterChanges, \ref ProcessData
*/
class IParamValueQueue : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** Returns its associated ID. */
	virtual ParamID PLUGIN_API getParameterId () = 0;

	/** Returns count of points in the queue. */
	virtual int32 PLUGIN_API getPointCount () = 0;

	/** Gets the value and offset at a given index. */
	virtual tresult PLUGIN_API getPoint (int32 index /*in*/, int32& sampleOffset /*out*/,
	                                     ParamValue& value /*out*/) = 0;

	/** Adds a new value at the end of the queue, its index is returned. */
	virtual tresult PLUGIN_API addPoint (int32 sampleOffset /*in*/, ParamValue value /*in*/,
	                                     int32& index /*out*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IParamValueQueue, 0x01263A18, 0xED074F6F, 0x98C9D356, 0x4686F9BA)

//----------------------------------------------------------------------
/** All parameter changes of a processing block: Vst::IParameterChanges
\ingroup vstIHost vst300
- [host imp]
- [released: 3.0.0]
- [mandatory]

This interface is used to transmit any changes to be applied to parameters
in the current processing block. A change can be caused by GUI interaction as
well as automation. They are transmitted as a list of queues (\ref IParamValueQueue)
containing only queues for parameters that actually did change.
See \ref IParamValueQueue, \ref ProcessData
*/
class IParameterChanges : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** Returns count of Parameter changes in the list. */
	virtual int32 PLUGIN_API getParameterCount () = 0;

	/** Returns the queue at a given index. */
	virtual IParamValueQueue* PLUGIN_API getParameterData (int32 index /*in*/) = 0;

	/** Adds a new parameter queue with a given ID at the end of the list,
	returns it and its index in the parameter changes list. */
	virtual IParamValueQueue* PLUGIN_API addParameterData (const ParamID& id /*in*/,
	                                                       int32& index /*out*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IParameterChanges, 0xA4779663, 0x0BB64A56, 0xB44384A8, 0x466FEB9D)

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpop.h"
//------------------------------------------------------------------------
