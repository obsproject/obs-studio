/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2signalrouter.cpp
    @brief		CNTV2SignalRouter implementation.
    @copyright	(C) 2014-2021 AJA Video Systems, Inc.
**/
#include "ntv2signalrouter.h"
#include "ntv2routingexpert.h"
#include "ntv2debug.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#include "ntv2registerexpert.h"

#include <memory.h>
#include <stdio.h>
#include <assert.h>
#include <algorithm>

using namespace std;

// Logging helpers
#define	HEX16(__x__)		"0x" << hex << setw(16) << setfill('0') << uint64_t(__x__)  << dec
#define INSTP(_p_)			HEX16(uint64_t(_p_))
#define	SRiFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_RoutingGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	SRiWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_RoutingGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	SRiNOTE(__x__)		AJA_sNOTICE	(AJA_DebugUnit_RoutingGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	SRiINFO(__x__)		AJA_sINFO	(AJA_DebugUnit_RoutingGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	SRiDBG(__x__)		AJA_sDEBUG	(AJA_DebugUnit_RoutingGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	SRFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_RoutingGeneric, AJAFUNC << ": " << __x__)
#define	SRWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_RoutingGeneric, AJAFUNC << ": " << __x__)
#define	SRNOTE(__x__)		AJA_sNOTICE	(AJA_DebugUnit_RoutingGeneric, AJAFUNC << ": " << __x__)
#define	SRINFO(__x__)		AJA_sINFO	(AJA_DebugUnit_RoutingGeneric, AJAFUNC << ": " << __x__)
#define	SRDBG(__x__)		AJA_sDEBUG	(AJA_DebugUnit_RoutingGeneric, AJAFUNC << ": " << __x__)

static NTV2StringList & Tokenize (const string & inString, NTV2StringList & outTokens, const string & inDelimiters = " ", bool inTrimEmpty = false)
{
    string::size_type	pos (0),	lastPos (0);
    outTokens.clear ();
    while (true)
    {
        pos = inString.find_first_of (inDelimiters, lastPos);
        if (pos == string::npos)
        {
            pos = inString.length ();
            if (pos != lastPos || !inTrimEmpty)
                outTokens.push_back (NTV2StringList::value_type (inString.data () + lastPos, NTV2StringList::size_type(pos - lastPos)));
            break;
        }
        else
        {
            if (pos != lastPos || !inTrimEmpty)
                outTokens.push_back (NTV2StringList::value_type (inString.data () + lastPos, NTV2StringList::size_type(pos - lastPos)));
        }
        lastPos = pos + 1;
    }
    return outTokens;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// CNTV2SignalRouter	Begin

bool CNTV2SignalRouter::AddConnection (const NTV2InputXptID inSignalInput, const NTV2OutputXptID inSignalOutput)
{
    mConnections.insert (NTV2SignalConnection (inSignalInput, inSignalOutput));
	SRiDBG(NTV2InputCrosspointIDToString(inSignalInput) << ", " << NTV2OutputCrosspointIDToString(inSignalOutput) << ": " << *this);
    return true;
}


bool CNTV2SignalRouter::HasInput (const NTV2InputXptID inSignalInput) const
{
    return mConnections.find (inSignalInput) != mConnections.end ();
}


NTV2OutputXptID CNTV2SignalRouter::GetConnectedOutput (const NTV2InputXptID inSignalInput) const
{
	NTV2XptConnectionsConstIter it(mConnections.find(inSignalInput));
	return it != mConnections.end()  ?  it->second  :  NTV2_XptBlack;
}


bool CNTV2SignalRouter::HasConnection (const NTV2InputXptID inSignalInput, const NTV2OutputXptID inSignalOutput) const
{
    NTV2XptConnectionsConstIter	iter (mConnections.find (inSignalInput));
    if (iter == mConnections.end())
        return false;
    return iter->second == inSignalOutput;
}


bool CNTV2SignalRouter::RemoveConnection (const NTV2InputXptID inSignalInput, const NTV2OutputXptID inSignalOutput)
{
    NTV2XptConnectionsIter	iter (mConnections.find (inSignalInput));
    if (iter == mConnections.end())
        return false;	//	Not in map
    if (iter->second != inSignalOutput)
        return false;	//	No match
    mConnections.erase (iter);
    return true;
}


static const ULWord	sSignalRouterRegMasks[]		=	{	0x000000FF,	0x0000FF00,	0x00FF0000,	0xFF000000	};
static const ULWord	sSignalRouterRegShifts[]	=	{	         0,	         8,	        16,	        24	};


bool CNTV2SignalRouter::ResetFromRegisters (const NTV2InputXptIDSet & inInputs, const NTV2RegisterReads & inRegReads)
{
	Reset();
	for (NTV2InputXptIDSetConstIter it(inInputs.begin());  it != inInputs.end();  ++it)
	{
		uint32_t	regNum(0),	maskNdx(0);
		CNTV2RegisterExpert::GetCrosspointSelectGroupRegisterInfo (*it, regNum, maskNdx);
		NTV2RegisterReadsConstIter	iter	(::FindFirstMatchingRegisterNumber(regNum, inRegReads));
		if (iter == inRegReads.end())
			continue;

		NTV2_ASSERT(iter->registerNumber == regNum);
		NTV2_ASSERT(iter->registerMask == 0xFFFFFFFF);
		NTV2_ASSERT(iter->registerShift == 0);
		NTV2_ASSERT(maskNdx < 4);
		const uint32_t	regValue	(iter->registerValue & sSignalRouterRegMasks[maskNdx]);
		const NTV2OutputXptID	outputXpt	(NTV2OutputXptID(regValue >> sSignalRouterRegShifts[maskNdx]));
		if (outputXpt != NTV2_XptBlack)
			mConnections.insert(NTV2SignalConnection (*it, outputXpt));
	}	//	for each NTV2InputXptID
	return true;
}


bool CNTV2SignalRouter::GetRegisterWrites (NTV2RegisterWrites & outRegWrites) const
{
    outRegWrites.clear ();

    for (NTV2XptConnectionsConstIter iter (mConnections.begin ());  iter != mConnections.end ();  ++iter)
    {
        const NTV2InputXptID	inputXpt(iter->first);
        const NTV2OutputXptID	outputXpt(iter->second);
        uint32_t regNum(0), ndx(999);

        if (!CNTV2RegisterExpert::GetCrosspointSelectGroupRegisterInfo (inputXpt, regNum, ndx)  ||  !regNum  ||  ndx > 3)
        {
            outRegWrites.clear();
            return false;
        }

        const NTV2RegInfo	regInfo	(regNum, outputXpt, sSignalRouterRegMasks[ndx], sSignalRouterRegShifts[ndx]);
        try
        {
            outRegWrites.push_back (regInfo);
        }
		catch (const bad_alloc &)
        {
            outRegWrites.clear ();
            return false;
        }
    }
    SRiDBG(outRegWrites);
    return true;
}


bool CNTV2SignalRouter::Compare (const CNTV2SignalRouter & inRHS, NTV2XptConnections & outNew,
								NTV2XptConnections & outChanged, NTV2XptConnections & outMissing) const
{
	outNew.clear();  outChanged.clear();  outMissing.clear();
	//	Check that my connections are also in RHS:
	for (NTV2XptConnectionsConstIter it(mConnections.begin());  it != mConnections.end();  ++it)
	{
		const NTV2SignalConnection &	connection (*it);
		const NTV2InputXptID			inputXpt(connection.first);
		const NTV2OutputXptID			outputXpt(connection.second);
		if (inRHS.HasConnection(inputXpt, outputXpt))
			;
		else if (inRHS.HasInput(inputXpt))
			outChanged.insert(NTV2Connection(inputXpt, inRHS.GetConnectedOutput(inputXpt)));	//	Connection changed from this
		else
			outNew.insert(connection);		//	Connection is new in me, not in RHS
	}

	//	Check that RHS' connections are also in me...
	const NTV2XptConnections	connectionsRHS(inRHS.GetConnections());
	for (NTV2XptConnectionsConstIter it(connectionsRHS.begin());  it != connectionsRHS.end();  ++it)
	{
		const NTV2SignalConnection &	connectionRHS (*it);
		const NTV2InputXptID			inputXpt(connectionRHS.first);
		const NTV2OutputXptID			outputXpt(connectionRHS.second);
		NTV2XptConnectionsConstIter		pFind (mConnections.find(inputXpt));
		if (pFind == mConnections.end())		//	If not found in me...
			outMissing.insert(connectionRHS);	//	...in RHS, but missing in me
		else if (pFind->second != outputXpt)	//	If output xpt differs...
			outChanged.insert(connectionRHS);	//	...then 'connection' is changed (in RHS, not in me)
	}

	return outNew.empty() && outChanged.empty() && outMissing.empty();	//	Return true if identical
}


ostream & CNTV2SignalRouter::Print (ostream & inOutStream, const bool inForRetailDisplay) const
{
    if (inForRetailDisplay)
    {
        inOutStream << mConnections.size() << " routing entries:" << endl;
        for (NTV2XptConnectionsConstIter iter (mConnections.begin());  iter != mConnections.end();  ++iter)
            inOutStream << ::NTV2InputCrosspointIDToString(iter->first, inForRetailDisplay)
						<< " <== " << ::NTV2OutputCrosspointIDToString(iter->second, inForRetailDisplay) << endl;
    }
    else
    {
        for (NTV2XptConnectionsConstIter iter (mConnections.begin());  iter != mConnections.end();  ++iter)
            inOutStream << CNTV2SignalRouter::NTV2InputCrosspointIDToString(iter->first)
            			<< " <== " << CNTV2SignalRouter::NTV2OutputCrosspointIDToString(iter->second) << endl;
    }
    return inOutStream;
}


bool CNTV2SignalRouter::PrintCode (string & outCode, const PrintCodeConfig & inConfig) const
{
	return ToCodeString(outCode, mConnections, inConfig);

}	//	PrintCode


bool CNTV2SignalRouter::ToCodeString (string & outCode, const NTV2XptConnections & inConnections,
										const PrintCodeConfig & inConfig)
{
	ostringstream	oss;

	outCode.clear ();

	if (inConfig.mShowComments)
	{
		oss << inConfig.mPreCommentText << DEC(inConnections.size()) << " routing ";
		oss << ((inConnections.size () == 1) ? "entry:" : "entries:");
		oss << inConfig.mPostCommentText << inConfig.mLineBreakText;
	}

	if (inConfig.mShowDeclarations)
	{
		if (inConfig.mUseRouter)
			oss << inConfig.mPreClassText << "CNTV2SignalRouter" << inConfig.mPostClassText
				<< "\t"<< inConfig.mPreVariableText << inConfig.mRouterVarName<< inConfig.mPostVariableText;
		else
			oss << inConfig.mPreClassText << "CNTV2Card" << inConfig.mPostClassText
				<< "\t" << inConfig.mPreVariableText << inConfig.mDeviceVarName<< inConfig.mPostVariableText;
		oss << ";" << inConfig.mLineBreakText;
	}

	const string	varName				(inConfig.mUseRouter ? inConfig.mRouterVarName : inConfig.mDeviceVarName);
	const string	variableNameText	(inConfig.mPreVariableText + varName + inConfig.mPostVariableText);
	const string	funcName			(inConfig.mUseRouter ? "AddConnection" : "Connect");
	const string	functionCallText	(inConfig.mPreFunctionText + funcName + inConfig.mPostFunctionText);
	for (NTV2XptConnectionsConstIter iter (inConnections.begin ());  iter != inConnections.end ();  ++iter)
	{
		const string	inXptStr	(inConfig.mPreXptText + ::NTV2InputCrosspointIDToString(iter->first, false) + inConfig.mPostXptText);
		const string	outXptStr	(inConfig.mPreXptText + ::NTV2OutputCrosspointIDToString(iter->second, false) + inConfig.mPostXptText);

		oss << variableNameText << "." << functionCallText << " (" << inXptStr << ", " << outXptStr << ");";

		if (inConfig.mShowComments)
		{
			NTV2XptConnectionsConstIter pNew(inConfig.mNew.find(iter->first));
			NTV2XptConnectionsConstIter pChanged(inConfig.mChanged.find(iter->first));
			if (pNew != inConfig.mNew.end()  &&  pNew->second == iter->second)
				oss << inConfig.mFieldBreakText << inConfig.mPreCommentText << "New" << inConfig.mPostCommentText;
			else if (pChanged != inConfig.mChanged.end()  &&  pChanged->second != iter->second)
				oss << inConfig.mFieldBreakText << inConfig.mPreCommentText << "Changed from "
					<< ::NTV2OutputCrosspointIDToString(pChanged->second, false) << inConfig.mPostCommentText;
		}
		oss << inConfig.mLineBreakText;
	}	//	for each connection

	if (inConfig.mShowComments)
		for (NTV2XptConnectionsConstIter pGone(inConfig.mMissing.begin());  pGone != inConfig.mMissing.end();  ++pGone)
			if (inConnections.find(pGone->first) == inConnections.end())
			{
				if (inConfig.mUseRouter)
					oss << inConfig.mPreCommentText << varName << "." << "RemoveConnection" << " ("
						<< ::NTV2InputCrosspointIDToString(pGone->first, false)
						<< ", " << ::NTV2OutputCrosspointIDToString(pGone->second, false)
						<< ");" << inConfig.mPostCommentText
						<< inConfig.mFieldBreakText << inConfig.mPreCommentText << "Deleted" << inConfig.mPostCommentText
						<< inConfig.mLineBreakText;
				else
					oss << inConfig.mPreCommentText << varName << "." << "Disconnect" << " ("
						<< ::NTV2InputCrosspointIDToString(pGone->first, false)
						<< ");" << inConfig.mPostCommentText << inConfig.mFieldBreakText
						<< inConfig.mPreCommentText
							<< "From " << ::NTV2OutputCrosspointIDToString(pGone->second, false)
						<< inConfig.mPostCommentText << inConfig.mLineBreakText;
			}

	outCode = oss.str();
	return true;
}


CNTV2SignalRouter::PrintCodeConfig::PrintCodeConfig ()
    :	mShowComments		(true),
        mShowDeclarations	(true),
        mUseRouter			(false),
        mPreCommentText		("// "),
        mPostCommentText	(),
        mPreClassText		(),
        mPostClassText		(),
        mPreVariableText	(),
        mPostVariableText	(),
        mPreXptText			(),
        mPostXptText		(),
        mPreFunctionText	(),
        mPostFunctionText	(),
        mDeviceVarName		("device"),
        mRouterVarName		("router"),
        mLineBreakText		("\n"),
        mFieldBreakText		("\t"),
        mNew				(),
        mChanged			(),
        mMissing			()
{
}


#if !defined (NTV2_DEPRECATE_12_5)
    bool CNTV2SignalRouter::PrintCode (string & outCode, const bool inShowComments, const bool inShowDeclarations, const bool inUseRouter) const
    {
        PrintCodeConfig	config;
        config.mShowComments		= inShowComments;
        config.mShowDeclarations	= inShowDeclarations;
        config.mUseRouter			= inUseRouter;
        return PrintCode (outCode, config);

    }	//	PrintCode
#endif	//	!defined (NTV2_DEPRECATE_12_5)


bool CNTV2SignalRouter::Initialize (void)		//	STATIC
{
	AJAAutoLock		locker(&gRoutingExpertLock);
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? true : false;
}	//	Initialize


bool CNTV2SignalRouter::Deinitialize (void)		//	STATIC
{
	return RoutingExpert::DisposeInstance();
}


bool CNTV2SignalRouter::IsInitialized (void)		//	STATIC
{
	AJAAutoLock		locker(&gRoutingExpertLock);
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance(false));
	return pExpert ? true : false;
}


string CNTV2SignalRouter::NTV2InputCrosspointIDToString (const NTV2InputXptID inInputXpt)		//	STATIC
{
	AJAAutoLock		locker(&gRoutingExpertLock);
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->InputXptToString(inInputXpt) : string();
}


string CNTV2SignalRouter::NTV2OutputCrosspointIDToString (const NTV2OutputXptID inOutputXpt)		//	STATIC
{
	AJAAutoLock		locker(&gRoutingExpertLock);
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->OutputXptToString(inOutputXpt) : string();
}


NTV2InputXptID CNTV2SignalRouter::StringToNTV2InputCrosspointID (const string & inStr)		//	STATIC
{
	AJAAutoLock		locker(&gRoutingExpertLock);
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->StringToInputXpt(inStr) : NTV2_INPUT_CROSSPOINT_INVALID;
}


NTV2OutputXptID CNTV2SignalRouter::StringToNTV2OutputCrosspointID (const string & inStr)		//	STATIC
{
	AJAAutoLock		locker(&gRoutingExpertLock);
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->StringToOutputXpt(inStr) : NTV2_OUTPUT_CROSSPOINT_INVALID;
}


bool CNTV2SignalRouter::GetWidgetIDs (const NTV2DeviceID inDeviceID, NTV2WidgetIDSet & outWidgets)		//	STATIC
{
    outWidgets.clear();
	for (NTV2WidgetID widgetID(NTV2WidgetID(NTV2_WIDGET_FIRST));  NTV2_IS_VALID_WIDGET(widgetID);  widgetID = NTV2WidgetID(widgetID+1))
        if (::NTV2DeviceCanDoWidget (inDeviceID, widgetID))
            outWidgets.insert(widgetID);
    return !outWidgets.empty();
}


bool CNTV2SignalRouter::GetWidgetsForInput (const NTV2InputXptID inInputXpt, NTV2WidgetIDSet & outWidgetIDs)		//	STATIC
{
	outWidgetIDs.clear();
	AJAAutoLock		locker(&gRoutingExpertLock);
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->GetWidgetsForInput(inInputXpt, outWidgetIDs) : false;
}


bool CNTV2SignalRouter::GetWidgetForInput (const NTV2InputXptID inInputXpt, NTV2WidgetID & outWidgetID, const NTV2DeviceID inDeviceID)		//	STATIC
{
	outWidgetID = NTV2_WIDGET_INVALID;
	NTV2WidgetIDSet	wgts;
	if (!GetWidgetsForInput(inInputXpt, wgts))
		return false;
	if (inDeviceID == DEVICE_ID_NOTFOUND)
		outWidgetID = *(wgts.begin());
	else
		for (NTV2WidgetIDSetConstIter it(wgts.begin());  it != wgts.end();  ++it)
			if (::NTV2DeviceCanDoWidget(inDeviceID, *it))
			{
				outWidgetID = *it;
				break;
			}
	return outWidgetID != NTV2_WIDGET_INVALID;
}


bool CNTV2SignalRouter::GetWidgetsForOutput (const NTV2OutputXptID inOutputXpt, NTV2WidgetIDSet & outWidgetIDs)		//	STATIC
{
	outWidgetIDs.clear();
	AJAAutoLock		locker(&gRoutingExpertLock);
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->GetWidgetsForOutput(inOutputXpt, outWidgetIDs) : false;
}


bool CNTV2SignalRouter::GetWidgetForOutput (const NTV2OutputXptID inOutputXpt, NTV2WidgetID & outWidgetID, const NTV2DeviceID inDeviceID)		//	STATIC
{
	outWidgetID = NTV2_WIDGET_INVALID;
	NTV2WidgetIDSet	wgts;
	{
		AJAAutoLock		locker(&gRoutingExpertLock);
		if (!GetWidgetsForOutput(inOutputXpt, wgts))
			return false;
	}
	if (inDeviceID == DEVICE_ID_NOTFOUND)
		outWidgetID = *(wgts.begin());
	else
		for (NTV2WidgetIDSetConstIter it(wgts.begin());  it != wgts.end();  ++it)
			if (::NTV2DeviceCanDoWidget(inDeviceID, *it))
			{
				outWidgetID = *it;
				break;
			}
	return outWidgetID != NTV2_WIDGET_INVALID;
}


bool CNTV2SignalRouter::GetWidgetInputs (const NTV2WidgetID inWidgetID, NTV2InputXptIDSet & outInputs)		//	STATIC
{
	outInputs.clear();
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->GetWidgetInputs(inWidgetID, outInputs) : false;
}


bool CNTV2SignalRouter::GetAllWidgetInputs (const NTV2DeviceID inDeviceID, NTV2InputXptIDSet & outInputs)		//	STATIC
{
    outInputs.clear();
    NTV2WidgetIDSet	widgetIDs;
    if (!GetWidgetIDs (inDeviceID, widgetIDs))
        return false;	//	Fail

    for (NTV2WidgetIDSetConstIter iter(widgetIDs.begin());  iter != widgetIDs.end ();  ++iter)
    {
        NTV2InputXptIDSet	inputs;
        CNTV2SignalRouter::GetWidgetInputs (*iter, inputs);
        outInputs.insert(inputs.begin(), inputs.end());
    }
    return true;
}


bool CNTV2SignalRouter::GetAllRoutingRegInfos (const NTV2InputXptIDSet & inInputs, NTV2RegisterWrites & outRegInfos) 	//	STATIC
{
    outRegInfos.clear();

	set<uint32_t>	regNums;
	uint32_t		regNum(0),	maskNdx(0);
	for (NTV2InputXptIDSetConstIter it(inInputs.begin());  it != inInputs.end();  ++it)
		if (CNTV2RegisterExpert::GetCrosspointSelectGroupRegisterInfo (*it, regNum, maskNdx))
			if (regNums.find(regNum) == regNums.end())
				regNums.insert(regNum);
	for (set<uint32_t>::const_iterator iter(regNums.begin());  iter != regNums.end();  ++iter)
		outRegInfos.push_back(NTV2RegInfo(*iter));

    return true;
}


bool CNTV2SignalRouter::GetWidgetOutputs (const NTV2WidgetID inWidgetID, NTV2OutputXptIDSet & outOutputs)		//	STATIC
{
	outOutputs.clear();
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->GetWidgetOutputs(inWidgetID, outOutputs) : false;
}

bool CNTV2SignalRouter::IsRGBOnlyInputXpt (const NTV2InputXptID inInputXpt)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsRGBOnlyInputXpt(inInputXpt) : false;
}

bool CNTV2SignalRouter::IsYUVOnlyInputXpt (const NTV2InputXptID inInputXpt)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsYUVOnlyInputXpt(inInputXpt) : false;
}

bool CNTV2SignalRouter::IsKeyInputXpt (const NTV2InputXptID inInputXpt)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsKeyInputXpt(inInputXpt) : false;
}

NTV2Channel CNTV2SignalRouter::WidgetIDToChannel(const NTV2WidgetID inWidgetID)
{
	RoutingExpertPtr pExpert(RoutingExpert::GetInstance());
	if (pExpert)
		return pExpert->WidgetIDToChannel(inWidgetID);
	return NTV2_CHANNEL_INVALID;
}

NTV2WidgetID CNTV2SignalRouter::WidgetIDFromTypeAndChannel(const NTV2WidgetType inWidgetType, const NTV2Channel inChannel)
{
	RoutingExpertPtr pExpert(RoutingExpert::GetInstance());
	if (pExpert)
		return pExpert->WidgetIDFromTypeAndChannel(inWidgetType, inChannel);
	return NTV2_WIDGET_INVALID;
}

NTV2WidgetType CNTV2SignalRouter::WidgetIDToType (const NTV2WidgetID inWidgetID)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	if (pExpert)
		return pExpert->WidgetIDToType(inWidgetID);
	return NTV2WidgetType_Invalid;
}

bool CNTV2SignalRouter::IsSDIWidgetType (const NTV2WidgetType inWidgetType)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsSDIWidget(inWidgetType) : false;
}

bool CNTV2SignalRouter::IsSDIInputWidgetType (const NTV2WidgetType inWidgetType)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsSDIInWidget(inWidgetType) : false;
}

bool CNTV2SignalRouter::IsSDIOutputWidgetType (const NTV2WidgetType inWidgetType)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsSDIOutWidget(inWidgetType) : false;
}

bool CNTV2SignalRouter::Is3GSDIWidgetType (const NTV2WidgetType inWidgetType)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->Is3GSDIWidget(inWidgetType) : false;
}

bool CNTV2SignalRouter::Is12GSDIWidgetType (const NTV2WidgetType inWidgetType)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->Is12GSDIWidget(inWidgetType) : false;
}

bool CNTV2SignalRouter::IsDualLinkWidgetType(const NTV2WidgetType inWidgetType)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsDualLinkWidget(inWidgetType) : false;
}

bool CNTV2SignalRouter::IsDualLinkInWidgetType(const NTV2WidgetType inWidgetType)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsDualLinkInWidget(inWidgetType) : false;
}

bool CNTV2SignalRouter::IsDualLinkOutWidgetType(const NTV2WidgetType inWidgetType)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsDualLinkOutWidget(inWidgetType) : false;
}

bool CNTV2SignalRouter::IsHDMIWidgetType(const NTV2WidgetType inWidgetType)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsHDMIWidget(inWidgetType) : false;
}

bool CNTV2SignalRouter::IsHDMIInWidgetType(const NTV2WidgetType inWidgetType)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsHDMIInWidget(inWidgetType) : false;
}

bool CNTV2SignalRouter::IsHDMIOutWidgetType(const NTV2WidgetType inWidgetType)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsHDMIOutWidget(inWidgetType) : false;
}

bool CNTV2SignalRouter::GetConnectionsFromRegs (const NTV2InputXptIDSet & inInputXptIDs, const NTV2RegisterReads & inRegValues, NTV2XptConnections & outConnections)
{
	outConnections.clear();
	for (NTV2InputXptIDSetConstIter it(inInputXptIDs.begin());  it != inInputXptIDs.end();  ++it)
	{
		uint32_t	regNum(0),	maskNdx(0);
		CNTV2RegisterExpert::GetCrosspointSelectGroupRegisterInfo (*it, regNum, maskNdx);
		NTV2RegisterReadsConstIter	iter	(::FindFirstMatchingRegisterNumber(regNum, inRegValues));
		if (iter == inRegValues.end())
			continue;

		if (iter->registerNumber != regNum)
			return false;	//	Register numbers must match here
		if (iter->registerMask != 0xFFFFFFFF)
			return false;	//	Mask must be 0xFFFFFFFF
		if (iter->registerShift)
			return false;	//	Shift must be zero
		NTV2_ASSERT(maskNdx < 4);
		const uint32_t	regValue	(iter->registerValue & sSignalRouterRegMasks[maskNdx]);
		const NTV2OutputXptID	outputXpt	(NTV2OutputXptID(regValue >> sSignalRouterRegShifts[maskNdx]));
		if (outputXpt != NTV2_XptBlack)
			outConnections.insert(NTV2SignalConnection (*it, outputXpt));
	}	//	for each NTV2InputXptID
	return true;
}


bool CNTV2SignalRouter::CompareConnections (const NTV2XptConnections & inLHS,
											const NTV2XptConnections & inRHS,
											NTV2XptConnections & outNew,
											NTV2XptConnections & outMissing)
{
	outNew.clear();  outMissing.clear();
	//	Check that LHS connections are also in RHS:
	for (NTV2XptConnectionsConstIter it(inLHS.begin());  it != inLHS.end();  ++it)
	{
		const NTV2SignalConnection &	LHSconnection(*it);
		const NTV2InputXptID			inputXpt(LHSconnection.first);
		const NTV2OutputXptID			outputXpt(LHSconnection.second);
		NTV2XptConnectionsConstIter		RHSit(inRHS.find(inputXpt));
		if (RHSit == inRHS.end())
			outMissing.insert(LHSconnection);	//	LHSConnection's inputXpt missing from RHS
		else if (RHSit->second == outputXpt)
			;	//	LHS's input xpt connected to same output xpt as RHS
		else
		{
			outMissing.insert(LHSconnection);	//	LHS connection missing from RHS
			outNew.insert(*RHSit);				//	RHS connection is new
		}
	}

	//	Check that RHS connections are also in LHS...
	for (NTV2XptConnectionsConstIter it(inRHS.begin());  it != inRHS.end();  ++it)
	{
		const NTV2SignalConnection &	connectionRHS (*it);
		const NTV2InputXptID			inputXpt(connectionRHS.first);
		const NTV2OutputXptID			outputXpt(connectionRHS.second);
		NTV2XptConnectionsConstIter		LHSit(inLHS.find(inputXpt));
		if (LHSit == inLHS.end())				//	If RHS input xpt not in LHS...
			outNew.insert(connectionRHS);		//	...then RHS connection is new
		else if (LHSit->second != outputXpt)	//	Else if output xpt changed...
			//	Should've already been handled in previous for loop
			NTV2_ASSERT(outMissing.find(LHSit->first) != outMissing.end()  &&  outNew.find(LHSit->first) != outNew.end());
	}

	return outNew.empty() && outMissing.empty();	//	Return true if identical
}


bool CNTV2SignalRouter::CreateFromString (const string & inString, NTV2XptConnections & outConnections)	//	STATIC
{
	NTV2StringList	lines;
	string	stringToParse(inString);    aja::strip(aja::lower(stringToParse));
	aja::replace(stringToParse, " ", "");
	aja::replace(stringToParse, "\t", "");
	aja::replace(stringToParse, "&lt;","<");	//	in case uuencoded

	outConnections.clear();
	if (Tokenize(stringToParse, lines, "\n\r", true).empty())	//	Split the string at line breaks
	{
		SRWARN("No lines resulted from input string '" << stringToParse << "'");
		return true;	//	Nothing there
	}

	if (lines.front().find("<==") != string::npos)
	{
		SRDBG(lines.size() << " lines");
		for (NTV2StringListConstIter pEachLine(lines.begin());  pEachLine != lines.end();  ++pEachLine)
		{
			SRDBG("  line '" << *pEachLine << "'");
			size_t	pos	(pEachLine->find("<=="));
			if (pos == string::npos)
				{SRFAIL("Parse error: '<==' missing in line '" << *pEachLine << "'");  return false;}
			string	leftPiece	(pEachLine->substr(0, pos));	aja::strip(leftPiece);
			string	rightPiece	(pEachLine->substr(pos + 3, pEachLine->length()));  aja::strip(rightPiece);
			NTV2InputXptID	inputXpt	(StringToNTV2InputCrosspointID(leftPiece));
			NTV2OutputXptID	outputXpt	(StringToNTV2OutputCrosspointID(rightPiece));
			//SRDBG(" L'" << leftPiece << "',  R'" << rightPiece << "'");
			if (inputXpt == NTV2_INPUT_CROSSPOINT_INVALID)
				{SRFAIL("Parse error: invalid input crosspoint from '" << leftPiece << "' from line '" << *pEachLine << "'");  	return false;}
			if (outConnections.find(inputXpt) != outConnections.end())
		        SRWARN("Overwriting " << ::NTV2InputCrosspointIDToString(inputXpt) << "-" << ::NTV2OutputCrosspointIDToString(outConnections[inputXpt])
						<< " with " << ::NTV2InputCrosspointIDToString(inputXpt) << "-" << ::NTV2OutputCrosspointIDToString(outputXpt));
			outConnections.insert(NTV2Connection(inputXpt, outputXpt));
		}	//	for each line
	}
	else if (lines.front().find("connect(") != string::npos)
	{
		for (NTV2StringListConstIter pLine(lines.begin());  pLine != lines.end();  ++pLine)
		{
			string line(*pLine);  aja::strip(line);
			if (line.empty())
				continue;
			SRDBG("  line '" << line << "'");
			size_t	openParenPos(line.find("(")), closedParenPos(line.find(");"));
			if (openParenPos == string::npos  ||  closedParenPos == string::npos  ||  openParenPos > closedParenPos)
				{SRFAIL("Parse error: '(' or ');' missing in line '" << line << "'");  return false;}
			string remainder(line.substr(openParenPos+1, closedParenPos - openParenPos - 1));
			NTV2StringList xptNames;
			aja::split(remainder, ',', xptNames);
			if (xptNames.size() < 2  ||  xptNames.size() > 2)
				{SRFAIL("Parse error: " << DEC(xptNames.size()) << " 'Connect' parameter(s) found, expected 2");  return false;}
			NTV2InputXptID	inputXpt (StringToNTV2InputCrosspointID(xptNames.at(0)));
			NTV2OutputXptID	outputXpt (StringToNTV2OutputCrosspointID(xptNames.at(1)));
			//SRDBG(" L'" << xptNames.at(0) << "',  R'" << xptNames.at(1) << "'");
			if (inputXpt == NTV2_INPUT_CROSSPOINT_INVALID)
				{SRFAIL("Parse error: invalid input crosspoint from '" << xptNames.at(0) << "' from line '" << *pLine << "'");  return false;}
			if (outputXpt == NTV2_OUTPUT_CROSSPOINT_INVALID)
				{SRFAIL("Parse error: invalid output crosspoint from '" << xptNames.at(1) << "' from line '" << *pLine << "'");  return false;}
			if (outConnections.find(inputXpt) != outConnections.end())
		        SRWARN("Overwriting " << ::NTV2InputCrosspointIDToString(inputXpt) << "-" << ::NTV2OutputCrosspointIDToString(outConnections[inputXpt])
						<< " with " << ::NTV2InputCrosspointIDToString(inputXpt) << "-" << ::NTV2OutputCrosspointIDToString(outputXpt));
			outConnections.insert(NTV2Connection(inputXpt, outputXpt));
		}	//	for each line
	}
	else
		{SRFAIL("Unable to parse '" << lines.front() << "' -- expected '.contains(' or '<=='");  return false;}
	SRINFO(DEC(outConnections.size()) << " connection(s) created from input string");
    return true;
}


bool CNTV2SignalRouter::CreateFromString (const string & inString, CNTV2SignalRouter & outRouter)	//	STATIC
{
    NTV2XptConnections	connections;
    outRouter.Reset();
    if (!CreateFromString(inString, connections))
		return false;
    return outRouter.ResetFrom(connections);
}

#if !defined (NTV2_DEPRECATE)
    bool CNTV2SignalRouter::addWithRegisterAndValue (const NTV2RoutingEntry & inEntry, const ULWord inRegisterNum, const ULWord inValue, const ULWord inMask, const ULWord inShift)
    {
        NTV2RoutingEntry	foo	= inEntry;

        if (GetNumberOfConnections () >= MAX_ROUTING_ENTRIES)
            return false;

        foo.value		= inValue;
        foo.registerNum	= inRegisterNum;
        foo.mask		= inMask;
        foo.shift		= inShift;
        return add (foo);
    }

    CNTV2SignalRouter::operator NTV2RoutingTable () const
    {
        NTV2RoutingTable	result;
        GetRoutingTable (result);
        return result;
    }

    CNTV2SignalRouter::operator NTV2RoutingTable* ()
    {
        static NTV2RoutingTable	result;
        GetRoutingTable (result);
        return &result;
    }

    const NTV2RoutingEntry & CNTV2SignalRouter::GetRoutingEntry (const ULWord inIndex) const
    {
        static NTV2RoutingTable	result;
        static NTV2RoutingEntry	nullEntry	= {0, 0, 0, 0};
        GetRoutingTable (result);
        if (inIndex < result.numEntries)
            return result.routingEntry [inIndex];
        else
            return nullEntry;
    }

    bool CNTV2SignalRouter::GetRoutingTable (NTV2RoutingTable & outRoutingTable) const
    {
        NTV2RegisterWrites	regInfos;
        ::memset (&outRoutingTable, 0, sizeof (NTV2RoutingTable));
        if (!GetRegisterWrites (regInfos))
            return false;
        for (NTV2RegisterWritesConstIter iter (regInfos.begin ());  iter != regInfos.end ();  ++iter)
        {
            NTV2RoutingEntry &	entry	(outRoutingTable.routingEntry [outRoutingTable.numEntries++]);
            entry = *iter;
            if (outRoutingTable.numEntries >= MAX_ROUTING_ENTRIES)
                break;
        }
        return true;
    }
#endif	//	!defined (NTV2_DEPRECATE)


#if !defined (NTV2_DEPRECATE_12_5)
    bool CNTV2SignalRouter::addWithValue (const NTV2RoutingEntry & inEntry, const ULWord inValue)
    {
        if (GetNumberOfConnections () >= MAX_ROUTING_ENTRIES)
            return false;

        return AddConnection (NTV2RoutingEntryToInputCrosspointID (inEntry), NTV2OutputCrosspointID (inValue));
    }


    bool CNTV2SignalRouter::AddConnection (const NTV2RoutingEntry & inEntry, const NTV2OutputCrosspointID inSignalOutput)
    {
        return AddConnection (NTV2RoutingEntryToInputCrosspointID (inEntry), inSignalOutput);
    }
#endif	//	!defined (NTV2_DEPRECATE_12_5)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// CNTV2SignalRouter	End



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Crosspoint Utils	Begin

NTV2InputXptID GetFrameBufferInputXptFromChannel (const NTV2Channel inChannel, const bool inIsBInput)
{
    static const NTV2InputXptID	gFrameBufferInputs []	=	{	NTV2_XptFrameBuffer1Input,	NTV2_XptFrameBuffer2Input,	NTV2_XptFrameBuffer3Input,	NTV2_XptFrameBuffer4Input,
																NTV2_XptFrameBuffer5Input,	NTV2_XptFrameBuffer6Input,	NTV2_XptFrameBuffer7Input,	NTV2_XptFrameBuffer8Input};
    static const NTV2InputXptID	gFrameBufferBInputs []	=	{	NTV2_XptFrameBuffer1BInput,	NTV2_XptFrameBuffer2BInput,	NTV2_XptFrameBuffer3BInput,	NTV2_XptFrameBuffer4BInput,
																NTV2_XptFrameBuffer5BInput,	NTV2_XptFrameBuffer6BInput,	NTV2_XptFrameBuffer7BInput,	NTV2_XptFrameBuffer8BInput};
    if (NTV2_IS_VALID_CHANNEL (inChannel))
        return inIsBInput ? gFrameBufferBInputs [inChannel] : gFrameBufferInputs [inChannel];
    else
        return NTV2_INPUT_CROSSPOINT_INVALID;
}


NTV2InputXptID GetCSCInputXptFromChannel (const NTV2Channel inChannel, const bool inIsKeyInput)
{
    static const NTV2InputXptID	gCSCVideoInput [] = {	NTV2_XptCSC1VidInput,	NTV2_XptCSC2VidInput,	NTV2_XptCSC3VidInput,	NTV2_XptCSC4VidInput,
														NTV2_XptCSC5VidInput,	NTV2_XptCSC6VidInput,	NTV2_XptCSC7VidInput,	NTV2_XptCSC8VidInput};
    static const NTV2InputXptID	gCSCKeyInput [] = {		NTV2_XptCSC1KeyInput,	NTV2_XptCSC2KeyInput,	NTV2_XptCSC3KeyInput,	NTV2_XptCSC4KeyInput,
														NTV2_XptCSC5KeyInput,	NTV2_XptCSC6KeyInput,	NTV2_XptCSC7KeyInput,	NTV2_XptCSC8KeyInput};
    if (NTV2_IS_VALID_CHANNEL(inChannel))
        return inIsKeyInput ? gCSCKeyInput[inChannel] : gCSCVideoInput[inChannel];
    else
        return NTV2_INPUT_CROSSPOINT_INVALID;
}


NTV2InputXptID GetLUTInputXptFromChannel (const NTV2Channel inLUT)
{
    static const NTV2InputXptID	gLUTInput[] = {	NTV2_XptLUT1Input,	NTV2_XptLUT2Input,	NTV2_XptLUT3Input,	NTV2_XptLUT4Input,
												NTV2_XptLUT5Input,	NTV2_XptLUT6Input,	NTV2_XptLUT7Input,	NTV2_XptLUT8Input};
	return NTV2_IS_VALID_CHANNEL(inLUT) ? gLUTInput[inLUT] : NTV2_INPUT_CROSSPOINT_INVALID;
}


NTV2InputXptID GetDLInInputXptFromChannel (const NTV2Channel inChannel, const bool inLinkB)
{
    static const NTV2InputXptID	gDLInputs[] = {	NTV2_XptDualLinkIn1Input, NTV2_XptDualLinkIn2Input, NTV2_XptDualLinkIn3Input, NTV2_XptDualLinkIn4Input,
												NTV2_XptDualLinkIn5Input, NTV2_XptDualLinkIn6Input, NTV2_XptDualLinkIn7Input, NTV2_XptDualLinkIn8Input };
    static const NTV2InputXptID	gDLBInputs[] = {NTV2_XptDualLinkIn1DSInput, NTV2_XptDualLinkIn2DSInput, NTV2_XptDualLinkIn3DSInput, NTV2_XptDualLinkIn4DSInput,
												NTV2_XptDualLinkIn5DSInput, NTV2_XptDualLinkIn6DSInput, NTV2_XptDualLinkIn7DSInput, NTV2_XptDualLinkIn8DSInput };
    if (NTV2_IS_VALID_CHANNEL(inChannel))
        return inLinkB ? gDLBInputs[inChannel] : gDLInputs[inChannel];
    else
        return NTV2_INPUT_CROSSPOINT_INVALID;
}

NTV2InputXptID GetDLOutInputXptFromChannel (const NTV2Channel inChannel)
{
    static const NTV2InputXptID	gDLOutInputs[] = {	NTV2_XptDualLinkOut1Input, NTV2_XptDualLinkOut2Input, NTV2_XptDualLinkOut3Input, NTV2_XptDualLinkOut4Input,
													NTV2_XptDualLinkOut5Input, NTV2_XptDualLinkOut6Input, NTV2_XptDualLinkOut7Input, NTV2_XptDualLinkOut8Input };
    if (NTV2_IS_VALID_CHANNEL(inChannel))
        return gDLOutInputs[inChannel];
    else
        return NTV2_INPUT_CROSSPOINT_INVALID;
}


NTV2OutputXptID GetCSCOutputXptFromChannel (const NTV2Channel inChannel, const bool inIsKey, const bool inIsRGB)
{
	static const NTV2OutputXptID gCSCKeyOutputs [] = {	NTV2_XptCSC1KeyYUV,		NTV2_XptCSC2KeyYUV,		NTV2_XptCSC3KeyYUV,		NTV2_XptCSC4KeyYUV,
														NTV2_XptCSC5KeyYUV,		NTV2_XptCSC6KeyYUV,		NTV2_XptCSC7KeyYUV,		NTV2_XptCSC8KeyYUV};
	static const NTV2OutputXptID gCSCRGBOutputs [] = {	NTV2_XptCSC1VidRGB,		NTV2_XptCSC2VidRGB,		NTV2_XptCSC3VidRGB,		NTV2_XptCSC4VidRGB,
														NTV2_XptCSC5VidRGB,		NTV2_XptCSC6VidRGB,		NTV2_XptCSC7VidRGB,		NTV2_XptCSC8VidRGB};
	static const NTV2OutputXptID gCSCYUVOutputs [] = {	NTV2_XptCSC1VidYUV,		NTV2_XptCSC2VidYUV,		NTV2_XptCSC3VidYUV,		NTV2_XptCSC4VidYUV,
														NTV2_XptCSC5VidYUV,		NTV2_XptCSC6VidYUV,		NTV2_XptCSC7VidYUV,		NTV2_XptCSC8VidYUV};
	if (NTV2_IS_VALID_CHANNEL(inChannel))
	{
		if (inIsKey)
			return gCSCKeyOutputs[inChannel];
		else
			return inIsRGB  ?  gCSCRGBOutputs[inChannel]  :  gCSCYUVOutputs[inChannel];
	}
	else
		return NTV2_OUTPUT_CROSSPOINT_INVALID;
}

NTV2OutputXptID GetLUTOutputXptFromChannel (const NTV2Channel inLUT)
{
	static const NTV2OutputXptID gLUTRGBOutputs[] = {	NTV2_XptLUT1Out, NTV2_XptLUT2Out, NTV2_XptLUT3Out, NTV2_XptLUT4Out,
														NTV2_XptLUT5Out, NTV2_XptLUT6Out, NTV2_XptLUT7Out, NTV2_XptLUT8Out};
	return NTV2_IS_VALID_CHANNEL(inLUT) ? gLUTRGBOutputs[inLUT] : NTV2_OUTPUT_CROSSPOINT_INVALID;
}

NTV2OutputXptID GetFrameBufferOutputXptFromChannel (const NTV2Channel inChannel, const bool inIsRGB, const bool inIs425)
{
    static const NTV2OutputXptID gFrameBufferYUVOutputs[] = {		NTV2_XptFrameBuffer1YUV,		NTV2_XptFrameBuffer2YUV,		NTV2_XptFrameBuffer3YUV,		NTV2_XptFrameBuffer4YUV,
																	NTV2_XptFrameBuffer5YUV,		NTV2_XptFrameBuffer6YUV,		NTV2_XptFrameBuffer7YUV,		NTV2_XptFrameBuffer8YUV};
    static const NTV2OutputXptID gFrameBufferRGBOutputs[] = {		NTV2_XptFrameBuffer1RGB,		NTV2_XptFrameBuffer2RGB,		NTV2_XptFrameBuffer3RGB,		NTV2_XptFrameBuffer4RGB,
																	NTV2_XptFrameBuffer5RGB,		NTV2_XptFrameBuffer6RGB,		NTV2_XptFrameBuffer7RGB,		NTV2_XptFrameBuffer8RGB};
    static const NTV2OutputXptID gFrameBufferYUV425Outputs[] = {	NTV2_XptFrameBuffer1_DS2YUV,	NTV2_XptFrameBuffer2_DS2YUV,	NTV2_XptFrameBuffer3_DS2YUV,	NTV2_XptFrameBuffer4_DS2YUV,
																	NTV2_XptFrameBuffer5_DS2YUV,	NTV2_XptFrameBuffer6_DS2YUV,	NTV2_XptFrameBuffer7_DS2YUV,	NTV2_XptFrameBuffer8_DS2YUV};
    static const NTV2OutputXptID gFrameBufferRGB425Outputs[] = {	NTV2_XptFrameBuffer1_DS2RGB,	NTV2_XptFrameBuffer2_DS2RGB,	NTV2_XptFrameBuffer3_DS2RGB,	NTV2_XptFrameBuffer4_DS2RGB,
																	NTV2_XptFrameBuffer5_DS2RGB,	NTV2_XptFrameBuffer6_DS2RGB,	NTV2_XptFrameBuffer7_DS2RGB,	NTV2_XptFrameBuffer8_DS2RGB};
    if (NTV2_IS_VALID_CHANNEL(inChannel))
        if (inIs425)
            return inIsRGB ? gFrameBufferRGB425Outputs[inChannel] : gFrameBufferYUV425Outputs[inChannel];
        else
            return inIsRGB ? gFrameBufferRGBOutputs[inChannel] : gFrameBufferYUVOutputs[inChannel];
    else
        return NTV2_OUTPUT_CROSSPOINT_INVALID;
}


NTV2OutputXptID GetInputSourceOutputXpt (const NTV2InputSource inInputSource, const bool inIsSDI_DS2, const bool inIsHDMI_RGB, const UWord inHDMI_Quadrant)
{
	static const NTV2OutputXptID gHDMIInputOutputs [4][4] =		{	{	NTV2_XptHDMIIn1,	NTV2_XptHDMIIn1Q2,		NTV2_XptHDMIIn1Q3,		NTV2_XptHDMIIn1Q4		},
																	{   NTV2_XptHDMIIn2,	NTV2_XptHDMIIn2Q2,		NTV2_XptHDMIIn2Q3,		NTV2_XptHDMIIn2Q4		},
																	{   NTV2_XptHDMIIn3,	NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack			},
																	{   NTV2_XptHDMIIn4,	NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack			}   };
	static const NTV2OutputXptID gHDMIInputRGBOutputs [4][4] =	{	{	NTV2_XptHDMIIn1RGB, NTV2_XptHDMIIn1Q2RGB,	NTV2_XptHDMIIn1Q3RGB,	NTV2_XptHDMIIn1Q4RGB	},
																	{	NTV2_XptHDMIIn2RGB,	NTV2_XptHDMIIn2Q2RGB,	NTV2_XptHDMIIn2Q3RGB,	NTV2_XptHDMIIn2Q4RGB	},
																	{	NTV2_XptHDMIIn3RGB,	NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack			},
																	{	NTV2_XptHDMIIn4RGB,	NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack			}	};

    if (NTV2_INPUT_SOURCE_IS_SDI (inInputSource))
        return ::GetSDIInputOutputXptFromChannel (::NTV2InputSourceToChannel (inInputSource), inIsSDI_DS2);
    else if (NTV2_INPUT_SOURCE_IS_HDMI (inInputSource))
    {
		NTV2Channel channel = ::NTV2InputSourceToChannel (inInputSource);
        if (inHDMI_Quadrant < 4)
			return inIsHDMI_RGB  ?  gHDMIInputRGBOutputs [channel][inHDMI_Quadrant]  :  gHDMIInputOutputs [channel][inHDMI_Quadrant];
        else
            return NTV2_OUTPUT_CROSSPOINT_INVALID;
    }
    else if (NTV2_INPUT_SOURCE_IS_ANALOG (inInputSource))
        return NTV2_XptAnalogIn;
    else
        return NTV2_OUTPUT_CROSSPOINT_INVALID;
}


NTV2OutputXptID GetSDIInputOutputXptFromChannel (const NTV2Channel inChannel,  const bool inIsDS2)
{
    static const NTV2OutputXptID gSDIInputOutputs []	=	{	NTV2_XptSDIIn1,		NTV2_XptSDIIn2,		NTV2_XptSDIIn3,		NTV2_XptSDIIn4,
																NTV2_XptSDIIn5,		NTV2_XptSDIIn6,		NTV2_XptSDIIn7,		NTV2_XptSDIIn8};
    static const NTV2OutputXptID gSDIInputDS2Outputs []	=	{	NTV2_XptSDIIn1DS2,	NTV2_XptSDIIn2DS2,	NTV2_XptSDIIn3DS2,	NTV2_XptSDIIn4DS2,
																NTV2_XptSDIIn5DS2,	NTV2_XptSDIIn6DS2,	NTV2_XptSDIIn7DS2,	NTV2_XptSDIIn8DS2};
    if (NTV2_IS_VALID_CHANNEL(inChannel))
        return inIsDS2  ?  gSDIInputDS2Outputs[inChannel]  :  gSDIInputOutputs[inChannel];
    else
        return NTV2_OUTPUT_CROSSPOINT_INVALID;
}

NTV2OutputXptID GetDLOutOutputXptFromChannel(const NTV2Channel inChannel, const bool inIsLinkB)
{
    static const NTV2OutputXptID gDLOutOutputs[] = {	NTV2_XptDuallinkOut1, NTV2_XptDuallinkOut2, NTV2_XptDuallinkOut3, NTV2_XptDuallinkOut4,
														NTV2_XptDuallinkOut5, NTV2_XptDuallinkOut6, NTV2_XptDuallinkOut7, NTV2_XptDuallinkOut8 };
    static const NTV2OutputXptID gDLOutDS2Outputs[] = {	NTV2_XptDuallinkOut1DS2, NTV2_XptDuallinkOut2DS2, NTV2_XptDuallinkOut3DS2, NTV2_XptDuallinkOut4DS2,
														NTV2_XptDuallinkOut5DS2, NTV2_XptDuallinkOut6DS2, NTV2_XptDuallinkOut7DS2, NTV2_XptDuallinkOut8DS2 };
    if (NTV2_IS_VALID_CHANNEL(inChannel))
        return inIsLinkB ? gDLOutDS2Outputs[inChannel] : gDLOutOutputs[inChannel];
    else
        return NTV2_OUTPUT_CROSSPOINT_INVALID;
}

NTV2OutputXptID GetDLInOutputXptFromChannel(const NTV2Channel inChannel)
{
    static const NTV2OutputXptID gDLInOutputs[] = {	NTV2_XptDuallinkIn1, NTV2_XptDuallinkIn2, NTV2_XptDuallinkIn3, NTV2_XptDuallinkIn4,
													NTV2_XptDuallinkIn5, NTV2_XptDuallinkIn6, NTV2_XptDuallinkIn7, NTV2_XptDuallinkIn8 };
    if (NTV2_IS_VALID_CHANNEL(inChannel))
        return gDLInOutputs[inChannel];
    else
        return NTV2_OUTPUT_CROSSPOINT_INVALID;
}


NTV2InputXptID GetOutputDestInputXpt (const NTV2OutputDestination inOutputDest,  const bool inIsSDI_DS2,  const UWord inHDMI_Quadrant)
{
    static const NTV2InputXptID	gHDMIOutputInputs[] = {	NTV2_XptHDMIOutQ1Input,	NTV2_XptHDMIOutQ2Input,	NTV2_XptHDMIOutQ3Input,	NTV2_XptHDMIOutQ4Input};
    if (NTV2_OUTPUT_DEST_IS_SDI(inOutputDest))
        return ::GetSDIOutputInputXpt (::NTV2OutputDestinationToChannel(inOutputDest), inIsSDI_DS2);
    else if (NTV2_OUTPUT_DEST_IS_HDMI(inOutputDest))
        return inHDMI_Quadrant > 3  ?  NTV2_XptHDMIOutInput :  gHDMIOutputInputs[inHDMI_Quadrant];
    else if (NTV2_OUTPUT_DEST_IS_ANALOG(inOutputDest))
        return NTV2_XptAnalogOutInput;
    else
        return NTV2_INPUT_CROSSPOINT_INVALID;
}


NTV2InputXptID GetSDIOutputInputXpt (const NTV2Channel inChannel,  const bool inIsDS2)
{
    static const NTV2InputXptID	gSDIOutputInputs []		=	{	NTV2_XptSDIOut1Input,		NTV2_XptSDIOut2Input,		NTV2_XptSDIOut3Input,		NTV2_XptSDIOut4Input,
																NTV2_XptSDIOut5Input,		NTV2_XptSDIOut6Input,		NTV2_XptSDIOut7Input,		NTV2_XptSDIOut8Input};
    static const NTV2InputXptID	gSDIOutputDS2Inputs []	=	{	NTV2_XptSDIOut1InputDS2,	NTV2_XptSDIOut2InputDS2,	NTV2_XptSDIOut3InputDS2,	NTV2_XptSDIOut4InputDS2,
																NTV2_XptSDIOut5InputDS2,	NTV2_XptSDIOut6InputDS2,	NTV2_XptSDIOut7InputDS2,	NTV2_XptSDIOut8InputDS2};
    if (NTV2_IS_VALID_CHANNEL (inChannel))
        return inIsDS2  ?  gSDIOutputDS2Inputs [inChannel]  :  gSDIOutputInputs [inChannel];
    else
        return NTV2_INPUT_CROSSPOINT_INVALID;
}


NTV2OutputXptID GetMixerOutputXptFromChannel (const NTV2Channel inChannel, const bool inIsKey)
{
    static const NTV2OutputXptID gMixerVidYUVOutputs []	= {	NTV2_XptMixer1VidYUV,	NTV2_XptMixer1VidYUV,	NTV2_XptMixer2VidYUV,	NTV2_XptMixer2VidYUV,
															NTV2_XptMixer3VidYUV,	NTV2_XptMixer3VidYUV,	NTV2_XptMixer4VidYUV,	NTV2_XptMixer4VidYUV};
    static const NTV2OutputXptID gMixerKeyYUVOutputs []	= {	NTV2_XptMixer1KeyYUV,	NTV2_XptMixer1KeyYUV,	NTV2_XptMixer2KeyYUV,	NTV2_XptMixer2KeyYUV,
															NTV2_XptMixer3KeyYUV,	NTV2_XptMixer3KeyYUV,	NTV2_XptMixer4KeyYUV,	NTV2_XptMixer4KeyYUV};
    if (NTV2_IS_VALID_CHANNEL(inChannel))
        return inIsKey  ?  gMixerKeyYUVOutputs[inChannel]  :  gMixerVidYUVOutputs[inChannel];
    else
        return NTV2_OUTPUT_CROSSPOINT_INVALID;
}


NTV2InputXptID GetMixerFGInputXpt (const NTV2Channel inChannel,  const bool inIsKey)
{
    static const NTV2InputXptID	gMixerFGVideoInputs []	= {	NTV2_XptMixer1FGVidInput,	NTV2_XptMixer1FGVidInput,	NTV2_XptMixer2FGVidInput,	NTV2_XptMixer2FGVidInput,
															NTV2_XptMixer3FGVidInput,	NTV2_XptMixer3FGVidInput,	NTV2_XptMixer4FGVidInput,	NTV2_XptMixer4FGVidInput};
    static const NTV2InputXptID	gMixerFGKeyInputs []	= {	NTV2_XptMixer1FGKeyInput,	NTV2_XptMixer1FGKeyInput,	NTV2_XptMixer2FGKeyInput,	NTV2_XptMixer2FGKeyInput,
															NTV2_XptMixer3FGKeyInput,	NTV2_XptMixer3FGKeyInput,	NTV2_XptMixer4FGKeyInput,	NTV2_XptMixer4FGKeyInput};
    if (NTV2_IS_VALID_CHANNEL(inChannel))
        return inIsKey  ?  gMixerFGKeyInputs[inChannel]  :  gMixerFGVideoInputs[inChannel];
    else
        return NTV2_INPUT_CROSSPOINT_INVALID;
}


NTV2InputXptID GetMixerBGInputXpt (const NTV2Channel inChannel,  const bool inIsKey)
{
    static const NTV2InputXptID	gMixerBGVideoInputs []	= {	NTV2_XptMixer1BGVidInput,	NTV2_XptMixer1BGVidInput,	NTV2_XptMixer2BGVidInput,	NTV2_XptMixer2BGVidInput,
															NTV2_XptMixer3BGVidInput,	NTV2_XptMixer3BGVidInput,	NTV2_XptMixer4BGVidInput,	NTV2_XptMixer4BGVidInput};
    static const NTV2InputXptID	gMixerBGKeyInputs []	= {	NTV2_XptMixer1BGKeyInput,	NTV2_XptMixer1BGKeyInput,	NTV2_XptMixer2BGKeyInput,	NTV2_XptMixer2BGKeyInput,
															NTV2_XptMixer3BGKeyInput,	NTV2_XptMixer3BGKeyInput,	NTV2_XptMixer4BGKeyInput,	NTV2_XptMixer4BGKeyInput};
    if (NTV2_IS_VALID_CHANNEL(inChannel))
        return inIsKey  ?  gMixerBGKeyInputs[inChannel]  :  gMixerBGVideoInputs[inChannel];
    else
        return NTV2_INPUT_CROSSPOINT_INVALID;
}

NTV2InputXptID GetTSIMuxInputXptFromChannel (const NTV2Channel inChannel, const bool inLinkB)
{
    static const NTV2InputXptID	gDLInputs[] = {	NTV2_Xpt425Mux1AInput, NTV2_Xpt425Mux2AInput, NTV2_Xpt425Mux3AInput, NTV2_Xpt425Mux4AInput,
												NTV2_INPUT_CROSSPOINT_INVALID,NTV2_INPUT_CROSSPOINT_INVALID,NTV2_INPUT_CROSSPOINT_INVALID,NTV2_INPUT_CROSSPOINT_INVALID};
    static const NTV2InputXptID	gDLBInputs[]= {	NTV2_Xpt425Mux1BInput, NTV2_Xpt425Mux2BInput, NTV2_Xpt425Mux3BInput, NTV2_Xpt425Mux4BInput,
												NTV2_INPUT_CROSSPOINT_INVALID,NTV2_INPUT_CROSSPOINT_INVALID,NTV2_INPUT_CROSSPOINT_INVALID,NTV2_INPUT_CROSSPOINT_INVALID};
    if (NTV2_IS_VALID_CHANNEL(inChannel))
        return inLinkB ? gDLBInputs[inChannel] : gDLInputs[inChannel];
    else
        return NTV2_INPUT_CROSSPOINT_INVALID;
}

NTV2OutputXptID GetTSIMuxOutputXptFromChannel (const NTV2Channel inChannel, const bool inLinkB, const bool inIsRGB)
{
    static const NTV2OutputXptID gMuxARGBOutputs[] = {	NTV2_Xpt425Mux1ARGB,	NTV2_Xpt425Mux2ARGB,	NTV2_Xpt425Mux3ARGB,	NTV2_Xpt425Mux4ARGB,
														NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack};
    static const NTV2OutputXptID gMuxAYUVOutputs[] = {	NTV2_Xpt425Mux1AYUV,	NTV2_Xpt425Mux2AYUV,	NTV2_Xpt425Mux3AYUV,	NTV2_Xpt425Mux4AYUV,
														NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack};
    static const NTV2OutputXptID gMuxBRGBOutputs[] = {	NTV2_Xpt425Mux1BRGB,	NTV2_Xpt425Mux2BRGB,	NTV2_Xpt425Mux3BRGB,	NTV2_Xpt425Mux4BRGB,
														NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack};
    static const NTV2OutputXptID gMuxBYUVOutputs[] = {	NTV2_Xpt425Mux1BYUV,	NTV2_Xpt425Mux2BYUV,	NTV2_Xpt425Mux3BYUV,	NTV2_Xpt425Mux4BYUV,
														NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack};
    if (NTV2_IS_VALID_CHANNEL(inChannel))
    {
        if (inLinkB)
            return inIsRGB ? gMuxBRGBOutputs[inChannel] : gMuxBYUVOutputs[inChannel];
        else
            return inIsRGB  ?  gMuxARGBOutputs[inChannel]  :  gMuxAYUVOutputs[inChannel];
    }
    else
        return NTV2_OUTPUT_CROSSPOINT_INVALID;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Crosspoint Utils	End


// static
bool CNTV2SignalRouter::GetRouteROMInfoFromReg (const ULWord inRegNum, const ULWord inRegVal,
							NTV2InputXptID & outInputXpt, NTV2OutputXptIDSet & outOutputXpts, const bool inAppendOutputXpts)
{
	static const ULWord firstROMReg(kRegFirstValidXptROMRegister);
	static const ULWord firstInpXpt(NTV2_FIRST_INPUT_CROSSPOINT);
	if (!inAppendOutputXpts)
		outOutputXpts.clear();
	outInputXpt = NTV2_INPUT_CROSSPOINT_INVALID;
	if (inRegNum < uint32_t(kRegFirstValidXptROMRegister))
		return false;
	if (inRegNum >= uint32_t(kRegInvalidValidXptROMRegister))
		return false;

	const ULWord regOffset(inRegNum - firstROMReg);
	const ULWord bitOffset((regOffset % 4) * 32);
	outInputXpt = NTV2InputXptID(firstInpXpt  +  regOffset / 4UL);	//	4 regs per inputXpt
	if (!inRegVal)
		return true;	//	No bits set

	RoutingExpertPtr pExpert(RoutingExpert::GetInstance());
	NTV2_ASSERT(pExpert);
	for (UWord bitNdx(0);  bitNdx < 32;  bitNdx++)
		if (inRegVal & ULWord(1UL << bitNdx))
		{
			const NTV2OutputXptID yuvOutputXpt (NTV2OutputXptID((bitOffset + bitNdx) & 0x0000007F));
			const NTV2OutputXptID rgbOutputXpt (NTV2OutputXptID(yuvOutputXpt | 0x80));
			if (pExpert  &&  pExpert->IsOutputXptValid(yuvOutputXpt))
				outOutputXpts.insert(yuvOutputXpt);
			if (pExpert  &&  pExpert->IsOutputXptValid(rgbOutputXpt))
				outOutputXpts.insert(rgbOutputXpt);
		}
	return true;
}

// static
bool CNTV2SignalRouter::GetPossibleConnections (const NTV2RegReads & inROMRegs, NTV2PossibleConnections & outConnections)
{
	outConnections.clear();
	for (NTV2RegReadsConstIter iter(inROMRegs.begin());  iter != inROMRegs.end();  ++iter)
	{
		if (iter->registerNumber < kRegFirstValidXptROMRegister  ||  iter->registerNumber >= kRegInvalidValidXptROMRegister)
			continue;	//	Skip -- not a ROM reg
		NTV2InputXptID inputXpt(NTV2_INPUT_CROSSPOINT_INVALID);
		NTV2OutputXptIDSet	outputXpts;
		if (GetRouteROMInfoFromReg (iter->registerNumber, iter->registerValue, inputXpt, outputXpts, true))
			for (NTV2OutputXptIDSetConstIter it(outputXpts.begin());  it != outputXpts.end();  ++it)
				outConnections.insert(NTV2Connection(inputXpt, *it));
	}
	return !outConnections.empty();
}

// static
bool CNTV2SignalRouter::MakeRouteROMRegisters (NTV2RegReads & outROMRegs)
{
	outROMRegs.clear();
	for (uint32_t regNum(kRegFirstValidXptROMRegister);  regNum < kRegInvalidValidXptROMRegister;  regNum++)
		outROMRegs.push_back(NTV2RegInfo(regNum));
	return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// NTV2RoutingEntry	Begin
#if !defined (NTV2_DEPRECATE_12_5)
	//////////	Per-widget input crosspoint selection register/mask/shift values
	#if defined (NTV2_DEPRECATE)
		//	The new way:	make the NTV2RoutingEntrys invisible to SDK clients (must use Getter functions):
		#define	AJA_LOCAL_STATIC	static
	#else	//	!defined (NTV2_DEPRECATE)
		//	The old way:	keep the NTV2RoutingEntrys visible to SDK clients (don't need to use Getter functions):
		#define	AJA_LOCAL_STATIC
	#endif	//	!defined (NTV2_DEPRECATE)


	AJA_LOCAL_STATIC	const NTV2RoutingEntry FrameBuffer1InputSelectEntry		=	{kRegXptSelectGroup2,		kK2RegMaskFrameBuffer1InputSelect,			kK2RegShiftFrameBuffer1InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry FrameBuffer1BInputSelectEntry	=	{kRegXptSelectGroup34,		kK2RegMaskFrameBuffer1BInputSelect,			kK2RegShiftFrameBuffer1BInputSelect,		0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry FrameBuffer2InputSelectEntry		=	{kRegXptSelectGroup5,		kK2RegMaskFrameBuffer2InputSelect,			kK2RegShiftFrameBuffer2InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry FrameBuffer2BInputSelectEntry	=	{kRegXptSelectGroup34,		kK2RegMaskFrameBuffer2BInputSelect,			kK2RegShiftFrameBuffer2BInputSelect,		0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry FrameBuffer3InputSelectEntry		=	{kRegXptSelectGroup13,		kK2RegMaskFrameBuffer3InputSelect,			kK2RegShiftFrameBuffer3InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry FrameBuffer3BInputSelectEntry	=	{kRegXptSelectGroup34,		kK2RegMaskFrameBuffer3BInputSelect,			kK2RegShiftFrameBuffer3BInputSelect,		0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry FrameBuffer4InputSelectEntry		=	{kRegXptSelectGroup13,		kK2RegMaskFrameBuffer4InputSelect,			kK2RegShiftFrameBuffer4InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry FrameBuffer4BInputSelectEntry	=	{kRegXptSelectGroup34,		kK2RegMaskFrameBuffer4BInputSelect,			kK2RegShiftFrameBuffer4BInputSelect,		0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry FrameBuffer5InputSelectEntry		=	{kRegXptSelectGroup21,		kK2RegMaskFrameBuffer5InputSelect,			kK2RegShiftFrameBuffer5InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry FrameBuffer5BInputSelectEntry	=	{kRegXptSelectGroup35,		kK2RegMaskFrameBuffer5BInputSelect,			kK2RegShiftFrameBuffer5BInputSelect,		0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry FrameBuffer6InputSelectEntry		=	{kRegXptSelectGroup21,		kK2RegMaskFrameBuffer6InputSelect,			kK2RegShiftFrameBuffer6InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry FrameBuffer6BInputSelectEntry	=	{kRegXptSelectGroup35,		kK2RegMaskFrameBuffer6BInputSelect,			kK2RegShiftFrameBuffer6BInputSelect,		0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry FrameBuffer7InputSelectEntry		=	{kRegXptSelectGroup21,		kK2RegMaskFrameBuffer7InputSelect,			kK2RegShiftFrameBuffer7InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry FrameBuffer7BInputSelectEntry	=	{kRegXptSelectGroup35,		kK2RegMaskFrameBuffer7BInputSelect,			kK2RegShiftFrameBuffer7BInputSelect,		0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry FrameBuffer8InputSelectEntry		=	{kRegXptSelectGroup21,		kK2RegMaskFrameBuffer8InputSelect,			kK2RegShiftFrameBuffer8InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry FrameBuffer8BInputSelectEntry	=	{kRegXptSelectGroup35,		kK2RegMaskFrameBuffer8BInputSelect,			kK2RegShiftFrameBuffer8BInputSelect,		0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry CSC1VidInputSelectEntry			=	{kRegXptSelectGroup1,		kK2RegMaskColorSpaceConverterInputSelect,	kK2RegShiftColorSpaceConverterInputSelect,	0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry CSC1KeyInputSelectEntry			=	{kRegXptSelectGroup3,		kK2RegMaskCSC1KeyInputSelect,				kK2RegShiftCSC1KeyInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry CSC2VidInputSelectEntry			=	{kRegXptSelectGroup5,		kK2RegMaskCSC2VidInputSelect,				kK2RegShiftCSC2VidInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry CSC2KeyInputSelectEntry			=	{kRegXptSelectGroup5,		kK2RegMaskCSC2KeyInputSelect,				kK2RegShiftCSC2KeyInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry CSC3VidInputSelectEntry			=	{kRegXptSelectGroup17,		kK2RegMaskCSC3VidInputSelect,				kK2RegShiftCSC3VidInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry CSC3KeyInputSelectEntry			=	{kRegXptSelectGroup17,		kK2RegMaskCSC3KeyInputSelect,				kK2RegShiftCSC3KeyInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry CSC4VidInputSelectEntry			=	{kRegXptSelectGroup17,		kK2RegMaskCSC4VidInputSelect,				kK2RegShiftCSC4VidInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry CSC4KeyInputSelectEntry			=	{kRegXptSelectGroup17,		kK2RegMaskCSC4KeyInputSelect,				kK2RegShiftCSC4KeyInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry CSC5VidInputSelectEntry			=	{kRegXptSelectGroup18,		kK2RegMaskCSC5VidInputSelect,				kK2RegShiftCSC5VidInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry CSC5KeyInputSelectEntry			=	{kRegXptSelectGroup18,		kK2RegMaskCSC5KeyInputSelect,				kK2RegShiftCSC5KeyInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry CSC6VidInputSelectEntry			=	{kRegXptSelectGroup30,		kK2RegMaskCSC6VidInputSelect,				kK2RegShiftCSC6VidInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry CSC6KeyInputSelectEntry			=	{kRegXptSelectGroup30,		kK2RegMaskCSC6KeyInputSelect,				kK2RegShiftCSC6KeyInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry CSC7VidInputSelectEntry			=	{kRegXptSelectGroup23,		kK2RegMaskCSC7VidInputSelect,				kK2RegShiftCSC7VidInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry CSC7KeyInputSelectEntry			=	{kRegXptSelectGroup23,      kK2RegMaskCSC7KeyInputSelect,				kK2RegShiftCSC7KeyInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry CSC8VidInputSelectEntry			=	{kRegXptSelectGroup23,      kK2RegMaskCSC8VidInputSelect,				kK2RegShiftCSC8VidInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry CSC8KeyInputSelectEntry			=	{kRegXptSelectGroup23,      kK2RegMaskCSC8KeyInputSelect,				kK2RegShiftCSC8KeyInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry LUT1InputSelectEntry				=	{kRegXptSelectGroup1,		kK2RegMaskXptLUTInputSelect,				kK2RegShiftXptLUTInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry LUT2InputSelectEntry				=	{kRegXptSelectGroup5,		kK2RegMaskXptLUT2InputSelect,				kK2RegShiftXptLUT2InputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry LUT3InputSelectEntry				=	{kRegXptSelectGroup12,		kK2RegMaskXptLUT3InputSelect,				kK2RegShiftXptLUT3InputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry LUT4InputSelectEntry				=	{kRegXptSelectGroup12,		kK2RegMaskXptLUT4InputSelect,				kK2RegShiftXptLUT4InputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry LUT5InputSelectEntry				=	{kRegXptSelectGroup12,		kK2RegMaskXptLUT5InputSelect,				kK2RegShiftXptLUT5InputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry LUT6InputSelectEntry				=	{kRegXptSelectGroup24,		kK2RegMaskXptLUT6InputSelect,				kK2RegShiftXptLUT6InputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry LUT7InputSelectEntry				=	{kRegXptSelectGroup24,		kK2RegMaskXptLUT7InputSelect,				kK2RegShiftXptLUT7InputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry LUT8InputSelectEntry				=	{kRegXptSelectGroup24,		kK2RegMaskXptLUT8InputSelect,				kK2RegShiftXptLUT8InputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry SDIOut1StandardSelectEntry		=	{kRegSDIOut1Control,    	kK2RegMaskSDIOutStandard,					kK2RegShiftSDIOutStandard,					0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry SDIOut2StandardSelectEntry		=	{kRegSDIOut2Control,    	kK2RegMaskSDIOutStandard,					kK2RegShiftSDIOutStandard,					0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry SDIOut3StandardSelectEntry		=	{kRegSDIOut3Control,    	kK2RegMaskSDIOutStandard,					kK2RegShiftSDIOutStandard,					0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry SDIOut4StandardSelectEntry		=	{kRegSDIOut4Control,    	kK2RegMaskSDIOutStandard,					kK2RegShiftSDIOutStandard,					0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry SDIOut1InputSelectEntry			=	{kRegXptSelectGroup3,		kK2RegMaskSDIOut1InputSelect,				kK2RegShiftSDIOut1InputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry SDIOut1InputDS2SelectEntry		=	{kRegXptSelectGroup10,		kK2RegMaskSDIOut1DS2InputSelect,			kK2RegShiftSDIOut1DS2InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry SDIOut2InputSelectEntry			=	{kRegXptSelectGroup3,		kK2RegMaskSDIOut2InputSelect,				kK2RegShiftSDIOut2InputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry SDIOut2InputDS2SelectEntry		=	{kRegXptSelectGroup10,		kK2RegMaskSDIOut2DS2InputSelect,			kK2RegShiftSDIOut2DS2InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry SDIOut3InputSelectEntry			=	{kRegXptSelectGroup8,		kK2RegMaskSDIOut3InputSelect,				kK2RegShiftSDIOut3InputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry SDIOut3InputDS2SelectEntry		=	{kRegXptSelectGroup14,		kK2RegMaskSDIOut3DS2InputSelect,			kK2RegShiftSDIOut3DS2InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry SDIOut4InputSelectEntry			=	{kRegXptSelectGroup8,		kK2RegMaskSDIOut4InputSelect,				kK2RegShiftSDIOut4InputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry SDIOut4InputDS2SelectEntry		=	{kRegXptSelectGroup14,		kK2RegMaskSDIOut4DS2InputSelect,			kK2RegShiftSDIOut4DS2InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry SDIOut5InputSelectEntry			=	{kRegXptSelectGroup8,		kK2RegMaskSDIOut5InputSelect,				kK2RegShiftSDIOut5InputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry SDIOut5InputDS2SelectEntry		=	{kRegXptSelectGroup14,		kK2RegMaskSDIOut5DS2InputSelect,			kK2RegShiftSDIOut5DS2InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry SDIOut6InputSelectEntry			=	{kRegXptSelectGroup22,		kK2RegMaskSDIOut6InputSelect,				kK2RegShiftSDIOut6InputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry SDIOut6InputDS2SelectEntry		=	{kRegXptSelectGroup22,		kK2RegMaskSDIOut6DS2InputSelect,			kK2RegShiftSDIOut6DS2InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry SDIOut7InputSelectEntry			=	{kRegXptSelectGroup22,		kK2RegMaskSDIOut7InputSelect,				kK2RegShiftSDIOut7InputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry SDIOut7InputDS2SelectEntry		=	{kRegXptSelectGroup22,		kK2RegMaskSDIOut7DS2InputSelect,			kK2RegShiftSDIOut7DS2InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry SDIOut8InputSelectEntry			=	{kRegXptSelectGroup30,		kK2RegMaskSDIOut8InputSelect,				kK2RegShiftSDIOut8InputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry SDIOut8InputDS2SelectEntry		=	{kRegXptSelectGroup30,		kK2RegMaskSDIOut8DS2InputSelect,			kK2RegShiftSDIOut8DS2InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkIn1InputSelectEntry		=	{kRegXptSelectGroup11,		kK2RegMaskDuallinkIn1InputSelect,			kK2RegShiftDuallinkIn1InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkIn1DSInputSelectEntry	=	{kRegXptSelectGroup11,		kK2RegMaskDuallinkIn1DSInputSelect,			kK2RegShiftDuallinkIn1DSInputSelect,		0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkIn2InputSelectEntry		=	{kRegXptSelectGroup11,		kK2RegMaskDuallinkIn2InputSelect,			kK2RegShiftDuallinkIn2InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkIn2DSInputSelectEntry	=	{kRegXptSelectGroup11,		kK2RegMaskDuallinkIn2DSInputSelect,			kK2RegShiftDuallinkIn2DSInputSelect,		0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkIn3InputSelectEntry		=	{kRegXptSelectGroup15,		kK2RegMaskDuallinkIn3InputSelect,			kK2RegShiftDuallinkIn3InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkIn3DSInputSelectEntry	=	{kRegXptSelectGroup15,		kK2RegMaskDuallinkIn3DSInputSelect,			kK2RegShiftDuallinkIn3DSInputSelect,		0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkIn4InputSelectEntry		=	{kRegXptSelectGroup15,		kK2RegMaskDuallinkIn4InputSelect,			kK2RegShiftDuallinkIn4InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkIn4DSInputSelectEntry	=	{kRegXptSelectGroup15,		kK2RegMaskDuallinkIn4DSInputSelect,			kK2RegShiftDuallinkIn4DSInputSelect,		0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkIn5InputSelectEntry		=	{kRegXptSelectGroup25,		kK2RegMaskDuallinkIn5InputSelect,			kK2RegShiftDuallinkIn5InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkIn5DSInputSelectEntry	=	{kRegXptSelectGroup25,		kK2RegMaskDuallinkIn5DSInputSelect,			kK2RegShiftDuallinkIn5DSInputSelect,		0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkIn6InputSelectEntry		=	{kRegXptSelectGroup25,		kK2RegMaskDuallinkIn6InputSelect,			kK2RegShiftDuallinkIn6InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkIn6DSInputSelectEntry	=	{kRegXptSelectGroup25,		kK2RegMaskDuallinkIn6DSInputSelect,			kK2RegShiftDuallinkIn6DSInputSelect,		0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkIn7InputSelectEntry		=	{kRegXptSelectGroup26,		kK2RegMaskDuallinkIn7InputSelect,			kK2RegShiftDuallinkIn7InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkIn7DSInputSelectEntry	=	{kRegXptSelectGroup26,		kK2RegMaskDuallinkIn7DSInputSelect,			kK2RegShiftDuallinkIn7DSInputSelect,		0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkIn8InputSelectEntry		=	{kRegXptSelectGroup26,		kK2RegMaskDuallinkIn8InputSelect,			kK2RegShiftDuallinkIn8InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkIn8DSInputSelectEntry	=	{kRegXptSelectGroup26,		kK2RegMaskDuallinkIn8DSInputSelect,			kK2RegShiftDuallinkIn8DSInputSelect,		0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkOut1InputSelectEntry		=	{kRegXptSelectGroup2,		kK2RegMaskDuallinkOutInputSelect,			kK2RegShiftDuallinkOutInputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkOut2InputSelectEntry		=	{kRegXptSelectGroup7,		kK2RegMaskDuallinkOut2InputSelect,			kK2RegShiftDuallinkOut2InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkOut3InputSelectEntry		=	{kRegXptSelectGroup16,		kK2RegMaskDuallinkOut3InputSelect,			kK2RegShiftDuallinkOut3InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkOut4InputSelectEntry		=	{kRegXptSelectGroup16,		kK2RegMaskDuallinkOut4InputSelect,			kK2RegShiftDuallinkOut4InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkOut5InputSelectEntry		=	{kRegXptSelectGroup16,		kK2RegMaskDuallinkOut5InputSelect,			kK2RegShiftDuallinkOut5InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkOut6InputSelectEntry		=	{kRegXptSelectGroup27,		kK2RegMaskDuallinkOut6InputSelect,			kK2RegShiftDuallinkOut6InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkOut7InputSelectEntry		=	{kRegXptSelectGroup27,		kK2RegMaskDuallinkOut7InputSelect,			kK2RegShiftDuallinkOut7InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry DualLinkOut8InputSelectEntry		=	{kRegXptSelectGroup27,		kK2RegMaskDuallinkOut8InputSelect,			kK2RegShiftDuallinkOut8InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Mixer1BGKeyInputSelectEntry		=	{kRegXptSelectGroup4,		kK2RegMaskMixerBGKeyInputSelect,			kK2RegShiftMixerBGKeyInputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Mixer1BGVidInputSelectEntry		=	{kRegXptSelectGroup4,		kK2RegMaskMixerBGVidInputSelect,			kK2RegShiftMixerBGVidInputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Mixer1FGKeyInputSelectEntry		=	{kRegXptSelectGroup4,		kK2RegMaskMixerFGKeyInputSelect,			kK2RegShiftMixerFGKeyInputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Mixer1FGVidInputSelectEntry		=	{kRegXptSelectGroup4,		kK2RegMaskMixerFGVidInputSelect,			kK2RegShiftMixerFGVidInputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Mixer2BGKeyInputSelectEntry		=	{kRegXptSelectGroup9,		kK2RegMaskMixer2BGKeyInputSelect,			kK2RegShiftMixer2BGKeyInputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Mixer2BGVidInputSelectEntry		=	{kRegXptSelectGroup9,		kK2RegMaskMixer2BGVidInputSelect,			kK2RegShiftMixer2BGVidInputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Mixer2FGKeyInputSelectEntry		=	{kRegXptSelectGroup9,		kK2RegMaskMixer2FGKeyInputSelect,			kK2RegShiftMixer2FGKeyInputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Mixer2FGVidInputSelectEntry		=	{kRegXptSelectGroup9,		kK2RegMaskMixer2FGVidInputSelect,			kK2RegShiftMixer2FGVidInputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Mixer3BGKeyInputSelectEntry		=	{kRegXptSelectGroup28,		kK2RegMaskMixer3BGKeyInputSelect,			kK2RegShiftMixer3BGKeyInputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Mixer3BGVidInputSelectEntry		=	{kRegXptSelectGroup28,		kK2RegMaskMixer3BGVidInputSelect,			kK2RegShiftMixer3BGVidInputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Mixer3FGKeyInputSelectEntry		=	{kRegXptSelectGroup28,		kK2RegMaskMixer3FGKeyInputSelect,			kK2RegShiftMixer3FGKeyInputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Mixer3FGVidInputSelectEntry		=	{kRegXptSelectGroup28,		kK2RegMaskMixer3FGVidInputSelect,			kK2RegShiftMixer3FGVidInputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Mixer4BGKeyInputSelectEntry		=	{kRegXptSelectGroup29,		kK2RegMaskMixer4BGKeyInputSelect,			kK2RegShiftMixer4BGKeyInputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Mixer4BGVidInputSelectEntry		=	{kRegXptSelectGroup29,		kK2RegMaskMixer4BGVidInputSelect,			kK2RegShiftMixer4BGVidInputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Mixer4FGKeyInputSelectEntry		=	{kRegXptSelectGroup29,		kK2RegMaskMixer4FGKeyInputSelect,			kK2RegShiftMixer4FGKeyInputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Mixer4FGVidInputSelectEntry		=	{kRegXptSelectGroup29,		kK2RegMaskMixer4FGVidInputSelect,			kK2RegShiftMixer4FGVidInputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry HDMIOutInputSelectEntry			=	{kRegXptSelectGroup6,		kK2RegMaskHDMIOutInputSelect,				kK2RegShiftHDMIOutInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry HDMIOutQ1InputSelectEntry		=	HDMIOutInputSelectEntry;
	AJA_LOCAL_STATIC	const NTV2RoutingEntry HDMIOutQ2InputSelectEntry		=	{kRegXptSelectGroup20,		kK2RegMaskHDMIOutV2Q2InputSelect,			kK2RegShiftHDMIOutV2Q2InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry HDMIOutQ3InputSelectEntry		=	{kRegXptSelectGroup20,		kK2RegMaskHDMIOutV2Q3InputSelect,			kK2RegShiftHDMIOutV2Q3InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry HDMIOutQ4InputSelectEntry		=	{kRegXptSelectGroup20,		kK2RegMaskHDMIOutV2Q4InputSelect,			kK2RegShiftHDMIOutV2Q4InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Xpt4KDCQ1InputSelectEntry		=	{kRegXptSelectGroup19,		kK2RegMask4KDCQ1InputSelect,				kK2RegShift4KDCQ1InputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Xpt4KDCQ2InputSelectEntry		=	{kRegXptSelectGroup19,		kK2RegMask4KDCQ2InputSelect,				kK2RegShift4KDCQ2InputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Xpt4KDCQ3InputSelectEntry		=	{kRegXptSelectGroup19,		kK2RegMask4KDCQ3InputSelect,				kK2RegShift4KDCQ3InputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Xpt4KDCQ4InputSelectEntry		=	{kRegXptSelectGroup19,		kK2RegMask4KDCQ4InputSelect,				kK2RegShift4KDCQ4InputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Xpt425Mux1AInputSelectEntry		=	{kRegXptSelectGroup32,		kK2RegMask425Mux1AInputSelect,				kK2RegShift425Mux1AInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Xpt425Mux1BInputSelectEntry		=	{kRegXptSelectGroup32,		kK2RegMask425Mux1BInputSelect,				kK2RegShift425Mux1BInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Xpt425Mux2AInputSelectEntry		=	{kRegXptSelectGroup32,		kK2RegMask425Mux2AInputSelect,				kK2RegShift425Mux2AInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Xpt425Mux2BInputSelectEntry		=	{kRegXptSelectGroup32,		kK2RegMask425Mux2BInputSelect,				kK2RegShift425Mux2BInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Xpt425Mux3AInputSelectEntry		=	{kRegXptSelectGroup33,		kK2RegMask425Mux3AInputSelect,				kK2RegShift425Mux3AInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Xpt425Mux3BInputSelectEntry		=	{kRegXptSelectGroup33,		kK2RegMask425Mux3BInputSelect,				kK2RegShift425Mux3BInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Xpt425Mux4AInputSelectEntry		=	{kRegXptSelectGroup33,		kK2RegMask425Mux4AInputSelect,				kK2RegShift425Mux4AInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry Xpt425Mux4BInputSelectEntry		=	{kRegXptSelectGroup33,		kK2RegMask425Mux4BInputSelect,				kK2RegShift425Mux4BInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry CompressionModInputSelectEntry	=	{kRegXptSelectGroup1,		kK2RegMaskCompressionModInputSelect,		kK2RegShiftCompressionModInputSelect,		0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry ConversionModInputSelectEntry	=	{kRegXptSelectGroup1,		kK2RegMaskConversionModInputSelect,			kK2RegShiftConversionModInputSelect,		0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry ConversionMod2InputSelectEntry	=	{kRegXptSelectGroup6,		kK2RegMaskSecondConverterInputSelect,		kK2RegShiftSecondConverterInputSelect,		0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry CSC1KeyFromInput2SelectEntry		=	{kRegCSCoefficients1_2,		kK2RegMaskMakeAlphaFromKeySelect,			kK2RegShiftMakeAlphaFromKeySelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry FrameSync2InputSelectEntry		=	{kRegXptSelectGroup2,		kK2RegMaskFrameSync2InputSelect,			kK2RegShiftFrameSync2InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry FrameSync1InputSelectEntry		=	{kRegXptSelectGroup2,		kK2RegMaskFrameSync1InputSelect,			kK2RegShiftFrameSync1InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry AnalogOutInputSelectEntry		=	{kRegXptSelectGroup3,		kK2RegMaskAnalogOutInputSelect,				kK2RegShiftAnalogOutInputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry AnalogOutCompositeOutSelectEntry	=	{kRegFS1ReferenceSelect,	kFS1RegMaskSecondAnalogOutInputSelect,		kFS1RegShiftSecondAnalogOutInputSelect,		0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry ProAmpInputSelectEntry			=	{kRegFS1ReferenceSelect,	kFS1RegMaskProcAmpInputSelect,				kFS1RegShiftProcAmpInputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry IICT1InputSelectEntry			=	{kRegXptSelectGroup6,		kK2RegMaskIICTInputSelect,					kK2RegShiftIICTInputSelect,					0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry IICT2InputSelectEntry			=	{kRegXptSelectGroup7,		kK2RegMaskIICT2InputSelect,					kK2RegShiftIICT2InputSelect,				0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry WaterMarker1InputSelectEntry		=	{kRegXptSelectGroup6,		kK2RegMaskWaterMarkerInputSelect,			kK2RegShiftWaterMarkerInputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry WaterMarker2InputSelectEntry		=	{kRegXptSelectGroup7,		kK2RegMaskWaterMarker2InputSelect,			kK2RegShiftWaterMarker2InputSelect,			0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry StereoLeftInputSelectEntry		=	{kRegStereoCompressor,		kRegMaskStereoCompressorLeftSource,			kRegShiftStereoCompressorLeftSource,		0};
	AJA_LOCAL_STATIC	const NTV2RoutingEntry StereoRightInputSelectEntry		=	{kRegStereoCompressor,		kRegMaskStereoCompressorRightSource,		kRegShiftStereoCompressorRightSource,		0};

	//	Make this one a read-only register in case of accident...
	AJA_LOCAL_STATIC	const NTV2RoutingEntry UpdateRegisterSelectEntry		=	{kRegStatus,				0xFFFFFFFF,									0,											0};


		const NTV2RoutingEntry &	GetFrameBuffer1InputSelectEntry		(void)	{return FrameBuffer1InputSelectEntry;}
		const NTV2RoutingEntry &	GetFrameBuffer1BInputSelectEntry	(void)	{return FrameBuffer1BInputSelectEntry;}
		const NTV2RoutingEntry &	GetFrameBuffer2InputSelectEntry		(void)	{return FrameBuffer2InputSelectEntry;}
		const NTV2RoutingEntry &	GetFrameBuffer2BInputSelectEntry	(void)	{return FrameBuffer2BInputSelectEntry;}
		const NTV2RoutingEntry &	GetFrameBuffer3InputSelectEntry		(void)	{return FrameBuffer3InputSelectEntry;}
		const NTV2RoutingEntry &	GetFrameBuffer3BInputSelectEntry	(void)	{return FrameBuffer3BInputSelectEntry;}
		const NTV2RoutingEntry &	GetFrameBuffer4InputSelectEntry		(void)	{return FrameBuffer4InputSelectEntry;}
		const NTV2RoutingEntry &	GetFrameBuffer4BInputSelectEntry	(void)	{return FrameBuffer4BInputSelectEntry;}
		const NTV2RoutingEntry &	GetFrameBuffer5InputSelectEntry		(void)	{return FrameBuffer5InputSelectEntry;}
		const NTV2RoutingEntry &	GetFrameBuffer5BInputSelectEntry	(void)	{return FrameBuffer5BInputSelectEntry;}
		const NTV2RoutingEntry &	GetFrameBuffer6InputSelectEntry		(void)	{return FrameBuffer6InputSelectEntry;}
		const NTV2RoutingEntry &	GetFrameBuffer6BInputSelectEntry	(void)	{return FrameBuffer6BInputSelectEntry;}
		const NTV2RoutingEntry &	GetFrameBuffer7InputSelectEntry		(void)	{return FrameBuffer7InputSelectEntry;}
		const NTV2RoutingEntry &	GetFrameBuffer7BInputSelectEntry	(void)	{return FrameBuffer7BInputSelectEntry;}
		const NTV2RoutingEntry &	GetFrameBuffer8InputSelectEntry		(void)	{return FrameBuffer8InputSelectEntry;}
		const NTV2RoutingEntry &	GetFrameBuffer8BInputSelectEntry	(void)	{return FrameBuffer8BInputSelectEntry;}
		const NTV2RoutingEntry &	GetCSC1VidInputSelectEntry			(void)	{return CSC1VidInputSelectEntry;}
		const NTV2RoutingEntry &	GetCSC1KeyInputSelectEntry			(void)	{return CSC1KeyInputSelectEntry;}
		const NTV2RoutingEntry &	GetCSC2VidInputSelectEntry			(void)	{return CSC2VidInputSelectEntry;}
		const NTV2RoutingEntry &	GetCSC2KeyInputSelectEntry			(void)	{return CSC2KeyInputSelectEntry;}
		const NTV2RoutingEntry &	GetCSC3VidInputSelectEntry			(void)	{return CSC3VidInputSelectEntry;}
		const NTV2RoutingEntry &	GetCSC3KeyInputSelectEntry			(void)	{return CSC3KeyInputSelectEntry;}
		const NTV2RoutingEntry &	GetCSC4VidInputSelectEntry			(void)	{return CSC4VidInputSelectEntry;}
		const NTV2RoutingEntry &	GetCSC4KeyInputSelectEntry			(void)	{return CSC4KeyInputSelectEntry;}
		const NTV2RoutingEntry &	GetCSC5VidInputSelectEntry			(void)	{return CSC5VidInputSelectEntry;}
		const NTV2RoutingEntry &	GetCSC5KeyInputSelectEntry			(void)	{return CSC5KeyInputSelectEntry;}
		const NTV2RoutingEntry &	GetCSC6VidInputSelectEntry			(void)	{return CSC6VidInputSelectEntry;}
		const NTV2RoutingEntry &	GetCSC6KeyInputSelectEntry			(void)	{return CSC6KeyInputSelectEntry;}
		const NTV2RoutingEntry &	GetCSC7VidInputSelectEntry			(void)	{return CSC7VidInputSelectEntry;}
		const NTV2RoutingEntry &	GetCSC7KeyInputSelectEntry			(void)	{return CSC7KeyInputSelectEntry;}
		const NTV2RoutingEntry &	GetCSC8VidInputSelectEntry			(void)	{return CSC8VidInputSelectEntry;}
		const NTV2RoutingEntry &	GetCSC8KeyInputSelectEntry			(void)	{return CSC8KeyInputSelectEntry;}
		const NTV2RoutingEntry &	GetLUT1InputSelectEntry				(void)	{return LUT1InputSelectEntry;}
		const NTV2RoutingEntry &	GetLUT2InputSelectEntry				(void)	{return LUT2InputSelectEntry;}
		const NTV2RoutingEntry &	GetLUT3InputSelectEntry				(void)	{return LUT3InputSelectEntry;}
		const NTV2RoutingEntry &	GetLUT4InputSelectEntry				(void)	{return LUT4InputSelectEntry;}
		const NTV2RoutingEntry &	GetLUT5InputSelectEntry				(void)	{return LUT5InputSelectEntry;}
		const NTV2RoutingEntry &	GetLUT6InputSelectEntry				(void)	{return LUT6InputSelectEntry;}
		const NTV2RoutingEntry &	GetLUT7InputSelectEntry				(void)	{return LUT7InputSelectEntry;}
		const NTV2RoutingEntry &	GetLUT8InputSelectEntry				(void)	{return LUT8InputSelectEntry;}
		const NTV2RoutingEntry &	GetSDIOut1StandardSelectEntry		(void)	{return SDIOut1StandardSelectEntry;}
		const NTV2RoutingEntry &	GetSDIOut2StandardSelectEntry		(void)	{return SDIOut2StandardSelectEntry;}
		const NTV2RoutingEntry &	GetSDIOut3StandardSelectEntry		(void)	{return SDIOut3StandardSelectEntry;}
		const NTV2RoutingEntry &	GetSDIOut4StandardSelectEntry		(void)	{return SDIOut4StandardSelectEntry;}
		const NTV2RoutingEntry &	GetSDIOut1InputSelectEntry			(void)	{return SDIOut1InputSelectEntry;}
		const NTV2RoutingEntry &	GetSDIOut1InputDS2SelectEntry		(void)	{return SDIOut1InputDS2SelectEntry;}
		const NTV2RoutingEntry &	GetSDIOut2InputSelectEntry			(void)	{return SDIOut2InputSelectEntry;}
		const NTV2RoutingEntry &	GetSDIOut2InputDS2SelectEntry		(void)	{return SDIOut2InputDS2SelectEntry;}
		const NTV2RoutingEntry &	GetSDIOut3InputSelectEntry			(void)	{return SDIOut3InputSelectEntry;}
		const NTV2RoutingEntry &	GetSDIOut3InputDS2SelectEntry		(void)	{return SDIOut3InputDS2SelectEntry;}
		const NTV2RoutingEntry &	GetSDIOut4InputSelectEntry			(void)	{return SDIOut4InputSelectEntry;}
		const NTV2RoutingEntry &	GetSDIOut4InputDS2SelectEntry		(void)	{return SDIOut4InputDS2SelectEntry;}
		const NTV2RoutingEntry &	GetSDIOut5InputSelectEntry			(void)	{return SDIOut5InputSelectEntry;}
		const NTV2RoutingEntry &	GetSDIOut5InputDS2SelectEntry		(void)	{return SDIOut5InputDS2SelectEntry;}
		const NTV2RoutingEntry &	GetSDIOut6InputSelectEntry			(void)	{return SDIOut6InputSelectEntry;}
		const NTV2RoutingEntry &	GetSDIOut6InputDS2SelectEntry		(void)	{return SDIOut6InputDS2SelectEntry;}
		const NTV2RoutingEntry &	GetSDIOut7InputSelectEntry			(void)	{return SDIOut7InputSelectEntry;}
		const NTV2RoutingEntry &	GetSDIOut7InputDS2SelectEntry		(void)	{return SDIOut7InputDS2SelectEntry;}
		const NTV2RoutingEntry &	GetSDIOut8InputSelectEntry			(void)	{return SDIOut8InputSelectEntry;}
		const NTV2RoutingEntry &	GetSDIOut8InputDS2SelectEntry		(void)	{return SDIOut8InputDS2SelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkIn1InputSelectEntry		(void)	{return DualLinkIn1InputSelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkIn1DSInputSelectEntry	(void)	{return DualLinkIn1DSInputSelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkIn2InputSelectEntry		(void)	{return DualLinkIn2InputSelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkIn2DSInputSelectEntry	(void)	{return DualLinkIn2DSInputSelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkIn3InputSelectEntry		(void)	{return DualLinkIn3InputSelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkIn3DSInputSelectEntry	(void)	{return DualLinkIn3DSInputSelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkIn4InputSelectEntry		(void)	{return DualLinkIn4InputSelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkIn4DSInputSelectEntry	(void)	{return DualLinkIn4DSInputSelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkIn5InputSelectEntry		(void)	{return DualLinkIn5InputSelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkIn5DSInputSelectEntry	(void)	{return DualLinkIn5DSInputSelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkIn6InputSelectEntry		(void)	{return DualLinkIn6InputSelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkIn6DSInputSelectEntry	(void)	{return DualLinkIn6DSInputSelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkIn7InputSelectEntry		(void)	{return DualLinkIn7InputSelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkIn7DSInputSelectEntry	(void)	{return DualLinkIn7DSInputSelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkIn8InputSelectEntry		(void)	{return DualLinkIn8InputSelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkIn8DSInputSelectEntry	(void)	{return DualLinkIn8DSInputSelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkOut1InputSelectEntry		(void)	{return DualLinkOut1InputSelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkOut2InputSelectEntry		(void)	{return DualLinkOut2InputSelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkOut3InputSelectEntry		(void)	{return DualLinkOut3InputSelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkOut4InputSelectEntry		(void)	{return DualLinkOut4InputSelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkOut5InputSelectEntry		(void)	{return DualLinkOut5InputSelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkOut6InputSelectEntry		(void)	{return DualLinkOut6InputSelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkOut7InputSelectEntry		(void)	{return DualLinkOut7InputSelectEntry;}
		const NTV2RoutingEntry &	GetDualLinkOut8InputSelectEntry		(void)	{return DualLinkOut8InputSelectEntry;}
		const NTV2RoutingEntry &	GetMixer1BGKeyInputSelectEntry		(void)	{return Mixer1BGKeyInputSelectEntry;}
		const NTV2RoutingEntry &	GetMixer1BGVidInputSelectEntry		(void)	{return Mixer1BGVidInputSelectEntry;}
		const NTV2RoutingEntry &	GetMixer1FGKeyInputSelectEntry		(void)	{return Mixer1FGKeyInputSelectEntry;}
		const NTV2RoutingEntry &	GetMixer1FGVidInputSelectEntry		(void)	{return Mixer1FGVidInputSelectEntry;}
		const NTV2RoutingEntry &	GetMixer2BGKeyInputSelectEntry		(void)	{return Mixer2BGKeyInputSelectEntry;}
		const NTV2RoutingEntry &	GetMixer2BGVidInputSelectEntry		(void)	{return Mixer2BGVidInputSelectEntry;}
		const NTV2RoutingEntry &	GetMixer2FGKeyInputSelectEntry		(void)	{return Mixer2FGKeyInputSelectEntry;}
		const NTV2RoutingEntry &	GetMixer2FGVidInputSelectEntry		(void)	{return Mixer2FGVidInputSelectEntry;}
		const NTV2RoutingEntry &	GetMixer3BGKeyInputSelectEntry		(void)	{return Mixer3BGKeyInputSelectEntry;}
		const NTV2RoutingEntry &	GetMixer3BGVidInputSelectEntry		(void)	{return Mixer3BGVidInputSelectEntry;}
		const NTV2RoutingEntry &	GetMixer3FGKeyInputSelectEntry		(void)	{return Mixer3FGKeyInputSelectEntry;}
		const NTV2RoutingEntry &	GetMixer3FGVidInputSelectEntry		(void)	{return Mixer3FGVidInputSelectEntry;}
		const NTV2RoutingEntry &	GetMixer4BGKeyInputSelectEntry		(void)	{return Mixer4BGKeyInputSelectEntry;}
		const NTV2RoutingEntry &	GetMixer4BGVidInputSelectEntry		(void)	{return Mixer4BGVidInputSelectEntry;}
		const NTV2RoutingEntry &	GetMixer4FGKeyInputSelectEntry		(void)	{return Mixer4FGKeyInputSelectEntry;}
		const NTV2RoutingEntry &	GetMixer4FGVidInputSelectEntry		(void)	{return Mixer4FGVidInputSelectEntry;}
		const NTV2RoutingEntry &	GetXpt4KDCQ1InputSelectEntry		(void)	{return Xpt4KDCQ1InputSelectEntry;}
		const NTV2RoutingEntry &	GetXpt4KDCQ2InputSelectEntry		(void)	{return Xpt4KDCQ2InputSelectEntry;}
		const NTV2RoutingEntry &	GetXpt4KDCQ3InputSelectEntry		(void)	{return Xpt4KDCQ3InputSelectEntry;}
		const NTV2RoutingEntry &	GetXpt4KDCQ4InputSelectEntry		(void)	{return Xpt4KDCQ4InputSelectEntry;}
		const NTV2RoutingEntry &	GetHDMIOutInputSelectEntry			(void)	{return HDMIOutInputSelectEntry;}
		const NTV2RoutingEntry &	GetHDMIOutQ1InputSelectEntry		(void)	{return HDMIOutQ1InputSelectEntry;}
		const NTV2RoutingEntry &	GetHDMIOutQ2InputSelectEntry		(void)	{return HDMIOutQ2InputSelectEntry;}
		const NTV2RoutingEntry &	GetHDMIOutQ3InputSelectEntry		(void)	{return HDMIOutQ3InputSelectEntry;}
		const NTV2RoutingEntry &	GetHDMIOutQ4InputSelectEntry		(void)	{return HDMIOutQ4InputSelectEntry;}

		const NTV2RoutingEntry &	Get425Mux1AInputSelectEntry			(void)	{return Xpt425Mux1AInputSelectEntry;}
		const NTV2RoutingEntry &	Get425Mux1BInputSelectEntry			(void)	{return Xpt425Mux1BInputSelectEntry;}
		const NTV2RoutingEntry &	Get425Mux2AInputSelectEntry			(void)	{return Xpt425Mux2AInputSelectEntry;}
		const NTV2RoutingEntry &	Get425Mux2BInputSelectEntry			(void)	{return Xpt425Mux2BInputSelectEntry;}
		const NTV2RoutingEntry &	Get425Mux3AInputSelectEntry			(void)	{return Xpt425Mux3AInputSelectEntry;}
		const NTV2RoutingEntry &	Get425Mux3BInputSelectEntry			(void)	{return Xpt425Mux3BInputSelectEntry;}
		const NTV2RoutingEntry &	Get425Mux4AInputSelectEntry			(void)	{return Xpt425Mux4AInputSelectEntry;}
		const NTV2RoutingEntry &	Get425Mux4BInputSelectEntry			(void)	{return Xpt425Mux4BInputSelectEntry;}

		const NTV2RoutingEntry &	GetCompressionModInputSelectEntry	(void)	{return CompressionModInputSelectEntry;}
		const NTV2RoutingEntry &	GetConversionModInputSelectEntry	(void)	{return ConversionModInputSelectEntry;}
		const NTV2RoutingEntry &	GetCSC1KeyFromInput2SelectEntry		(void)	{return CSC1KeyFromInput2SelectEntry;}
		const NTV2RoutingEntry &	GetFrameSync2InputSelectEntry		(void)	{return FrameSync2InputSelectEntry;}
		const NTV2RoutingEntry &	GetFrameSync1InputSelectEntry		(void)	{return FrameSync1InputSelectEntry;}
		const NTV2RoutingEntry &	GetAnalogOutInputSelectEntry		(void)	{return AnalogOutInputSelectEntry;}
		const NTV2RoutingEntry &	GetProAmpInputSelectEntry			(void)	{return ProAmpInputSelectEntry;}
		const NTV2RoutingEntry &	GetIICT1InputSelectEntry			(void)	{return IICT1InputSelectEntry;}
		const NTV2RoutingEntry &	GetWaterMarker1InputSelectEntry		(void)	{return WaterMarker1InputSelectEntry;}
		const NTV2RoutingEntry &	GetWaterMarker2InputSelectEntry		(void)	{return WaterMarker2InputSelectEntry;}
		const NTV2RoutingEntry &	GetUpdateRegisterSelectEntry		(void)	{return UpdateRegisterSelectEntry;}
		const NTV2RoutingEntry &	GetConversionMod2InputSelectEntry	(void)	{return ConversionMod2InputSelectEntry;}
		const NTV2RoutingEntry &	GetIICT2InputSelectEntry			(void)	{return IICT2InputSelectEntry;}
		const NTV2RoutingEntry &	GetAnalogOutCompositeOutSelectEntry	(void)	{return AnalogOutCompositeOutSelectEntry;}
		const NTV2RoutingEntry &	GetStereoLeftInputSelectEntry		(void)	{return StereoLeftInputSelectEntry;}
		const NTV2RoutingEntry &	GetStereoRightInputSelectEntry		(void)	{return StereoRightInputSelectEntry;}

	#define cmp_router_entries(a, b) (a.registerNum == b.registerNum && a.mask == b.mask && a.shift == b.shift && a.value == b.value) ? true : false

	NTV2InputCrosspointID CNTV2SignalRouter::NTV2RoutingEntryToInputCrosspointID (const NTV2RoutingEntry & inEntry)
	{
		if (cmp_router_entries(inEntry, FrameBuffer1InputSelectEntry))		return NTV2_XptFrameBuffer1Input;
		if (cmp_router_entries(inEntry, FrameBuffer1BInputSelectEntry))		return NTV2_XptFrameBuffer1BInput;
		if (cmp_router_entries(inEntry, FrameBuffer2InputSelectEntry))		return NTV2_XptFrameBuffer2Input;
		if (cmp_router_entries(inEntry, FrameBuffer2BInputSelectEntry))		return NTV2_XptFrameBuffer2BInput;
		if (cmp_router_entries(inEntry, FrameBuffer3InputSelectEntry))		return NTV2_XptFrameBuffer3Input;
		if (cmp_router_entries(inEntry, FrameBuffer3BInputSelectEntry))		return NTV2_XptFrameBuffer3BInput;
		if (cmp_router_entries(inEntry, FrameBuffer4InputSelectEntry))		return NTV2_XptFrameBuffer4Input;
		if (cmp_router_entries(inEntry, FrameBuffer4BInputSelectEntry))		return NTV2_XptFrameBuffer4BInput;
		if (cmp_router_entries(inEntry, FrameBuffer5InputSelectEntry))		return NTV2_XptFrameBuffer5Input;
		if (cmp_router_entries(inEntry, FrameBuffer5BInputSelectEntry))		return NTV2_XptFrameBuffer5BInput;
		if (cmp_router_entries(inEntry, FrameBuffer6InputSelectEntry))		return NTV2_XptFrameBuffer6Input;
		if (cmp_router_entries(inEntry, FrameBuffer6BInputSelectEntry))		return NTV2_XptFrameBuffer6BInput;
		if (cmp_router_entries(inEntry, FrameBuffer7InputSelectEntry))		return NTV2_XptFrameBuffer7Input;
		if (cmp_router_entries(inEntry, FrameBuffer7BInputSelectEntry))		return NTV2_XptFrameBuffer7BInput;
		if (cmp_router_entries(inEntry, FrameBuffer8InputSelectEntry))		return NTV2_XptFrameBuffer8Input;
		if (cmp_router_entries(inEntry, FrameBuffer8BInputSelectEntry))		return NTV2_XptFrameBuffer8BInput;
		if (cmp_router_entries(inEntry, CSC1VidInputSelectEntry))			return NTV2_XptCSC1VidInput;
		if (cmp_router_entries(inEntry, CSC1KeyInputSelectEntry))			return NTV2_XptCSC1KeyInput;
		if (cmp_router_entries(inEntry, CSC2VidInputSelectEntry))			return NTV2_XptCSC2VidInput;
		if (cmp_router_entries(inEntry, CSC2KeyInputSelectEntry))			return NTV2_XptCSC2KeyInput;
		if (cmp_router_entries(inEntry, CSC3VidInputSelectEntry))			return NTV2_XptCSC3VidInput;
		if (cmp_router_entries(inEntry, CSC3KeyInputSelectEntry))			return NTV2_XptCSC3KeyInput;
		if (cmp_router_entries(inEntry, CSC4VidInputSelectEntry))			return NTV2_XptCSC4VidInput;
		if (cmp_router_entries(inEntry, CSC4KeyInputSelectEntry))			return NTV2_XptCSC4KeyInput;
		if (cmp_router_entries(inEntry, CSC5VidInputSelectEntry))			return NTV2_XptCSC5VidInput;
		if (cmp_router_entries(inEntry, CSC5KeyInputSelectEntry))			return NTV2_XptCSC5KeyInput;
		if (cmp_router_entries(inEntry, CSC6VidInputSelectEntry))			return NTV2_XptCSC6VidInput;
		if (cmp_router_entries(inEntry, CSC6KeyInputSelectEntry))			return NTV2_XptCSC6KeyInput;
		if (cmp_router_entries(inEntry, CSC7VidInputSelectEntry))			return NTV2_XptCSC7VidInput;
		if (cmp_router_entries(inEntry, CSC7KeyInputSelectEntry))			return NTV2_XptCSC7KeyInput;
		if (cmp_router_entries(inEntry, CSC8VidInputSelectEntry))			return NTV2_XptCSC8VidInput;
		if (cmp_router_entries(inEntry, CSC8KeyInputSelectEntry))			return NTV2_XptCSC8KeyInput;
		if (cmp_router_entries(inEntry, LUT1InputSelectEntry))				return NTV2_XptLUT1Input;
		if (cmp_router_entries(inEntry, LUT2InputSelectEntry))				return NTV2_XptLUT2Input;
		if (cmp_router_entries(inEntry, LUT3InputSelectEntry))				return NTV2_XptLUT3Input;
		if (cmp_router_entries(inEntry, LUT4InputSelectEntry))				return NTV2_XptLUT4Input;
		if (cmp_router_entries(inEntry, LUT5InputSelectEntry))				return NTV2_XptLUT5Input;
		if (cmp_router_entries(inEntry, LUT6InputSelectEntry))				return NTV2_XptLUT6Input;
		if (cmp_router_entries(inEntry, LUT7InputSelectEntry))				return NTV2_XptLUT7Input;
		if (cmp_router_entries(inEntry, LUT8InputSelectEntry))				return NTV2_XptLUT8Input;
		if (cmp_router_entries(inEntry, SDIOut1StandardSelectEntry))		return NTV2_XptSDIOut1Standard;
		if (cmp_router_entries(inEntry, SDIOut2StandardSelectEntry))		return NTV2_XptSDIOut2Standard;
		if (cmp_router_entries(inEntry, SDIOut3StandardSelectEntry))		return NTV2_XptSDIOut3Standard;
		if (cmp_router_entries(inEntry, SDIOut4StandardSelectEntry))		return NTV2_XptSDIOut4Standard;
		if (cmp_router_entries(inEntry, SDIOut1InputSelectEntry))			return NTV2_XptSDIOut1Input;
		if (cmp_router_entries(inEntry, SDIOut1InputDS2SelectEntry))		return NTV2_XptSDIOut1InputDS2;
		if (cmp_router_entries(inEntry, SDIOut2InputSelectEntry))			return NTV2_XptSDIOut2Input;
		if (cmp_router_entries(inEntry, SDIOut2InputDS2SelectEntry))		return NTV2_XptSDIOut2InputDS2;
		if (cmp_router_entries(inEntry, SDIOut3InputSelectEntry))			return NTV2_XptSDIOut3Input;
		if (cmp_router_entries(inEntry, SDIOut3InputDS2SelectEntry))		return NTV2_XptSDIOut3InputDS2;
		if (cmp_router_entries(inEntry, SDIOut4InputSelectEntry))			return NTV2_XptSDIOut4Input;
		if (cmp_router_entries(inEntry, SDIOut4InputDS2SelectEntry))		return NTV2_XptSDIOut4InputDS2;
		if (cmp_router_entries(inEntry, SDIOut5InputSelectEntry))			return NTV2_XptSDIOut5Input;
		if (cmp_router_entries(inEntry, SDIOut5InputDS2SelectEntry))		return NTV2_XptSDIOut5InputDS2;
		if (cmp_router_entries(inEntry, SDIOut6InputSelectEntry))			return NTV2_XptSDIOut6Input;
		if (cmp_router_entries(inEntry, SDIOut6InputDS2SelectEntry))		return NTV2_XptSDIOut6InputDS2;
		if (cmp_router_entries(inEntry, SDIOut7InputSelectEntry))			return NTV2_XptSDIOut7Input;
		if (cmp_router_entries(inEntry, SDIOut7InputDS2SelectEntry))		return NTV2_XptSDIOut7InputDS2;
		if (cmp_router_entries(inEntry, SDIOut8InputSelectEntry))			return NTV2_XptSDIOut8Input;
		if (cmp_router_entries(inEntry, SDIOut8InputDS2SelectEntry))		return NTV2_XptSDIOut8InputDS2;
		if (cmp_router_entries(inEntry, DualLinkIn1InputSelectEntry))		return NTV2_XptDualLinkIn1Input;
		if (cmp_router_entries(inEntry, DualLinkIn1DSInputSelectEntry))		return NTV2_XptDualLinkIn1DSInput;
		if (cmp_router_entries(inEntry, DualLinkIn2InputSelectEntry))		return NTV2_XptDualLinkIn2Input;
		if (cmp_router_entries(inEntry, DualLinkIn2DSInputSelectEntry))		return NTV2_XptDualLinkIn2DSInput;
		if (cmp_router_entries(inEntry, DualLinkIn3InputSelectEntry))		return NTV2_XptDualLinkIn3Input;
		if (cmp_router_entries(inEntry, DualLinkIn3DSInputSelectEntry))		return NTV2_XptDualLinkIn3DSInput;
		if (cmp_router_entries(inEntry, DualLinkIn4InputSelectEntry))		return NTV2_XptDualLinkIn4Input;
		if (cmp_router_entries(inEntry, DualLinkIn4DSInputSelectEntry))		return NTV2_XptDualLinkIn4DSInput;
		if (cmp_router_entries(inEntry, DualLinkIn5InputSelectEntry))		return NTV2_XptDualLinkIn5Input;
		if (cmp_router_entries(inEntry, DualLinkIn5DSInputSelectEntry))		return NTV2_XptDualLinkIn5DSInput;
		if (cmp_router_entries(inEntry, DualLinkIn6InputSelectEntry))		return NTV2_XptDualLinkIn6Input;
		if (cmp_router_entries(inEntry, DualLinkIn6DSInputSelectEntry))		return NTV2_XptDualLinkIn6DSInput;
		if (cmp_router_entries(inEntry, DualLinkIn7InputSelectEntry))		return NTV2_XptDualLinkIn7Input;
		if (cmp_router_entries(inEntry, DualLinkIn7DSInputSelectEntry))		return NTV2_XptDualLinkIn7DSInput;
		if (cmp_router_entries(inEntry, DualLinkIn8InputSelectEntry))		return NTV2_XptDualLinkIn8Input;
		if (cmp_router_entries(inEntry, DualLinkIn8DSInputSelectEntry))		return NTV2_XptDualLinkIn8DSInput;
		if (cmp_router_entries(inEntry, DualLinkOut1InputSelectEntry))		return NTV2_XptDualLinkOut1Input;
		if (cmp_router_entries(inEntry, DualLinkOut2InputSelectEntry))		return NTV2_XptDualLinkOut2Input;
		if (cmp_router_entries(inEntry, DualLinkOut3InputSelectEntry))		return NTV2_XptDualLinkOut3Input;
		if (cmp_router_entries(inEntry, DualLinkOut4InputSelectEntry))		return NTV2_XptDualLinkOut4Input;
		if (cmp_router_entries(inEntry, DualLinkOut5InputSelectEntry))		return NTV2_XptDualLinkOut5Input;
		if (cmp_router_entries(inEntry, DualLinkOut6InputSelectEntry))		return NTV2_XptDualLinkOut6Input;
		if (cmp_router_entries(inEntry, DualLinkOut7InputSelectEntry))		return NTV2_XptDualLinkOut7Input;
		if (cmp_router_entries(inEntry, DualLinkOut8InputSelectEntry))		return NTV2_XptDualLinkOut8Input;
		if (cmp_router_entries(inEntry, Mixer1BGKeyInputSelectEntry))		return NTV2_XptMixer1BGKeyInput;
		if (cmp_router_entries(inEntry, Mixer1BGVidInputSelectEntry))		return NTV2_XptMixer1BGVidInput;
		if (cmp_router_entries(inEntry, Mixer1FGKeyInputSelectEntry))		return NTV2_XptMixer1FGKeyInput;
		if (cmp_router_entries(inEntry, Mixer1FGVidInputSelectEntry))		return NTV2_XptMixer1FGVidInput;
		if (cmp_router_entries(inEntry, Mixer2BGKeyInputSelectEntry))		return NTV2_XptMixer2BGKeyInput;
		if (cmp_router_entries(inEntry, Mixer2BGVidInputSelectEntry))		return NTV2_XptMixer2BGVidInput;
		if (cmp_router_entries(inEntry, Mixer2FGKeyInputSelectEntry))		return NTV2_XptMixer2FGKeyInput;
		if (cmp_router_entries(inEntry, Mixer2FGVidInputSelectEntry))		return NTV2_XptMixer2FGVidInput;
		if (cmp_router_entries(inEntry, Mixer3BGKeyInputSelectEntry))		return NTV2_XptMixer3BGKeyInput;
		if (cmp_router_entries(inEntry, Mixer3BGVidInputSelectEntry))		return NTV2_XptMixer3BGVidInput;
		if (cmp_router_entries(inEntry, Mixer3FGKeyInputSelectEntry))		return NTV2_XptMixer3FGKeyInput;
		if (cmp_router_entries(inEntry, Mixer3FGVidInputSelectEntry))		return NTV2_XptMixer3FGVidInput;
		if (cmp_router_entries(inEntry, Mixer4BGKeyInputSelectEntry))		return NTV2_XptMixer4BGKeyInput;
		if (cmp_router_entries(inEntry, Mixer4BGVidInputSelectEntry))		return NTV2_XptMixer4BGVidInput;
		if (cmp_router_entries(inEntry, Mixer4FGKeyInputSelectEntry))		return NTV2_XptMixer4FGKeyInput;
		if (cmp_router_entries(inEntry, Mixer4FGVidInputSelectEntry))		return NTV2_XptMixer4FGVidInput;
		if (cmp_router_entries(inEntry, HDMIOutInputSelectEntry))			return NTV2_XptHDMIOutInput;
		if (cmp_router_entries(inEntry, HDMIOutQ1InputSelectEntry))			return NTV2_XptHDMIOutQ1Input;
		if (cmp_router_entries(inEntry, HDMIOutQ2InputSelectEntry))			return NTV2_XptHDMIOutQ2Input;
		if (cmp_router_entries(inEntry, HDMIOutQ3InputSelectEntry))			return NTV2_XptHDMIOutQ3Input;
		if (cmp_router_entries(inEntry, HDMIOutQ4InputSelectEntry))			return NTV2_XptHDMIOutQ4Input;
		if (cmp_router_entries(inEntry, Xpt4KDCQ1InputSelectEntry))			return NTV2_Xpt4KDCQ1Input;
		if (cmp_router_entries(inEntry, Xpt4KDCQ2InputSelectEntry))			return NTV2_Xpt4KDCQ2Input;
		if (cmp_router_entries(inEntry, Xpt4KDCQ3InputSelectEntry))			return NTV2_Xpt4KDCQ3Input;
		if (cmp_router_entries(inEntry, Xpt4KDCQ4InputSelectEntry))			return NTV2_Xpt4KDCQ4Input;
		if (cmp_router_entries(inEntry, Xpt425Mux1AInputSelectEntry))		return NTV2_Xpt425Mux1AInput;
		if (cmp_router_entries(inEntry, Xpt425Mux1BInputSelectEntry))		return NTV2_Xpt425Mux1BInput;
		if (cmp_router_entries(inEntry, Xpt425Mux2AInputSelectEntry))		return NTV2_Xpt425Mux2AInput;
		if (cmp_router_entries(inEntry, Xpt425Mux2BInputSelectEntry))		return NTV2_Xpt425Mux2BInput;
		if (cmp_router_entries(inEntry, Xpt425Mux3AInputSelectEntry))		return NTV2_Xpt425Mux3AInput;
		if (cmp_router_entries(inEntry, Xpt425Mux3BInputSelectEntry))		return NTV2_Xpt425Mux3BInput;
		if (cmp_router_entries(inEntry, Xpt425Mux4AInputSelectEntry))		return NTV2_Xpt425Mux4AInput;
		if (cmp_router_entries(inEntry, Xpt425Mux4BInputSelectEntry))		return NTV2_Xpt425Mux4BInput;
		if (cmp_router_entries(inEntry, CompressionModInputSelectEntry))	return NTV2_XptCompressionModInput;
		if (cmp_router_entries(inEntry, ConversionModInputSelectEntry))		return NTV2_XptConversionModInput;
		if (cmp_router_entries(inEntry, ConversionMod2InputSelectEntry))	return NTV2_XptConversionMod2Input;
		if (cmp_router_entries(inEntry, CSC1KeyFromInput2SelectEntry))		return NTV2_XptCSC1KeyFromInput2;
		if (cmp_router_entries(inEntry, FrameSync2InputSelectEntry))		return NTV2_XptFrameSync2Input;
		if (cmp_router_entries(inEntry, FrameSync1InputSelectEntry))		return NTV2_XptFrameSync1Input;
		if (cmp_router_entries(inEntry, AnalogOutInputSelectEntry))			return NTV2_XptAnalogOutInput;
		if (cmp_router_entries(inEntry, AnalogOutCompositeOutSelectEntry))	return NTV2_XptAnalogOutCompositeOut;
		if (cmp_router_entries(inEntry, ProAmpInputSelectEntry))			return NTV2_XptProAmpInput;
		if (cmp_router_entries(inEntry, IICT1InputSelectEntry))				return NTV2_XptIICT1Input;
		if (cmp_router_entries(inEntry, IICT2InputSelectEntry))				return NTV2_XptIICT2Input;
		if (cmp_router_entries(inEntry, WaterMarker1InputSelectEntry))		return NTV2_XptWaterMarker1Input;
		if (cmp_router_entries(inEntry, WaterMarker2InputSelectEntry))		return NTV2_XptWaterMarker2Input;
		if (cmp_router_entries(inEntry, StereoLeftInputSelectEntry))		return NTV2_XptStereoLeftInput;
		if (cmp_router_entries(inEntry, StereoRightInputSelectEntry))		return NTV2_XptStereoRightInput;

		return NTV2_INPUT_CROSSPOINT_INVALID;
	}	//	NTV2RoutingEntryToInputCrosspointID


	const NTV2RoutingEntry & CNTV2SignalRouter::GetInputSelectEntry (const NTV2InputCrosspointID inInputXpt)
	{
		static NTV2RoutingEntry	NULLInputSelectEntry	= {0, 0, 0, 0};

		switch (inInputXpt)
		{
			case NTV2_XptFrameBuffer1Input:		return FrameBuffer1InputSelectEntry;
			case NTV2_XptFrameBuffer1BInput:	return FrameBuffer1BInputSelectEntry;
			case NTV2_XptFrameBuffer2Input:		return FrameBuffer2InputSelectEntry;
			case NTV2_XptFrameBuffer2BInput:	return FrameBuffer2BInputSelectEntry;
			case NTV2_XptFrameBuffer3Input:		return FrameBuffer3InputSelectEntry;
			case NTV2_XptFrameBuffer3BInput:	return FrameBuffer3BInputSelectEntry;
			case NTV2_XptFrameBuffer4Input:		return FrameBuffer4InputSelectEntry;
			case NTV2_XptFrameBuffer4BInput:	return FrameBuffer4BInputSelectEntry;
			case NTV2_XptFrameBuffer5Input:		return FrameBuffer5InputSelectEntry;
			case NTV2_XptFrameBuffer5BInput:	return FrameBuffer5BInputSelectEntry;
			case NTV2_XptFrameBuffer6Input:		return FrameBuffer6InputSelectEntry;
			case NTV2_XptFrameBuffer6BInput:	return FrameBuffer6BInputSelectEntry;
			case NTV2_XptFrameBuffer7Input:		return FrameBuffer7InputSelectEntry;
			case NTV2_XptFrameBuffer7BInput:	return FrameBuffer7BInputSelectEntry;
			case NTV2_XptFrameBuffer8Input:		return FrameBuffer8InputSelectEntry;
			case NTV2_XptFrameBuffer8BInput:	return FrameBuffer8BInputSelectEntry;
			case NTV2_XptCSC1VidInput:			return CSC1VidInputSelectEntry;
			case NTV2_XptCSC1KeyInput:			return CSC1KeyInputSelectEntry;
			case NTV2_XptCSC2VidInput:			return CSC2VidInputSelectEntry;
			case NTV2_XptCSC2KeyInput:			return CSC2KeyInputSelectEntry;
			case NTV2_XptCSC3VidInput:			return CSC3VidInputSelectEntry;
			case NTV2_XptCSC3KeyInput:			return CSC3KeyInputSelectEntry;
			case NTV2_XptCSC4VidInput:			return CSC4VidInputSelectEntry;
			case NTV2_XptCSC4KeyInput:			return CSC4KeyInputSelectEntry;
			case NTV2_XptCSC5VidInput:			return CSC5VidInputSelectEntry;
			case NTV2_XptCSC5KeyInput:			return CSC5KeyInputSelectEntry;
			case NTV2_XptCSC6VidInput:			return CSC6VidInputSelectEntry;
			case NTV2_XptCSC6KeyInput:			return CSC6KeyInputSelectEntry;
			case NTV2_XptCSC7VidInput:			return CSC7VidInputSelectEntry;
			case NTV2_XptCSC7KeyInput:			return CSC7KeyInputSelectEntry;
			case NTV2_XptCSC8VidInput:			return CSC8VidInputSelectEntry;
			case NTV2_XptCSC8KeyInput:			return CSC8KeyInputSelectEntry;
			case NTV2_XptLUT1Input:				return LUT1InputSelectEntry;
			case NTV2_XptLUT2Input:				return LUT2InputSelectEntry;
			case NTV2_XptLUT3Input:				return LUT3InputSelectEntry;
			case NTV2_XptLUT4Input:				return LUT4InputSelectEntry;
			case NTV2_XptLUT5Input:				return LUT5InputSelectEntry;
			case NTV2_XptLUT6Input:				return LUT6InputSelectEntry;
			case NTV2_XptLUT7Input:				return LUT7InputSelectEntry;
			case NTV2_XptLUT8Input:				return LUT8InputSelectEntry;
			case NTV2_XptSDIOut1Standard:		return NULLInputSelectEntry;
			case NTV2_XptSDIOut2Standard:		return NULLInputSelectEntry;
			case NTV2_XptSDIOut3Standard:		return NULLInputSelectEntry;
			case NTV2_XptSDIOut4Standard:		return NULLInputSelectEntry;
			case NTV2_XptSDIOut1Input:			return SDIOut1InputSelectEntry;
			case NTV2_XptSDIOut1InputDS2:		return SDIOut1InputDS2SelectEntry;
			case NTV2_XptSDIOut2Input:			return SDIOut2InputSelectEntry;
			case NTV2_XptSDIOut2InputDS2:		return SDIOut2InputDS2SelectEntry;
			case NTV2_XptSDIOut3Input:			return SDIOut3InputSelectEntry;
			case NTV2_XptSDIOut3InputDS2:		return SDIOut3InputDS2SelectEntry;
			case NTV2_XptSDIOut4Input:			return SDIOut4InputSelectEntry;
			case NTV2_XptSDIOut4InputDS2:		return SDIOut4InputDS2SelectEntry;
			case NTV2_XptSDIOut5Input:			return SDIOut5InputSelectEntry;
			case NTV2_XptSDIOut5InputDS2:		return SDIOut5InputDS2SelectEntry;
			case NTV2_XptSDIOut6Input:			return SDIOut6InputSelectEntry;
			case NTV2_XptSDIOut6InputDS2:		return SDIOut6InputDS2SelectEntry;
			case NTV2_XptSDIOut7Input:			return SDIOut7InputSelectEntry;
			case NTV2_XptSDIOut7InputDS2:		return SDIOut7InputDS2SelectEntry;
			case NTV2_XptSDIOut8Input:			return SDIOut8InputSelectEntry;
			case NTV2_XptSDIOut8InputDS2:		return SDIOut8InputDS2SelectEntry;
			case NTV2_XptDualLinkIn1Input:		return DualLinkIn1InputSelectEntry;
			case NTV2_XptDualLinkIn1DSInput:	return DualLinkIn1DSInputSelectEntry;
			case NTV2_XptDualLinkIn2Input:		return DualLinkIn2InputSelectEntry;
			case NTV2_XptDualLinkIn2DSInput:	return DualLinkIn2DSInputSelectEntry;
			case NTV2_XptDualLinkIn3Input:		return DualLinkIn3InputSelectEntry;
			case NTV2_XptDualLinkIn3DSInput:	return DualLinkIn3DSInputSelectEntry;
			case NTV2_XptDualLinkIn4Input:		return DualLinkIn4InputSelectEntry;
			case NTV2_XptDualLinkIn4DSInput:	return DualLinkIn4DSInputSelectEntry;
			case NTV2_XptDualLinkIn5Input:		return DualLinkIn5InputSelectEntry;
			case NTV2_XptDualLinkIn5DSInput:	return DualLinkIn5DSInputSelectEntry;
			case NTV2_XptDualLinkIn6Input:		return DualLinkIn6InputSelectEntry;
			case NTV2_XptDualLinkIn6DSInput:	return DualLinkIn6DSInputSelectEntry;
			case NTV2_XptDualLinkIn7Input:		return DualLinkIn7InputSelectEntry;
			case NTV2_XptDualLinkIn7DSInput:	return DualLinkIn7DSInputSelectEntry;
			case NTV2_XptDualLinkIn8Input:		return DualLinkIn8InputSelectEntry;
			case NTV2_XptDualLinkIn8DSInput:	return DualLinkIn8DSInputSelectEntry;
			case NTV2_XptDualLinkOut1Input:		return DualLinkOut1InputSelectEntry;
			case NTV2_XptDualLinkOut2Input:		return DualLinkOut2InputSelectEntry;
			case NTV2_XptDualLinkOut3Input:		return DualLinkOut3InputSelectEntry;
			case NTV2_XptDualLinkOut4Input:		return DualLinkOut4InputSelectEntry;
			case NTV2_XptDualLinkOut5Input:		return DualLinkOut5InputSelectEntry;
			case NTV2_XptDualLinkOut6Input:		return DualLinkOut6InputSelectEntry;
			case NTV2_XptDualLinkOut7Input:		return DualLinkOut7InputSelectEntry;
			case NTV2_XptDualLinkOut8Input:		return DualLinkOut8InputSelectEntry;
			case NTV2_XptMixer1BGKeyInput:		return Mixer1BGKeyInputSelectEntry;
			case NTV2_XptMixer1BGVidInput:		return Mixer1BGVidInputSelectEntry;
			case NTV2_XptMixer1FGKeyInput:		return Mixer1FGKeyInputSelectEntry;
			case NTV2_XptMixer1FGVidInput:		return Mixer1FGVidInputSelectEntry;
			case NTV2_XptMixer2BGKeyInput:		return Mixer2BGKeyInputSelectEntry;
			case NTV2_XptMixer2BGVidInput:		return Mixer2BGVidInputSelectEntry;
			case NTV2_XptMixer2FGKeyInput:		return Mixer2FGKeyInputSelectEntry;
			case NTV2_XptMixer2FGVidInput:		return Mixer2FGVidInputSelectEntry;
			case NTV2_XptMixer3BGKeyInput:		return Mixer3BGKeyInputSelectEntry;
			case NTV2_XptMixer3BGVidInput:		return Mixer3BGVidInputSelectEntry;
			case NTV2_XptMixer3FGKeyInput:		return Mixer3FGKeyInputSelectEntry;
			case NTV2_XptMixer3FGVidInput:		return Mixer3FGVidInputSelectEntry;
			case NTV2_XptMixer4BGKeyInput:		return Mixer4BGKeyInputSelectEntry;
			case NTV2_XptMixer4BGVidInput:		return Mixer4BGVidInputSelectEntry;
			case NTV2_XptMixer4FGKeyInput:		return Mixer4FGKeyInputSelectEntry;
			case NTV2_XptMixer4FGVidInput:		return Mixer4FGVidInputSelectEntry;
			case NTV2_XptHDMIOutInput:			return HDMIOutInputSelectEntry;
	// DUPE case NTV2_XptHDMIOutQ1Input:		return HDMIOutQ1InputSelectEntry;
			case NTV2_XptHDMIOutQ2Input:		return HDMIOutQ2InputSelectEntry;
			case NTV2_XptHDMIOutQ3Input:		return HDMIOutQ3InputSelectEntry;
			case NTV2_XptHDMIOutQ4Input:		return HDMIOutQ4InputSelectEntry;
			case NTV2_Xpt4KDCQ1Input:			return Xpt4KDCQ1InputSelectEntry;
			case NTV2_Xpt4KDCQ2Input:			return Xpt4KDCQ2InputSelectEntry;
			case NTV2_Xpt4KDCQ3Input:			return Xpt4KDCQ3InputSelectEntry;
			case NTV2_Xpt4KDCQ4Input:			return Xpt4KDCQ4InputSelectEntry;
			case NTV2_Xpt425Mux1AInput:			return Xpt425Mux1AInputSelectEntry;
			case NTV2_Xpt425Mux1BInput:			return Xpt425Mux1BInputSelectEntry;
			case NTV2_Xpt425Mux2AInput:			return Xpt425Mux2AInputSelectEntry;
			case NTV2_Xpt425Mux2BInput:			return Xpt425Mux2BInputSelectEntry;
			case NTV2_Xpt425Mux3AInput:			return Xpt425Mux3AInputSelectEntry;
			case NTV2_Xpt425Mux3BInput:			return Xpt425Mux3BInputSelectEntry;
			case NTV2_Xpt425Mux4AInput:			return Xpt425Mux4AInputSelectEntry;
			case NTV2_Xpt425Mux4BInput:			return Xpt425Mux4BInputSelectEntry;
			case NTV2_XptAnalogOutInput:		return AnalogOutInputSelectEntry;
			case NTV2_XptIICT2Input:			return IICT2InputSelectEntry;
			case NTV2_XptAnalogOutCompositeOut:	return AnalogOutCompositeOutSelectEntry;
			case NTV2_XptStereoLeftInput:		return StereoLeftInputSelectEntry;
			case NTV2_XptStereoRightInput:		return StereoRightInputSelectEntry;
			case NTV2_XptProAmpInput:			return ProAmpInputSelectEntry;
			case NTV2_XptIICT1Input:			return IICT1InputSelectEntry;
			case NTV2_XptWaterMarker1Input:		return WaterMarker1InputSelectEntry;
			case NTV2_XptWaterMarker2Input:		return WaterMarker2InputSelectEntry;
			case NTV2_XptUpdateRegister:		return UpdateRegisterSelectEntry;
			case NTV2_XptConversionMod2Input:	return ConversionMod2InputSelectEntry;
			case NTV2_XptCompressionModInput:	return CompressionModInputSelectEntry;
			case NTV2_XptConversionModInput:	return ConversionModInputSelectEntry;
			case NTV2_XptCSC1KeyFromInput2:		return CSC1KeyFromInput2SelectEntry;
			case NTV2_XptFrameSync2Input:		return FrameSync2InputSelectEntry;
			case NTV2_XptFrameSync1Input:		return FrameSync1InputSelectEntry;
			case NTV2_INPUT_CROSSPOINT_INVALID:	return NULLInputSelectEntry;
		}
		return NULLInputSelectEntry;
	}	//	GetInputSelectEntry
#endif	//	!defined (NTV2_DEPRECATE_12_5)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// NTV2RoutingEntry	End


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Stream Operators	Begin

ostream & operator << (ostream & inOutStream, const CNTV2SignalRouter & inObj)
{
    inObj.Print (inOutStream);
    return inOutStream;
}


ostream & operator << (ostream & inOutStream, const NTV2OutputXptIDSet & inObj)
{
    NTV2OutputXptIDSetConstIter	iter(inObj.begin());
    while (iter != inObj.end())
    {
        inOutStream << ::NTV2OutputCrosspointIDToString(*iter, false);
        ++iter;
        if (iter == inObj.end())
            break;
        inOutStream << ", ";
    }
    return inOutStream;
}

ostream & operator << (ostream & inOutStream, const NTV2InputXptIDSet & inObj)
{
    NTV2InputXptIDSetConstIter	iter(inObj.begin());
    while (iter != inObj.end())
    {
        inOutStream << ::NTV2InputCrosspointIDToString(*iter, false);
        ++iter;
        if (iter == inObj.end())
            break;
        inOutStream << ", ";
    }
    return inOutStream;
}

NTV2RoutingEntry & NTV2RoutingEntry::operator = (const NTV2RegInfo & inRHS)
{
    registerNum	= inRHS.registerNumber;
    mask		= inRHS.registerMask;
    shift		= inRHS.registerShift;
    value		= inRHS.registerValue;
    return *this;
}

ostream & operator << (ostream & inOutStream, const NTV2WidgetIDSet & inObj)
{
	for (NTV2WidgetIDSetConstIter iter (inObj.begin ());  iter != inObj.end ();  )
	{
		inOutStream << ::NTV2WidgetIDToString (*iter, true);
		if (++iter != inObj.end ())
			inOutStream << ",";
	}
	return inOutStream;
}

ostream & operator << (ostream & inOutStream, const NTV2XptConnections & inObj)
{
	for (NTV2XptConnectionsConstIter it(inObj.begin());  it != inObj.end();  )
	{
		inOutStream << ::NTV2InputCrosspointIDToString(it->first) << "-" << ::NTV2OutputCrosspointIDToString(it->second);
		if (++it != inObj.end())
			inOutStream << ", ";
	}
	return inOutStream;
}

#if !defined (NTV2_DEPRECATE_12_5)
    ostream & operator << (ostream & inOutStream, const NTV2RoutingEntry & inObj)
    {
        return inOutStream << ntv2RegStrings [inObj.registerNum] << " (" << inObj.registerNum << ") mask=0x" << inObj.mask << " shift=" << inObj.shift << " value=0x" << inObj.value;
    }

    ostream & operator << (ostream & inOutStream, const NTV2RoutingTable & inObj)
    {
        for (ULWord num (0);  num < inObj.numEntries;  num++)
            inOutStream << num << ":  " << inObj.routingEntry [num] << endl;
        return inOutStream;
    }
#endif	//	!defined (NTV2_DEPRECATE_12_5)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Stream Operators	End
