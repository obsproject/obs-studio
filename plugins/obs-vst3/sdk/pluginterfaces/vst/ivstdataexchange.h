//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivstdataexchange.h
// Created by  : Steinberg, 06/2022
// Description : VST Data Exchange Interface
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
namespace Vst {

//------------------------------------------------------------------------
class IAudioProcessor;

//------------------------------------------------------------------------
/** \ingroup vst3typedef */
/**@{*/
typedef uint32 DataExchangeQueueID;
typedef uint32 DataExchangeBlockID;
typedef uint32 DataExchangeUserContextID;
/**@}*/

//------------------------------------------------------------------------
static SMTG_CONSTEXPR DataExchangeQueueID InvalidDataExchangeQueueID = kMaxInt32;
static SMTG_CONSTEXPR DataExchangeBlockID InvalidDataExchangeBlockID = kMaxInt32;

//------------------------------------------------------------------------
struct DataExchangeBlock
{
	/** pointer to the memory buffer */
	void* data;
	/** size of the memory buffer */
	uint32 size;
	/** block identifier */
	DataExchangeBlockID blockID;
};

//------------------------------------------------------------------------
/** Host Data Exchange handler interface: Vst::IDataExchangeHandler
\ingroup vstHost vst379
- [host imp]
- [context interface]
- [released: 3.7.9]
- [optional]

The IDataExchangeHandler implements a direct and thread-safe connection from the realtime
audio context of the audio processor to the non-realtime audio context of the edit controller.
This should be used when the edit controller needs continuous data from the audio process for
visualization or other use-cases. To circumvent the bottleneck on the main thread it is possible
to configure the connection in a way that the calls to the edit controller will happen on a
background thread.

Opening a queue:
The main operation for a plug-in is to open a queue via the handler before the plug-in is activated
(but it must be connected to the edit controller via the IConnectionPoint when the plug-in is using
the recommended separation of edit controller and audio processor). The best place to do this is in
the IAudioProcessor::setupProcessing method as this is also the place where the plug-in knows the
sample rate and maximum block size which the plug-in may need to calculate the queue block size.
When a queue is opened the edit controller gets a notification about it and the controller can
decide if it wishes to receive the data on the main thread or the background thread.

Sending data:
In the IAudioProcessor::process call the plug-in can now lock a block from the handler, fill it and
when done free the block via the handler which then sends the block to the edit controller. The edit
controller then receives the block either on the main thread or on a background thread depending on
the setup of the queue.
The host guarantees that all blocks are send before the plug-in is deactivated.

Closing a queue:
The audio processor must close an opened queue and this has to be done after the processor was
deactivated and before it is disconnected from the edit controller (see IConnectionPoint).

What to do when the queue is full and no block can be locked?
The plug-in needs to be prepared for this situation as constraints in the overall system may cause
the queue to get full. If you need to get this information to the controller you can declare a
hidden parameter which you set to a special value and send this parameter change in your audio
process method.
*/
class IDataExchangeHandler : public FUnknown
{
public:
	/** open a new queue
	 *
	 *	only allowed to be called from the main thread when the component is not active but
	 *	initialized and connected (see IConnectionPoint)
	 *
	 *	@param processor the processor who wants to open the queue
	 *	@param blockSize size of one block
	 *	@param numBlocks number of blocks in the queue
	 *	@param alignment data alignment, if zero will use the platform default alignment if any
	 *	@param userContextID an identifier internal to the processor
	 *	@param outID on return the ID of the queue
	 *	@return kResultTrue on success
	 */
	virtual tresult PLUGIN_API openQueue (IAudioProcessor* processor, uint32 blockSize,
	                                      uint32 numBlocks, uint32 alignment,
	                                      DataExchangeUserContextID userContextID,
	                                      DataExchangeQueueID* outID) = 0;
	/** close a queue
	 *
	 *	closes and frees all memory of a previously opened queue
	 *	if there are locked blocks in the queue, they are freed and made invalid
	 *
	 *	only allowed to be called from the main thread when the component is not active but
	 *	initialized and connected
	 *
	 *	@param queueID the ID of the queue to close
	 *	@return kResultTrue on success
	 */
	virtual tresult PLUGIN_API closeQueue (DataExchangeQueueID queueID) = 0;

	/** lock a block if available
	 *
	 *	only allowed to be called from within the IAudioProcessor::process call
	 *
	 *	@param queueID the ID of the queue
	 *	@param block on return will contain the data pointer and size of the block
	 *	@return kResultTrue if a free block was found and kOutOfMemory if all blocks are locked
	 */
	virtual tresult PLUGIN_API lockBlock (DataExchangeQueueID queueId,
	                                      DataExchangeBlock* block) = 0;

	/**	free a previously locked block
	 *
	 *	only allowed to be called from within the IAudioProcessor::process call
	 *
	 *	@param queueID the ID of the queue
	 *	@param blockID the ID of the block
	 *	@param sendToController	if true the block data will be send to the IEditController otherwise
	 *							it will be discarded
	 *	@return kResultTrue on success
	 */
	virtual tresult PLUGIN_API freeBlock (DataExchangeQueueID queueId, DataExchangeBlockID blockID,
	                                      TBool sendToController) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IDataExchangeHandler, 0x36D551BD, 0x6FF54F08, 0xB48E830D, 0x8BD5A03B)

//------------------------------------------------------------------------
/** Data Exchange Receiver interface: Vst::IDataExchangeReceiver
\ingroup vstPlug vst379
- [plug imp]
- [released: 3.7.9
- [optional]

The receiver interface is required to receive data from the realtime audio process via the
IDataExchangeHandler.

\see \ref IDataExchangeHandler
*/
class IDataExchangeReceiver : public FUnknown
{
public:
	/** queue opened notification
	 *
	 *	called on the main thread when the processor has opened a queue
	 *
	 *	@param userContextID the user context ID of the queue
	 *	@param blockSize the size of one block of the queue
	 *	@param dispatchedOnBackgroundThread if true on output the blocks are dispatched on a
	 *										background thread [defaults to false in which case the
	 *										blocks are dispatched on the main thread]
	 */
	virtual void PLUGIN_API queueOpened (DataExchangeUserContextID userContextID, uint32 blockSize,
	                                     TBool& dispatchOnBackgroundThread) = 0;
	/** queue closed notification
	 *
	 *	called on the main thread when the processor has closed a queue
	 *
	 *	@param userContextID the user context ID of the queue
	 */
	virtual void PLUGIN_API queueClosed (DataExchangeUserContextID userContextID) = 0;

	/** one or more blocks were received
	 *
	 *	called either on the main thread or a background thread depending on the
	 *	dispatchOnBackgroundThread value in the queueOpened call.
	 *
	 *	the data of the blocks are only valid inside this call and the blocks only become available
	 *	to the queue afterwards.
	 *
	 *	@param userContextID the user context ID of the queue
	 *	@param numBlocks number of blocks
	 *	@param blocks the blocks
	 *	@param onBackgroundThread true if the call is done on a background thread
	 */
	virtual void PLUGIN_API onDataExchangeBlocksReceived (DataExchangeUserContextID userContextID,
	                                                      uint32 numBlocks,
	                                                      DataExchangeBlock* blocks,
	                                                      TBool onBackgroundThread) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IDataExchangeReceiver, 0x45A759DC, 0x84FA4907, 0xABCB6175, 0x2FC786B6)

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpop.h"
//------------------------------------------------------------------------
