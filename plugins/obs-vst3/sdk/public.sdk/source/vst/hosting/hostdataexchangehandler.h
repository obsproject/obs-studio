//-----------------------------------------------------------------------------
// Project     : VST SDK
// Flags       : clang-format SMTGSequencer
// Category    : Helpers
// Filename    : public.sdk/source/vst/hosting/hostdataexchangehandler.h
// Created by  : Steinberg, 06/2023
// Description : VST Data Exchange API Host Helper
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/ivstdataexchange.h"

#include <memory>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
struct IDataExchangeHandlerHost
{
	virtual ~IDataExchangeHandlerHost () noexcept = default;

	/** return if the audioprocessor is in an inactive state
	 *	[main thread]
	 */
	virtual bool isProcessorInactive (IAudioProcessor* processor) = 0;
	/** return the data exchange receiver (most likely the edit controller) for the processor
	 *	[main thread]
	 */
	virtual IPtr<IDataExchangeReceiver> findDataExchangeReceiver (IAudioProcessor* processor) = 0;
	/** check if the requested queue size should be allowed
	 *	[main thread]
	 */
	virtual bool allowAllocateSize (uint32 blockSize, uint32 numBlocks, uint32 alignment) = 0;
	/** check if this call is made on the main thread
	 *	[any thread]
	 */
	virtual bool isMainThread () = 0;

	/** check if the number of queues can be changed in this moment.
	 *
	 *	this is only allowed if no other thread can access the IDataExchangeManagerHost in this
	 *	moment
	 *	[main thread]
	 */
	virtual bool allowQueueListResize (uint32 newNumQueues) = 0;

	/** notification that the number of open queues changed
	 *	[main thread]
	 */
	virtual void numberOfQueuesChanged (uint32 openMainThreadQueues,
	                                    uint32 openBackgroundThreadQueues) = 0;

	/** notification that a new queue was opened */
	virtual void onQueueOpened (IAudioProcessor* processor, DataExchangeQueueID queueID,
	                            bool dispatchOnMainThread) = 0;
	/** notification that a queue was closed */
	virtual void onQueueClosed (IAudioProcessor* processor, DataExchangeQueueID queueID,
	                            bool dispatchOnMainThread) = 0;

	/** notification that a new block is ready to be send
	 *	[process thread]
	 */
	virtual void newBlockReadyToBeSend (DataExchangeQueueID queueID) = 0;
};

//------------------------------------------------------------------------
struct HostDataExchangeHandler
{
	/** Constructor
	 *
	 *	allocate and deallocate this object on the main thread
	 *
	 *	the number of queues is constant
	 *
	 *	@param host the managing host
	 *	@param maxQueues number of maximal allowed open queues
	 */
	HostDataExchangeHandler (IDataExchangeHandlerHost& host, uint32 maxQueues = 64);
	~HostDataExchangeHandler () noexcept;

	/** get the IHostDataExchangeManager interface
	 *
	 *	the interface you must provide to the IAudioProcessor
	 */
	IDataExchangeHandler* getInterface () const;

	/** send blocks
	 *
	 *	the host should periodically call this method on the main thread to send all queued blocks
	 *	which should be send on the main thread
	 */
	uint32 sendMainThreadBlocks ();
	/** send blocks
	 *
	 *	the host should call this on a dedicated background thread
	 *	inside a mutex is used, so don't delete this object while calling this
	 *
	 *	@param queueId	only send blocks from the specified queue. If queueId is equal to
	 *					InvalidDataExchangeQueueID all blocks from all queues are send.
	 */
	uint32 sendBackgroundBlocks (DataExchangeQueueID queueId = InvalidDataExchangeQueueID);

private:
	struct Impl;
	std::unique_ptr<Impl> impl;
};

//------------------------------------------------------------------------
} // Vst
} // Steinberg
