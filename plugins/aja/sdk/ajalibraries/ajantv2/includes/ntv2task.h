/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2task.h
	@brief		Declares the CNTV2Task class (deprecated).
	@copyright	(C) 2008-2021 AJA Video Systems, Inc.
**/

#ifndef NTV2TASK_H
#define NTV2TASK_H

#include "ajaexport.h"
#include "ntv2card.h"
#if !defined (NTV2_DEPRECATE)
	#include "ntv2signalrouter.h"
#endif	//	!defined (NTV2_DEPRECATE)

#if defined(AJALinux)
	#include <stdio.h>
#endif
#include <sstream>

/**
	@brief	The AutoCirculate "task" facility is deprecated. Use the new AutoCirculate APIs.
**/
class AJAExport NTV2_DEPRECATED_CLASS CNTV2Task
{
public:

	// constructors
	CNTV2Task();
	CNTV2Task (const CNTV2Task & other);
	virtual ~CNTV2Task();

	// initialize the task list
	AJA_VIRTUAL void Init();

	// clear the task list
	AJA_VIRTUAL void Clear();

	// add a frame synchronized register write
	AJA_VIRTUAL AutoCircGenericTask * AddRegisterWriteTask (ULWord registerNum,
															ULWord registerValue,
															ULWord registerMask		= 0xffffffff,
															ULWord registerShift	= 0);

	// add a frame synchronized register read
	AJA_VIRTUAL AutoCircGenericTask * AddRegisterReadTask (ULWord registerNum,
															ULWord registerMask		= 0xffffffff,
															ULWord registerShift	= 0);

	#if !defined (NTV2_DEPRECATE)
		// convert routing entries to register write tasks
		AJA_VIRTUAL NTV2_DEPRECATED_f(AutoCircGenericTask *	AddRoutingEntry (const NTV2RoutingEntry & inEntry));
		AJA_VIRTUAL NTV2_DEPRECATED_f(AutoCircGenericTask *	AddRoutingEntryWithValue (const NTV2RoutingEntry & inEntry, const ULWord inValue));
		AJA_VIRTUAL NTV2_DEPRECATED_f(void					AddSignalRouting (const CNTV2SignalRouter & inRouter));


		AJA_VIRTUAL NTV2_DEPRECATED_f(AutoCircGenericTask *	AddXena2Routing (const NTV2RoutingEntry & inEntry));
		AJA_VIRTUAL NTV2_DEPRECATED_f(AutoCircGenericTask *	AddXena2RoutingWithValue (const NTV2RoutingEntry & inEntry, const ULWord inValue));
		AJA_VIRTUAL NTV2_DEPRECATED_f(void					CopyXena2Routing (const CNTV2SignalRouter * pInRouter));
		// dump the task list to debug output
		AJA_VIRTUAL NTV2_DEPRECATED_f(void					DumpTaskList (void) const);
	#endif	//	!defined (NTV2_DEPRECATE)

	// add time code write
	AJA_VIRTUAL AutoCircGenericTask * AddTimeCodeWriteTask (RP188_STRUCT * pTCInOut1 = NULL,
															RP188_STRUCT* pTCInOut2 = NULL,
															RP188_STRUCT* pLTCEmbedded = NULL,
															RP188_STRUCT* pLTCAnalog = NULL,
															RP188_STRUCT* pLTCEmbedded2 = NULL,
															RP188_STRUCT* pLTCAnalog2 = NULL,
															RP188_STRUCT* pTCInOut3 = NULL,
															RP188_STRUCT* pTCInOut4 = NULL,
															RP188_STRUCT* pTCInOut5 = NULL,
															RP188_STRUCT* pTCInOut6 = NULL,
															RP188_STRUCT* pTCInOut7 = NULL,
															RP188_STRUCT* pTCInOut8 = NULL);

	// add time code read
	AJA_VIRTUAL AutoCircGenericTask* AddTimeCodeReadTask();

	// get a task by index
	AJA_VIRTUAL AutoCircGenericTask* GetTask (ULWord index);
	AJA_VIRTUAL const AutoCircGenericTask & GetTask (const ULWord index) const;

	// get the current number of tasks
	AJA_VIRTUAL ULWord GetNumTasks (void) const;

	// get the maximum number of tasks
	AJA_VIRTUAL ULWord GetMaxTasks (void) const;

	// get a pointer to the task structure for TransferWithAutoCirculate_Ex2
	AJA_VIRTUAL PAUTOCIRCULATE_TASK_STRUCT GetTaskStruct();

	// operators
	AJA_VIRTUAL CNTV2Task & operator = (const CNTV2Task & other);
	AJA_VIRTUAL operator AUTOCIRCULATE_TASK_STRUCT () const			{ return m_AutoCircTask; }
	AJA_VIRTUAL operator PAUTOCIRCULATE_TASK_STRUCT ()				{ return &m_AutoCircTask; }

protected:
	// used internally and by the driver
	static bool InitTaskArray(AutoCircGenericTask* pTaskArray, ULWord numTasks);
	static ULWord CopyTaskArray(AutoCircGenericTask* pDstArray, ULWord dstSize, ULWord dstMax,
		const AutoCircGenericTask* pSrcArray, ULWord srcSize, ULWord srcNum);

	AUTOCIRCULATE_TASK_STRUCT	m_AutoCircTask;
	AutoCircGenericTask			m_TaskArray [AUTOCIRCULATE_TASK_MAX_TASKS];

};	//	CNTV2Task


//	ostream operators
std::ostream & operator << (std::ostream & inOutStream, const AutoCircGenericTask & inObj);
std::ostream & operator << (std::ostream & inOutStream, const CNTV2Task & inObj);
std::ostream & operator << (std::ostream & inOutStream, const AutoCircRegisterTask & inObj);
std::ostream & operator << (std::ostream & inOutStream, const AutoCircTimeCodeTask & inObj);

#endif	//	NTV2TASK_H
