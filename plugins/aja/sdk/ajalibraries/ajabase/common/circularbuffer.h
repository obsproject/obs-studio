/* SPDX-License-Identifier: MIT */
/**
	@file		circularbuffer.h
	@brief		Declaration of AJACircularBuffer template class.
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_CIRCULAR_BUFFER_H
#define AJA_CIRCULAR_BUFFER_H

#include "ajabase/common/public.h"
#include "ajabase/system/lock.h"
#include "ajabase/system/event.h"



/**
	@brief	I am a circular frame buffer that simplifies implementing a type-safe producer/consumer
			model for processing frame-based streaming media. I can be used with any client-defined
			"frame", be it a struct or class. To use me:
				-#	Instantiate me.
				-#	Initialize me by calling my Add method, adding client-defined frames for me to manage.
				-#	Spawn a producer thread and a consumer thread.
				-#	The producer thread repeatedly calls my StartProduceNextBuffer, puts data in the frame,
					then calls EndProduceNextBuffer when finished.
				-#	The consumer thread repeatedly calls my StartConsumeNextBuffer, processes data in the frame,
					then calls EndConsumeNextBuffer when finished.
**/
template <typename FrameDataPtr>
class AJACircularBuffer
{
public:
	/**
		@brief	My default constructor.
	**/
	AJACircularBuffer ();


	/**
		@brief	My destructor.
	**/
	virtual ~AJACircularBuffer ();
	

	/**
		@brief	Tells me the boolean variable I should monitor such that when it gets set to "true" will cause
				any threads waiting on my events/locks to gracefully exit.
		@param[in]	pAbortFlag	Specifies the valid, non-NULL address of a boolean variable that, when it becomes "true",
								will cause threads waiting on me to exit gracefully.
	**/
	inline void SetAbortFlag (const bool * pAbortFlag)
	{
		mAbortFlag = pAbortFlag;
	}


	/**
		@brief	Retrieves the size count of the circular buffer, i.e. how far the tail pointer is behind the head pointer.
		@return	The number of frames that I contain.
	*/
	inline unsigned int GetCircBufferCount (void) const
	{
		return mCircBufferCount;
	}


	/**
		@brief	Returns "true" if I'm empty -- i.e., if my tail and head are in the same place.
		@return	True if I contain no frames.
	**/
	inline bool IsEmpty (void) const
	{
		return GetCircBufferCount () == 0;
	}

	/**
		@brief	Returns my frame storage capacity, which reflects how many times my Add method has been called.
		@return	My frame capacity.
	**/
	inline unsigned int GetNumFrames (void) const
	{
		return (unsigned int) mFrames.size ();
	}

	
	/** 
		@brief	Appends a new frame buffer to me, increasing my frame storage capacity by one frame.
		@param[in]	pInFrameData	Specifies the FrameDataPtr to be added to me.
		@return		AJA_STATUS_SUCCESS	Frame successfully added.
					AJA_STATUS_FAIL		Frame failed to add.
	**/
	AJAStatus Add (FrameDataPtr pInFrameData)
	{
		mFrames.push_back(pInFrameData);
		AJALock* lock = new AJALock;
		mLocks.push_back(lock);
		
		return (mFrames.size() == mLocks.size() && lock) ? AJA_STATUS_SUCCESS
			: AJA_STATUS_FAIL;
	}

	
	/**
		@brief	The thread that's responsible for providing frames -- the producer -- calls this function
				to populate the the returned FrameDataPtr.
		@return	A pointer (of the type in the template argument) to the next frame to be filled by the
				producer thread.
	**/
	FrameDataPtr StartProduceNextBuffer (void)
	{
		while (1)
		{
			if( !WaitForLockOrAbort(&mDataBufferLock) )
				return NULL;
			
			if ( mCircBufferCount == mFrames.size() )
			{
				mDataBufferLock.Unlock();
				if( !WaitForEventOrAbort(&mNotFullEvent) )
					return NULL;
				
				continue;
			}
			break;
		}
		if( !WaitForLockOrAbort(mLocks[mHead]) ) return NULL;
		mFillIndex = mHead;
		mHead = (mHead+1)%((unsigned int)(mFrames.size()));
		mCircBufferCount++;
		if( mCircBufferCount == mFrames.size() )
			mNotFullEvent.SetState(false);
		
		mDataBufferLock.Unlock();
		
		return mFrames[mFillIndex];
	}


	/** 
		@brief	The producer thread calls this function to signal that it has finished populating the frame
				it obtained from a prior call to StartProduceNextBuffer. This releases the frame, making it
				available for processing by the consumer thread.
	**/
	void EndProduceNextBuffer (void);


	/** 
		@brief	The thread that's responsible for processing incoming frames -- the consumer -- calls this
				function to obtain the next available frame.
		@return	A pointer (of the type in the template argument) to the next frame to be processed by the
				consumer thread.
	**/
	FrameDataPtr StartConsumeNextBuffer (void)
	{
		while (1)
		{
			if( !WaitForLockOrAbort(&mDataBufferLock) )
				return NULL;
			
			if ( mCircBufferCount == 0 )
			{
				mDataBufferLock.Unlock();
				if( !WaitForEventOrAbort(&mNotEmptyEvent) )
					return NULL;

				continue;
			}
			break;
		}
		
		if( !WaitForLockOrAbort(mLocks[mTail]) )
			return NULL;
		
		mEmptyIndex = mTail;
		mTail = (mTail+1)%((unsigned int)mFrames.size());
		mCircBufferCount--;
		if( mCircBufferCount == 0 )
			mNotEmptyEvent.SetState(false);
		mDataBufferLock.Unlock();
		
		return mFrames[mEmptyIndex];
	}


	/**
		@brief	The consumer thread calls this function to signal that it has finished processing the frame it
				obtained from a prior call to StartConsumeNextBuffer. This releases the frame, making it available
				for filling by the producer thread.
	**/
	void EndConsumeNextBuffer (void);


	/**
		@brief	Clears my frame collection, their locks, everything.
		@note	This is not thread-safe. Thus, before calling this method, be sure all locks have been released and
				producer/consumer threads using me have terminated.
	**/
	void Clear (void);


private:
	typedef std::vector <AJALock *>	AJALockVector;

	std::vector <FrameDataPtr>	mFrames;			///< @brief	My ordered frame collection
	AJALockVector				mLocks;				///< @brief	My per-frame locks, to control access to each frame
	
	unsigned int				mHead;				///< @brief	My current "head" pointer, an index into my frame collection
	unsigned int				mTail;				///< @brief	My current "tail" pointer, an index into my frame collection
	unsigned int				mCircBufferCount;	///< @brief	My current size, the distance between my "head" and "tail"
	AJAEvent					mNotFullEvent;		///< @brief	An event that signals when I transition from being full to not full
	AJAEvent					mNotEmptyEvent;		///< @brief	An event that signals when I transition from empty to having at least one frame
	AJALock						mDataBufferLock;	///< @brief	Protects my "head" and "tail" members

	unsigned int				mFillIndex;			///< @brief	Index where frames are added to me
	unsigned int				mEmptyIndex;		///< @brief	Index where frames are removed from me

	const bool *				mAbortFlag;			///< @brief	Optional pointer to a boolean that clients can set to break threads waiting on me
	
	/**
		@brief		Waits for the given event with a timeout, and abort the wait if mAbortFlag is set.
		@param[in]	ajaEvent	Specifies a valid, non-NULL AJAEvent object to trigger on.
    	@return		True if the event triggers successfully; false otherwise.
	**/
	bool WaitForEventOrAbort (AJAEvent * ajaEvent);


	/**
		@brief		Wait for mutex with a timeout and check to see if mAbortFlag is set.
		@param[in]	ajaEvent	Specifies a valid, non-NULL AJALock (mutex) object to wait on.
		@return		True if mutex is received; otherwise false.
	**/
	bool WaitForLockOrAbort (AJALock * ajaEvent);

};	//	AJACircularBuffer



template <typename FrameDataPtr>
AJACircularBuffer <FrameDataPtr>::AJACircularBuffer ()
	:	mHead (0),
		mTail (0),
		mCircBufferCount (0),
		mFillIndex (0),
		mEmptyIndex (0),
		mAbortFlag (NULL)
{
}

template <typename FrameDataPtr>
AJACircularBuffer <FrameDataPtr>::~AJACircularBuffer ()
{
	Clear ();
}




template<typename FrameDataPtr>
void AJACircularBuffer<FrameDataPtr>::EndProduceNextBuffer()
{
	mLocks[mFillIndex]->Unlock();
	mNotEmptyEvent.SetState(true);
}

template<typename FrameDataPtr>
void AJACircularBuffer<FrameDataPtr>::EndConsumeNextBuffer()
{
	mLocks[mEmptyIndex]->Unlock();
	mNotFullEvent.SetState(true);
}

template<typename FrameDataPtr>
bool AJACircularBuffer<FrameDataPtr>::WaitForEventOrAbort(AJAEvent* ajaEvent)
{
	const unsigned int timeout = 100;
	
	do {
		AJAStatus status = ajaEvent->WaitForSignal(timeout);
		if (status == AJA_STATUS_TIMEOUT)
			if ( mAbortFlag )
				if ( *mAbortFlag )
					return false;
		if (status == AJA_STATUS_FAIL)
			return false;
		if (status == AJA_STATUS_SUCCESS )
			break;
	} while(1);
	
	return true;
}

template<typename FrameDataPtr>
bool AJACircularBuffer<FrameDataPtr>::WaitForLockOrAbort(AJALock* ajaLock)
{
	const unsigned int timeout = 100;
	
	do {
		AJAStatus status = ajaLock->Lock(timeout);
		if (status == AJA_STATUS_TIMEOUT)
			if ( mAbortFlag )
				if ( *mAbortFlag)
					return false;
		if (status == AJA_STATUS_FAIL)
			return false;
		if (status == AJA_STATUS_SUCCESS )
			break;
	} while(1);
	
	return true;
}


template<typename FrameDataPtr>
void AJACircularBuffer<FrameDataPtr>::Clear (void)
{
	for (AJALockVector::iterator iter (mLocks.begin());  iter != mLocks.end();  ++iter)
		delete *iter;

	mLocks.clear();
	mFrames.clear();

	mHead = mTail =	mFillIndex = mEmptyIndex = mCircBufferCount = 0;
	mAbortFlag = NULL;
}


#endif	//	AJA_CIRCULAR_BUFFER_H
