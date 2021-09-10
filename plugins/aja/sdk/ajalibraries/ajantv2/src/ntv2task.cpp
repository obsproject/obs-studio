/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2task.cpp
	@brief		Implements the CNTV2Task class.
	@deprecate	Its functionality is deprecated.
	@copyright	(C) 2008-2021 AJA Video Systems, Inc.
**/

#include "ntv2task.h"
#include "ntv2debug.h"
#include "ntv2utils.h"

#include <string.h>
#include <iomanip>

#ifndef NULL
#define NULL (0)
#endif

using namespace std;


CNTV2Task::CNTV2Task()
{
	Init();
}

CNTV2Task::CNTV2Task(const CNTV2Task& other)
{
	*this = other;
}

CNTV2Task::~CNTV2Task()
{
}

void CNTV2Task::Init()
{
	m_AutoCircTask.taskVersion = AUTOCIRCULATE_TASK_VERSION;
	m_AutoCircTask.taskSize = sizeof(AutoCircGenericTask);
	m_AutoCircTask.maxTasks = AUTOCIRCULATE_TASK_MAX_TASKS;
	m_AutoCircTask.numTasks = 0;
	m_AutoCircTask.taskArray = m_TaskArray;
	InitTaskArray(m_TaskArray, AUTOCIRCULATE_TASK_MAX_TASKS);
}

void CNTV2Task::Clear()
{
	m_AutoCircTask.numTasks = 0;
}

AutoCircGenericTask* CNTV2Task::AddRegisterWriteTask(
	ULWord registerNum,
	ULWord registerValue,
	ULWord registerMask,
	ULWord registerShift)
{
	ULWord index = m_AutoCircTask.numTasks;

	if(index >= m_AutoCircTask.maxTasks)
	{
		return NULL;
	}

	AutoCircGenericTask* pGeneric = &m_TaskArray[index];
	pGeneric->taskType = eAutoCircTaskRegisterWrite;

	pGeneric->u.registerTask.regNum = registerNum;
	pGeneric->u.registerTask.mask = registerMask;
	pGeneric->u.registerTask.shift = registerShift;
	pGeneric->u.registerTask.value = registerValue;

	index++;
	m_AutoCircTask.numTasks = index;

	return pGeneric;
}

AutoCircGenericTask* CNTV2Task::AddRegisterReadTask(
	ULWord registerNum,
	ULWord registerMask,
	ULWord registerShift)
{
	ULWord index = m_AutoCircTask.numTasks;

	if(index >= m_AutoCircTask.maxTasks)
	{
		return NULL;
	}

	AutoCircGenericTask* pGeneric = &m_TaskArray[index];
	pGeneric->taskType = eAutoCircTaskRegisterRead;

	pGeneric->u.registerTask.regNum = registerNum;
	pGeneric->u.registerTask.mask = registerMask;
	pGeneric->u.registerTask.shift = registerShift;
	pGeneric->u.registerTask.value = 0;

	index++;
	m_AutoCircTask.numTasks = index;

	return pGeneric;
}


#if !defined (NTV2_DEPRECATE)
	AutoCircGenericTask* CNTV2Task::AddRoutingEntry (const NTV2RoutingEntry & inEntry)
	{
		return AddRegisterWriteTask (inEntry.registerNum, inEntry.value, inEntry.mask, inEntry.shift);
	}


	AutoCircGenericTask* CNTV2Task::AddRoutingEntryWithValue (const NTV2RoutingEntry & inEntry, const ULWord inValue)
	{
		return AddRegisterWriteTask (inEntry.registerNum, inValue, inEntry.mask, inEntry.shift);
	}


	void CNTV2Task::AddSignalRouting (const CNTV2SignalRouter & inRouter)
	{
		const unsigned	numRoutes	(inRouter.GetNumberOfConnections ());
		for (unsigned ndx (0);  ndx < numRoutes;  ndx++)
			AddRoutingEntry (inRouter.GetRoutingEntry (ndx));
	}	//	AddSignalRouting


	AutoCircGenericTask* CNTV2Task::AddXena2Routing (const NTV2RoutingEntry & inEntry)
	{
		return AddRoutingEntry (inEntry);
	}

	AutoCircGenericTask* CNTV2Task::AddXena2RoutingWithValue (const NTV2RoutingEntry & inEntry, const ULWord inValue)
	{
		return AddRoutingEntryWithValue (inEntry, inValue);
	}

	void CNTV2Task::CopyXena2Routing (const CNTV2SignalRouter * pInRoute)
	{
		if (pInRoute)
			AddSignalRouting (*pInRoute);
	}
#endif	//	!defined (NTV2_DEPRECATE)


AutoCircGenericTask* CNTV2Task::AddTimeCodeWriteTask(
	RP188_STRUCT* pTCInOut1,
	RP188_STRUCT* pTCInOut2,
	RP188_STRUCT* pLTCEmbedded,
	RP188_STRUCT* pLTCAnalog,
	RP188_STRUCT* pLTCEmbedded2,
	RP188_STRUCT* pLTCAnalog2,
	RP188_STRUCT* pTCInOut3,
	RP188_STRUCT* pTCInOut4,
	RP188_STRUCT* pTCInOut5,
	RP188_STRUCT* pTCInOut6,
	RP188_STRUCT* pTCInOut7,
	RP188_STRUCT* pTCInOut8)
{
	ULWord index = m_AutoCircTask.numTasks;

	if(index >= m_AutoCircTask.maxTasks)
	{
		return NULL;
	}

	AutoCircGenericTask* pGeneric = &m_TaskArray[index];
	memset((void*)pGeneric, 0, sizeof(AutoCircGenericTask));

	pGeneric->taskType = eAutoCircTaskTimeCodeWrite;

	if(pTCInOut1 != NULL)
	{
		pGeneric->u.timeCodeTask.TCInOut1 = *pTCInOut1;
	}
	else
	{
		pGeneric->u.timeCodeTask.TCInOut1.Low = 0xffffffff;
	}
	if(pTCInOut2 != NULL)
	{
		pGeneric->u.timeCodeTask.TCInOut2 = *pTCInOut2;
	}
	else
	{
		pGeneric->u.timeCodeTask.TCInOut2.Low = 0xffffffff;
	}
	if(pTCInOut3 != NULL)
	{
		pGeneric->u.timeCodeTask.TCInOut3 = *pTCInOut3;
	}
	else
	{
		pGeneric->u.timeCodeTask.TCInOut3.Low = 0xffffffff;
	}
	if(pTCInOut4 != NULL)
	{
		pGeneric->u.timeCodeTask.TCInOut4 = *pTCInOut4;
	}
	else
	{
		pGeneric->u.timeCodeTask.TCInOut4.Low = 0xffffffff;
	}
	if(pTCInOut5 != NULL)
	{
		pGeneric->u.timeCodeTask.TCInOut5 = *pTCInOut5;
	}
	else
	{
		pGeneric->u.timeCodeTask.TCInOut5.Low = 0xffffffff;
	}
	if(pTCInOut6 != NULL)
	{
		pGeneric->u.timeCodeTask.TCInOut6 = *pTCInOut6;
	}
	else
	{
		pGeneric->u.timeCodeTask.TCInOut6.Low = 0xffffffff;
	}
	if(pTCInOut7 != NULL)
	{
		pGeneric->u.timeCodeTask.TCInOut7 = *pTCInOut7;
	}
	else
	{
		pGeneric->u.timeCodeTask.TCInOut7.Low = 0xffffffff;
	}
	if(pTCInOut8 != NULL)
	{
		pGeneric->u.timeCodeTask.TCInOut8 = *pTCInOut8;
	}
	else
	{
		pGeneric->u.timeCodeTask.TCInOut8.Low = 0xffffffff;
	}
	if(pLTCEmbedded != NULL)
	{
		pGeneric->u.timeCodeTask.LTCEmbedded = *pLTCEmbedded;
	}
	else
	{
		pGeneric->u.timeCodeTask.LTCEmbedded.Low = 0xffffffff;
	}
	if(pLTCAnalog != NULL)
	{
		pGeneric->u.timeCodeTask.LTCAnalog = *pLTCAnalog;
	}
	else
	{
		pGeneric->u.timeCodeTask.LTCAnalog.Low = 0xffffffff;
	}
	if(pLTCEmbedded2 != NULL)
	{
		pGeneric->u.timeCodeTask.LTCEmbedded2 = *pLTCEmbedded2;
	}
	else
	{
		pGeneric->u.timeCodeTask.LTCEmbedded2.Low = 0xffffffff;
	}
	if(pLTCAnalog2 != NULL)
	{
		pGeneric->u.timeCodeTask.LTCAnalog2 = *pLTCAnalog2;
	}
	else
	{
		pGeneric->u.timeCodeTask.LTCAnalog2.Low = 0xffffffff;
	}

	index++;
	m_AutoCircTask.numTasks = index;

	return pGeneric;
}

AutoCircGenericTask* CNTV2Task::AddTimeCodeReadTask()
{
	ULWord index = m_AutoCircTask.numTasks;

	if(index >= m_AutoCircTask.maxTasks)
	{
		return NULL;
	}

	AutoCircGenericTask* pGeneric = &m_TaskArray[index];
	memset((void*)pGeneric, 0, sizeof(AutoCircGenericTask));

	pGeneric->taskType = eAutoCircTaskTimeCodeRead;

	index++;
	m_AutoCircTask.numTasks = index;

	return pGeneric;
}

AutoCircGenericTask* CNTV2Task::GetTask(ULWord index)
{
	if(index >= m_AutoCircTask.numTasks)
	{
		return NULL;
	}

	return &m_TaskArray[index];
}


const AutoCircGenericTask & CNTV2Task::GetTask (const ULWord inIndex) const
{
	if (inIndex >= m_AutoCircTask.numTasks)
	{
		static AutoCircGenericTask	nullTask;
		::memset ((void*)&nullTask, 0, sizeof (nullTask));
		return nullTask;
	}
	else
		return m_TaskArray [inIndex];
}


ULWord CNTV2Task::GetNumTasks (void) const
{
	return m_AutoCircTask.numTasks;
}

ULWord CNTV2Task::GetMaxTasks (void) const
{
	return m_AutoCircTask.maxTasks;
}

PAUTOCIRCULATE_TASK_STRUCT CNTV2Task::GetTaskStruct()
{
	return &m_AutoCircTask;
}

#if !defined (NTV2_DEPRECATE)
	void CNTV2Task::DumpTaskList (void) const
	{
		const int	numTasks	(GetNumTasks ());

		odprintf("NTV2Task(%p): dump %d tasks\n", (void*)this, numTasks);
		for (int i = 0; i < numTasks; i++)
		{
			const AutoCircGenericTask &	task	(GetTask (i));
			switch (task.taskType)
			{
				case eAutoCircTaskRegisterWrite:
					odprintf("NTV2Task: task %3d register write - reg %3d  value %08x  mask %08x  shift %3d\n",
						i,
						task.u.registerTask.regNum,
						task.u.registerTask.value,
						task.u.registerTask.mask,
						task.u.registerTask.shift);
					break;
				case eAutoCircTaskRegisterRead:
					odprintf("NTV2Task: task %3d register read  - reg %3d  value %08x  mask %08x  shift %3d\n",
						i,
						task.u.registerTask.regNum,
						task.u.registerTask.value,
						task.u.registerTask.mask,
						task.u.registerTask.shift);
					break;
				case eAutoCircTaskTimeCodeWrite:
					odprintf("NTV2Task: task %3d timecode write\n", i);
					break;	
				case eAutoCircTaskTimeCodeRead:
					odprintf("NTV2Task: task %3d timecode read\n", i);
					break;
				default:
					odprintf("NTV2Task: task %3d error - unknown task type: %d\n", i, task.taskType);
					break;
			}
		}
	}
#endif	//	!defined (NTV2_DEPRECATE)


static const string AutoCircTaskTypeToString (const AutoCircTaskType inTaskType)
{
	switch (inTaskType)
	{
		case eAutoCircTaskNone:				return "<none>";
		case eAutoCircTaskRegisterWrite:	return "RegWrite";
		case eAutoCircTaskRegisterRead:		return "RegRead";
		case eAutoCircTaskTimeCodeWrite:	return "TCWrite";
		case eAutoCircTaskTimeCodeRead:		return "TCRead";
		case MAX_NUM_AutoCircTaskTypes:		break;
	}
	return "";
}


ostream & operator << (ostream & inOutStream, const AutoCircRegisterTask & inObj)
{
	return inOutStream	<< "reg=" << inObj.regNum << ", mask=0x" << hex << setw (8) << setfill ('0') << inObj.mask << dec
						<< ", shift=" << inObj.shift << ", value=0x" << hex << setw (8) << setfill ('0') << inObj.value << dec;
}


ostream & operator << (ostream & inOutStream, const AutoCircTimeCodeTask & inObj)
{
	return inOutStream	<< "    TCInOut1: " << inObj.TCInOut1 << endl
						<< "    TCInOut2: " << inObj.TCInOut2 << endl
						<< "    TCInOut3: " << inObj.TCInOut3 << endl
						<< "    TCInOut4: " << inObj.TCInOut4 << endl
						<< "    TCInOut5: " << inObj.TCInOut5 << endl
						<< "    TCInOut6: " << inObj.TCInOut6 << endl
						<< "    TCInOut7: " << inObj.TCInOut7 << endl
						<< "    TCInOut8: " << inObj.TCInOut8 << endl
						<< " LTCEmbedded: " << inObj.LTCEmbedded << endl
						<< "LTCEmbedded2: " << inObj.LTCEmbedded2 << endl
						<< "LTCEmbedded3: " << inObj.LTCEmbedded3 << endl
						<< "LTCEmbedded4: " << inObj.LTCEmbedded4 << endl
						<< "LTCEmbedded5: " << inObj.LTCEmbedded5 << endl
						<< "LTCEmbedded6: " << inObj.LTCEmbedded6 << endl
						<< "LTCEmbedded7: " << inObj.LTCEmbedded7 << endl
						<< "LTCEmbedded8: " << inObj.LTCEmbedded8 << endl
						<< "   LTCAnalog: " << inObj.LTCAnalog << endl
						<< "  LTCAnalog2: " << inObj.LTCAnalog2;
}


ostream & operator << (ostream & inOutStream, const AutoCircGenericTask & inObj)
{
	inOutStream	<< ::AutoCircTaskTypeToString (inObj.taskType);
	if (NTV2_IS_REGISTER_TASK (inObj.taskType))
		inOutStream	<< ": " << inObj.u.registerTask;
	else if (NTV2_IS_TIMECODE_TASK (inObj.taskType))
		inOutStream	<< ":" << endl << inObj.u.timeCodeTask;
	return inOutStream;
}


ostream & operator << (ostream & inOutStream, const CNTV2Task & inObj)
{
	const ULWord	nTasks	(inObj.GetNumTasks ());
	inOutStream	<< nTasks << " task(s):"	<< endl;
	for (ULWord	num (0);  num < nTasks;  num++)
	{
		const AutoCircGenericTask &		task (inObj.GetTask (num));
		inOutStream << "[" << num << "] " << task << endl;
	}
	return inOutStream;
}


bool CNTV2Task::InitTaskArray(AutoCircGenericTask* pTaskArray, ULWord numTasks)
{
	if(pTaskArray == NULL)
	{
		return false;
	}

	memset((void*)pTaskArray, 0, numTasks * sizeof(AutoCircGenericTask));

	return true;
}

ULWord CNTV2Task::CopyTaskArray(AutoCircGenericTask* pDstArray, ULWord dstSize, ULWord dstMax,
							  const AutoCircGenericTask* pSrcArray, ULWord srcSize, ULWord srcNum)
{
	ULWord i;

	// copy src to dst with support for changes in sizeof(AutoCircGenericTask)

	if((pSrcArray == NULL) || (pDstArray == NULL))
	{
		return false;
	}

	ULWord transferSize = srcSize;
	if(transferSize > dstSize)
	{
		transferSize = dstSize;
	}

	ULWord transferNum = srcNum;
	if(transferNum > dstMax)
	{
		transferNum = dstMax;
	}

	UByte* pSrc = (UByte*)pSrcArray;
	UByte* pDst = (UByte*)pDstArray;

	for(i = 0; i < transferNum; i++)
	{
		memcpy(pDst, pSrc, transferSize);
		pSrc += srcSize;
		pDst += dstSize;
	}

	return transferNum;
}

CNTV2Task& CNTV2Task::operator= (const CNTV2Task& other)
{
	InitTaskArray(m_TaskArray, AUTOCIRCULATE_TASK_MAX_TASKS);

	m_AutoCircTask.numTasks = CopyTaskArray(m_TaskArray, m_AutoCircTask.taskSize, m_AutoCircTask.maxTasks,
											other.m_TaskArray, other.m_AutoCircTask.taskSize, other.m_AutoCircTask.numTasks);

	return *this;
}
