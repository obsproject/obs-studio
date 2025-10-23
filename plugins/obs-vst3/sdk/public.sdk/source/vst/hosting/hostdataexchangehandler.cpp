//-----------------------------------------------------------------------------
// Project     : VST SDK
// Flags       : clang-format SMTGSequencer
// Category    : Helpers
// Filename    : public.sdk/source/vst/hosting/hostdataexchangehandler.cpp
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

#include "hostdataexchangehandler.h"
#include "../utility/alignedalloc.h"
#include "../utility/ringbuffer.h"
#include "pluginterfaces/base/funknownimpl.h"

#include <algorithm>
#include <cassert>
#include <mutex>
#include <vector>

#ifdef _MSC_VER
#include <malloc.h>
#endif

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
struct HostDataExchangeHandler::Impl
: U::ImplementsNonDestroyable<U::Directly<IDataExchangeHandler>>
{
	struct Block
	{
		Block () = default;
		Block (uint32 blockSize, uint32 alignment, DataExchangeBlockID id)
		: blockID (id), alignment (alignment)
		{
			data = aligned_alloc (blockSize, alignment);
		}
		Block (Block&& other) { *this = std::move (other); }
		~Block () noexcept
		{
			if (data)
				aligned_free (data, alignment);
		}

		Block& operator= (Block&& other)
		{
			data = other.data;
			other.data = nullptr;
			blockID = other.blockID;
			other.blockID = InvalidDataExchangeBlockID;
			alignment = other.alignment;
			return *this;
		}

		void* data {nullptr};
		DataExchangeBlockID blockID {InvalidDataExchangeBlockID};
		uint32 alignment {0};
	};
	struct Queue
	{
		using BlockRingBuffer = OneReaderOneWriter::RingBuffer<Block>;
		// we do the assumption that the std::vector does not allocate memory when we don't push
		// more items than we reserve before
		using BlockVector = std::vector<Block>;

		Queue (IAudioProcessor* owner, IDataExchangeReceiver* receiver,
		       DataExchangeUserContextID userContext, uint32 blockSize, uint32 numBlocks,
		       uint32 alignment)
		: owner (owner)
		, receiver (receiver)
		, userContext (userContext)
		, blockSize (blockSize)
		, numBlocks (numBlocks)
		{
			receiver->queueOpened (userContext, blockSize, wantBlocksOnBackgroundThread);
			freeList.resize (numBlocks);
			sendList.resize (numBlocks);
			lockList.reserve (numBlocks);
			freeListOnRTThread.reserve (numBlocks);
			for (auto idx = 0u; idx < numBlocks; ++idx)
				freeList.push (Block (blockSize, alignment, idx));
		}
		~Queue () noexcept
		{
			if (receiver)
				receiver->queueClosed (userContext);
		}

		bool lock (DataExchangeBlock& block)
		{
			if (freeListOnRTThread.empty () == false)
			{
				auto& back = freeListOnRTThread.back ();
				block.data = back.data;
				block.size = blockSize;
				block.blockID = back.blockID;
				lockList.emplace_back (std::move (back));
				freeListOnRTThread.pop_back ();
				return true;
			}
			Block b;
			if (freeList.pop (b))
			{
				block.data = b.data;
				block.size = blockSize;
				block.blockID = b.blockID;
				lockList.emplace_back (std::move (b));
				return true;
			}
			return false;
		}

		bool free (DataExchangeBlockID blockID)
		{
			if (blockID >= numBlocks)
				return false;
			auto it = std::find_if (lockList.begin (), lockList.end (),
			                        [&] (const auto& el) { return el.blockID == blockID; });
			if (it == lockList.end ())
				return false;

			Block b = std::move (*it);
			freeListOnRTThread.emplace_back (std::move (b));
			lockList.erase (it);
			return true;
		}

		bool readyToSend (DataExchangeBlockID blockID)
		{
			if (blockID >= numBlocks)
				return false;
			auto it = std::find_if (lockList.begin (), lockList.end (),
			                        [&] (const auto& el) { return el.blockID == blockID; });
			if (it == lockList.end ())
				return false;

			Block b = std::move (*it);
			sendList.push (std::move (b));
			lockList.erase (it);
			return true;
		}

		uint32 sendBlocks (DataExchangeQueueID queueID)
		{
			BlockVector blocks;
			Block b;
			while (sendList.pop (b))
			{
				blocks.emplace_back (std::move (b));
			}
			if (blocks.empty ())
				return 0;

			std::vector<DataExchangeBlock> debs;
			std::for_each (blocks.begin (), blocks.end (), [&] (const auto& el) {
				DataExchangeBlock block;
				block.data = el.data;
				block.size = blockSize;
				block.blockID = el.blockID;
				debs.push_back (block);
			});

			receiver->onDataExchangeBlocksReceived (userContext, static_cast<uint32> (debs.size ()),
			                                        debs.data (), wantBlocksOnBackgroundThread);

			std::for_each (blocks.begin (), blocks.end (),
			               [&] (auto&& el) { freeList.push (std::move (el)); });
			return static_cast<uint32> (debs.size ());
		}

		IAudioProcessor* owner;
		IPtr<IDataExchangeReceiver> receiver;
		DataExchangeUserContextID userContext {};
		TBool wantBlocksOnBackgroundThread {false};

		BlockRingBuffer freeList;
		BlockVector freeListOnRTThread;
		BlockVector lockList;
		BlockRingBuffer sendList;

		uint32 blockSize {0};
		uint32 numBlocks {0};
	};
	using QueuePtr = std::unique_ptr<Queue>;
	using QueueList = std::vector<QueuePtr>;

	Impl (IDataExchangeHandlerHost& host, uint32 maxQueues) : host (host)
	{
		queues.resize (maxQueues);
	}

	void setQueue (DataExchangeQueueID queueID, QueuePtr&& queue)
	{
		queuesLock.lock ();
		queues[queueID] = std::move (queue);
		if (queues[queueID]->wantBlocksOnBackgroundThread)
			++numOpenBackgroundQueues;
		else
			++numOpenMainThreadQueues;
		queuesLock.unlock ();
		host.onQueueOpened (queues[queueID]->owner, queueID,
		                    queues[queueID]->wantBlocksOnBackgroundThread);
		host.numberOfQueuesChanged (numOpenMainThreadQueues, numOpenBackgroundQueues);
	}

	tresult PLUGIN_API openQueue (IAudioProcessor* owner, uint32 blockSize, uint32 numBlocks,
	                              uint32 alignment, DataExchangeUserContextID userContext,
	                              DataExchangeQueueID* outID) override
	{
		if (!host.isMainThread ())
			return kResultFalse;
		if (outID == nullptr)
			return kInvalidArgument;
		if (!host.isProcessorInactive (owner))
			return kResultFalse;
		auto receiver = host.findDataExchangeReceiver (owner);
		if (!receiver)
			return kInvalidArgument;
		if (!host.allowAllocateSize (blockSize, numBlocks, alignment))
			return kOutOfMemory;

		for (auto queueID = 0; queueID < queues.size (); ++queueID)
		{
			if (queues[queueID] == nullptr)
			{
				auto newQueue = std::make_unique<Queue> (owner, receiver, userContext, blockSize,
				                                         numBlocks, alignment);
				setQueue (queueID, std::move (newQueue));
				*outID = queueID;
				return kResultTrue;
			}
		}
		auto queueSize = queues.size ();
		if (host.allowQueueListResize (static_cast<uint32> (queueSize + 1)))
		{
			queues.resize (queueSize + 1);
			assert (queues.size () == queueSize + 1);
			DataExchangeQueueID queueID = static_cast<DataExchangeQueueID> (queueSize);
			auto newQueue = std::make_unique<Queue> (owner, receiver, userContext, blockSize,
			                                         numBlocks, alignment);
			setQueue (queueID, std::move (newQueue));
			*outID = queueID;
			return kResultTrue;
		}
		return kOutOfMemory;
	}

	tresult PLUGIN_API closeQueue (DataExchangeQueueID queueID) override
	{
		if (!host.isMainThread ())
			return kResultFalse;
		if (queues[queueID])
		{
			if (!host.isProcessorInactive (queues[queueID]->owner))
				return kResultFalse;

			QueuePtr q;
			queuesLock.lock ();
			std::swap (q, queues[queueID]);
			if (q->wantBlocksOnBackgroundThread)
				--numOpenBackgroundQueues;
			else
				--numOpenMainThreadQueues;
			queuesLock.unlock ();
			host.onQueueClosed (q->owner, queueID, q->wantBlocksOnBackgroundThread);
			host.numberOfQueuesChanged (numOpenMainThreadQueues, numOpenBackgroundQueues);
			q.reset ();
			return kResultTrue;
		}
		return kResultFalse;
	}

	tresult PLUGIN_API lockBlock (DataExchangeQueueID queueId, DataExchangeBlock* block) override
	{
		if (!block || queueId >= queues.size () || queues[queueId] == nullptr)
			return kInvalidArgument;
		if (queues[queueId]->lock (*block))
			return kResultTrue;
		return kOutOfMemory;
	}

	tresult PLUGIN_API freeBlock (DataExchangeQueueID queueId, DataExchangeBlockID blockID,
	                              TBool sendToController) override
	{
		if (queueId >= queues.size () || queues[queueId] == nullptr)
			return kInvalidArgument;
		if (sendToController)
		{
			if (queues[queueId]->readyToSend (blockID))
			{
				++numReadyToSendBlocks;
				host.newBlockReadyToBeSend (queueId);
				return kResultTrue;
			}
			return kResultFalse;
		}
		return queues[queueId]->free (blockID) ? kResultTrue : kResultFalse;
	}

	bool sendBlocks (bool isMainThread, size_t queueID, uint32& numSendBlocks)
	{
		LockGuard guard (queuesLock);
		if (auto& queue = queues[queueID])
		{
			if (queue->wantBlocksOnBackgroundThread != static_cast<TBool> (isMainThread))
			{
				numSendBlocks = queue->sendBlocks (static_cast<DataExchangeQueueID> (queueID));
			}
			return true;
		}
		return false;
	}

	uint32 sendBlocks (bool isMainThread, DataExchangeQueueID queueFilter)
	{
		if (queueFilter != InvalidDataExchangeQueueID)
		{
			if (queueFilter < queues.size ())
			{
				uint32 numSendBlocks;
				if (sendBlocks (isMainThread, queueFilter, numSendBlocks))
					return numSendBlocks;
			}
			return 0;
		}
		uint32 totalSendBlocks = 0;
		uint32 openQueues = numOpenBackgroundQueues + numOpenMainThreadQueues;
		for (auto queueID = 0u; queueID < queues.size (); ++queueID)
		{
			uint32 numSendBlocks;
			if (sendBlocks (isMainThread, queueID, numSendBlocks))
			{
				numReadyToSendBlocks -= numSendBlocks;
				totalSendBlocks += numSendBlocks;
				if ((--openQueues) == 0)
					break;
			}
		}
		return totalSendBlocks;
	}

	IDataExchangeHandlerHost& host;
	QueueList queues;
	std::atomic<uint32> numReadyToSendBlocks {0};
	std::atomic<uint32> numOpenMainThreadQueues {0};
	std::atomic<uint32> numOpenBackgroundQueues {0};
	using Mutex = std::recursive_mutex;
	using LockGuard = std::lock_guard<Mutex>;
	Mutex queuesLock;
};

//------------------------------------------------------------------------
HostDataExchangeHandler::HostDataExchangeHandler (IDataExchangeHandlerHost& host, uint32 maxQueues)
{
	impl = std::make_unique<Impl> (host, maxQueues);
}

//------------------------------------------------------------------------
HostDataExchangeHandler::~HostDataExchangeHandler () noexcept = default;

//------------------------------------------------------------------------
IDataExchangeHandler* HostDataExchangeHandler::getInterface () const
{
	return impl.get ();
}

//------------------------------------------------------------------------
uint32 HostDataExchangeHandler::sendMainThreadBlocks ()
{
	return impl->sendBlocks (true, InvalidDataExchangeQueueID);
}

//------------------------------------------------------------------------
uint32 HostDataExchangeHandler::sendBackgroundBlocks (DataExchangeQueueID queueId)
{
	return impl->sendBlocks (false, queueId);
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
