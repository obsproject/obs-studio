/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2routingexpert.cpp
	@brief		RoutingExpert implementation used within CNTV2SignalRouter.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/

#include "ntv2routingexpert.h"

// Logging helpers
#define	HEX16(__x__)		"0x" << std::hex << std::setw(16) << std::setfill('0') << uint64_t(__x__)  << std::dec
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

static uint32_t						gInstanceTally(0);
static uint32_t						gLivingInstances(0);

RoutingExpertPtr RoutingExpert::GetInstance(const bool inCreateIfNecessary)
{
	AJAAutoLock		locker(&gRoutingExpertLock);
	if (!gpRoutingExpert  &&  inCreateIfNecessary)
		gpRoutingExpert = new RoutingExpert;
	return gpRoutingExpert;
}

bool RoutingExpert::DisposeInstance(void)
{
	AJAAutoLock		locker(&gRoutingExpertLock);
	if (!gpRoutingExpert)
		return false;
	gpRoutingExpert = AJA_NULL;
	return true;
}

uint32_t RoutingExpert::NumInstances(void)
{
	return gLivingInstances;
}

//#define	DUMP_WIDGETID_TO_INPUT_XPTS_MMAP
RoutingExpert::RoutingExpert()
{
	InitInputXpt2String();
	InitOutputXpt2String();
	InitInputXpt2WidgetIDs();
	InitOutputXpt2WidgetIDs();
	InitWidgetIDToChannels();
	InitWidgetIDToWidgetTypes();
	AJAAtomic::Increment(&gInstanceTally);
	AJAAtomic::Increment(&gLivingInstances);
	SRiNOTE(DEC(gLivingInstances) << " extant, " << DEC(gInstanceTally) << " total");
	#if defined(DUMP_WIDGETID_TO_INPUT_XPTS_MMAP)
		for (Widget2InputXptsConstIter it(gWidget2InputXpts.begin());  it != gWidget2InputXpts.end();  ++it)
			SRiDBG(::NTV2WidgetIDToString(it->first,false) << " ('" << ::NTV2WidgetIDToString(it->first,true)
					<< "', " << DEC(it->first) << ") => " << ::NTV2InputCrosspointIDToString(it->second,false)
					<< " (" << ::NTV2InputCrosspointIDToString(it->second,true) << ", " << DEC(it->second) << ")");
	#endif	//	defined(DUMP_WIDGETID_TO_INPUT_XPTS_MMAP)
}

RoutingExpert::~RoutingExpert()
{
	AJAAtomic::Decrement(&gLivingInstances);
	SRiNOTE(DEC(gLivingInstances) << " extant, " << DEC(gInstanceTally) << " total");
}

std::string				RoutingExpert::InputXptToString (const NTV2InputXptID inInputXpt) const
{
	AJAAutoLock	lock(&gLock);
	NTV2_ASSERT(!gInputXpt2String.empty());
	InputXpt2StringConstIter iter(gInputXpt2String.find(inInputXpt));
	return iter != gInputXpt2String.end() ? iter->second : std::string();
}

std::string				RoutingExpert::OutputXptToString (const NTV2OutputXptID inOutputXpt) const
{
	AJAAutoLock	lock(&gLock);
	NTV2_ASSERT(!gOutputXpt2String.empty());
	OutputXpt2StringConstIter	iter(gOutputXpt2String.find(inOutputXpt));
	return iter != gOutputXpt2String.end() ? iter->second : std::string();
}

NTV2InputXptID		RoutingExpert::StringToInputXpt (const std::string & inStr) const
{
	AJAAutoLock	lock(&gLock);
	NTV2_ASSERT(!gString2InputXpt.empty ());
	std::string	str(inStr);	aja::lower(aja::strip(str));
	String2InputXptConstIter iter(gString2InputXpt.find(str));
	return iter != gString2InputXpt.end() ? iter->second : NTV2_INPUT_CROSSPOINT_INVALID;
}

NTV2OutputXptID		RoutingExpert::StringToOutputXpt (const std::string & inStr) const
{
	AJAAutoLock	lock(&gLock);
	NTV2_ASSERT(!gString2OutputXpt.empty());
	std::string	str(inStr);	aja::lower(aja::strip(str));
	String2OutputXptConstIter	iter(gString2OutputXpt.find(str));
	return iter != gString2OutputXpt.end() ? iter->second : NTV2_XptBlack;
}

NTV2WidgetType				RoutingExpert::WidgetIDToType (const NTV2WidgetID inWidgetID)
{
	AJAAutoLock lock(&gLock);
	NTV2_ASSERT(!gWidget2Types.empty());
	for (Widget2TypesConstIter iter = gWidget2Types.begin(); iter != gWidget2Types.end(); iter++) {
		if (iter->first == inWidgetID) {
			return iter->second;
		}
	}
	return NTV2WidgetType_Invalid;
}

NTV2Channel			RoutingExpert::WidgetIDToChannel(const NTV2WidgetID inWidgetID)
{
	AJAAutoLock lock(&gLock);
	NTV2_ASSERT(!gWidget2Channels.empty());
	for (Widget2ChannelsConstIter iter = gWidget2Channels.begin(); iter != gWidget2Channels.end(); iter++) {
		if (iter->first == inWidgetID)
			return iter->second;
	}
	return NTV2_CHANNEL_INVALID;
}

NTV2WidgetID		RoutingExpert::WidgetIDFromTypeAndChannel(const NTV2WidgetType inWidgetType, const NTV2Channel inChannel)
{
	AJAAutoLock lock(&gLock);
	NTV2_ASSERT(!gWidget2Types.empty());
	for (Widget2TypesConstIter iter = gWidget2Types.begin(); iter != gWidget2Types.end(); iter++) {
		if (iter->second == inWidgetType && WidgetIDToChannel(iter->first) == inChannel)
			return iter->first;
	}
	return NTV2_WIDGET_INVALID;
}

bool				RoutingExpert::GetWidgetsForOutput (const NTV2OutputXptID inOutputXpt, NTV2WidgetIDSet & outWidgetIDs) const
{
	AJAAutoLock	lock(&gLock);
	NTV2_ASSERT(!gOutputXpt2WidgetIDs.empty());
	outWidgetIDs.clear();
	for (OutputXpt2WidgetIDsConstIter iter(gOutputXpt2WidgetIDs.find(inOutputXpt));
		iter != gOutputXpt2WidgetIDs.end()  &&  iter->first == inOutputXpt;
		++iter)
			outWidgetIDs.insert(iter->second);
	return !outWidgetIDs.empty();
}

bool				RoutingExpert::GetWidgetsForInput (const NTV2InputXptID inInputXpt, NTV2WidgetIDSet & outWidgetIDs) const
{
	AJAAutoLock	lock(&gLock);
	NTV2_ASSERT(!gInputXpt2WidgetIDs.empty());
	outWidgetIDs.clear();
	InputXpt2WidgetIDsConstIter iter(gInputXpt2WidgetIDs.find(inInputXpt));
	while (iter != gInputXpt2WidgetIDs.end()  &&  iter->first == inInputXpt)
	{
		outWidgetIDs.insert(iter->second);
		++iter;
	}
	return !outWidgetIDs.empty();
}

bool				RoutingExpert::GetWidgetInputs (const NTV2WidgetID inWidgetID, NTV2InputXptIDSet & outInputs) const
{
	AJAAutoLock	lock(&gLock);
	NTV2_ASSERT(!gWidget2InputXpts.empty());
	outInputs.clear();
	Widget2InputXptsConstIter	iter(gWidget2InputXpts.find(inWidgetID));
	while (iter != gWidget2InputXpts.end() && iter->first == inWidgetID)
	{
		outInputs.insert(iter->second);
		++iter;
	}
	return !outInputs.empty();
}

bool				RoutingExpert::GetWidgetOutputs (const NTV2WidgetID inWidgetID, NTV2OutputXptIDSet & outOutputs) const
{
	AJAAutoLock	lock(&gLock);
	NTV2_ASSERT(!gWidget2OutputXpts.empty());
	outOutputs.clear();
	Widget2OutputXptsConstIter	iter(gWidget2OutputXpts.find(inWidgetID));
	while (iter != gWidget2OutputXpts.end()  &&  iter->first == inWidgetID)
	{
		outOutputs.insert(iter->second);
		++iter;
	}
	return !outOutputs.empty();
}

bool				RoutingExpert::IsOutputXptValid (const NTV2OutputXptID inOutputXpt) const
{
	AJAAutoLock	lock(&gLock);
	NTV2_ASSERT(!gOutputXpt2WidgetIDs.empty());
	return gOutputXpt2WidgetIDs.find(inOutputXpt) != gOutputXpt2WidgetIDs.end();
}

bool				RoutingExpert::IsRGBOnlyInputXpt (const NTV2InputXptID inInputXpt) const
{
	AJAAutoLock	lock(&gLock);
	NTV2_ASSERT(!gRGBOnlyInputXpts.empty());
	return gRGBOnlyInputXpts.find(inInputXpt) != gRGBOnlyInputXpts.end();
}

bool				RoutingExpert::IsYUVOnlyInputXpt (const NTV2InputXptID inInputXpt) const
{
	AJAAutoLock	lock(&gLock);
	NTV2_ASSERT(!gYUVOnlyInputXpts.empty());
	return gYUVOnlyInputXpts.find(inInputXpt) != gYUVOnlyInputXpts.end();
}

bool				RoutingExpert::IsKeyInputXpt (const NTV2InputXptID inInputXpt) const
{
	AJAAutoLock	lock(&gLock);
	NTV2_ASSERT(!gKeyInputXpts.empty());
	return gKeyInputXpts.find(inInputXpt) != gKeyInputXpts.end();
}

bool				RoutingExpert::IsSDIWidget(const NTV2WidgetType inWidgetType) const
{
	AJAAutoLock lock(&gLock);
	NTV2_ASSERT(!gSDIWidgetTypes.empty());
	return gSDIWidgetTypes.find(inWidgetType) != gSDIWidgetTypes.end();
}

bool				RoutingExpert::IsSDIInWidget(const NTV2WidgetType inWidgetType) const
{
	AJAAutoLock lock(&gLock);
	NTV2_ASSERT(!gSDIInWidgetTypes.empty());
	return gSDIInWidgetTypes.find(inWidgetType) != gSDIInWidgetTypes.end();
}

bool				RoutingExpert::IsSDIOutWidget(const NTV2WidgetType inWidgetType) const
{
	AJAAutoLock lock(&gLock);
	NTV2_ASSERT(!gSDIOutWidgetTypes.empty());
	return gSDIOutWidgetTypes.find(inWidgetType) != gSDIOutWidgetTypes.end();
}

bool				RoutingExpert::Is3GSDIWidget(const NTV2WidgetType inWidgetType) const
{
	AJAAutoLock lock(&gLock);
	NTV2_ASSERT(!gSDI3GWidgetTypes.empty());
	return gSDI3GWidgetTypes.find(inWidgetType) != gSDI3GWidgetTypes.end();
}

bool				RoutingExpert::Is12GSDIWidget(const NTV2WidgetType inWidgetType) const
{
	AJAAutoLock lock(&gLock);
	NTV2_ASSERT(!gSDI12GWidgetTypes.empty());
	return gSDI12GWidgetTypes.find(inWidgetType) != gSDI12GWidgetTypes.end();
}

bool				RoutingExpert::IsDualLinkWidget(const NTV2WidgetType inWidgetType) const
{
	AJAAutoLock lock(&gLock);
	NTV2_ASSERT(!gDualLinkWidgetTypes.empty());
	return gDualLinkWidgetTypes.find(inWidgetType) != gDualLinkWidgetTypes.end();
}

bool				RoutingExpert::IsDualLinkInWidget(const NTV2WidgetType inWidgetType) const
{
	AJAAutoLock lock(&gLock);
	NTV2_ASSERT(!gDualLinkInWidgetTypes.empty());
	return gDualLinkInWidgetTypes.find(inWidgetType) != gDualLinkInWidgetTypes.end();
}

bool				RoutingExpert::IsDualLinkOutWidget(const NTV2WidgetType inWidgetType) const
{
	AJAAutoLock lock(&gLock);
	NTV2_ASSERT(!gDualLinkOutWidgetTypes.empty());
	return gDualLinkOutWidgetTypes.find(inWidgetType) != gDualLinkOutWidgetTypes.end();
}

bool				RoutingExpert::IsHDMIWidget(const NTV2WidgetType inWidgetType) const
{
	AJAAutoLock lock(&gLock);
	NTV2_ASSERT(!gHDMIWidgetTypes.empty());
	return gHDMIWidgetTypes.find(inWidgetType) != gHDMIWidgetTypes.end();
}

bool				RoutingExpert::IsHDMIInWidget(const NTV2WidgetType inWidgetType) const
{
	AJAAutoLock lock(&gLock);
	NTV2_ASSERT(!gHDMIInWidgetTypes.empty());
	return gHDMIInWidgetTypes.find(inWidgetType) != gHDMIInWidgetTypes.end();
}

bool				RoutingExpert::IsHDMIOutWidget(const NTV2WidgetType inWidgetType) const
{
	AJAAutoLock lock(&gLock);
	NTV2_ASSERT(!gHDMIOutWidgetTypes.empty());
	return gHDMIOutWidgetTypes.find(inWidgetType) != gHDMIOutWidgetTypes.end();
}


#define NTV2SR_ASSIGN_BOTH(enumToStrMap, strToEnumMap, inEnum, inNameStr)		\
	{																			\
		enumToStrMap[inEnum] = inNameStr;										\
		std::string lowerstr_(#inEnum);												\
		strToEnumMap[aja::lower(lowerstr_)] = inEnum;							\
	}

void RoutingExpert::InitInputXpt2String(void)
{
	//	gInputXpt2String	--	widgets with inputs & outputs
	std::string lowerstr;
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_Xpt425Mux1AInput,	"425Mux1a");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_Xpt425Mux1BInput,	"425Mux1b");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_Xpt425Mux2AInput,	"425Mux2a");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_Xpt425Mux2BInput,	"425Mux2b");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_Xpt425Mux3AInput,	"425Mux3a");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_Xpt425Mux3BInput,	"425Mux3b");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_Xpt425Mux4AInput,	"425Mux4a");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_Xpt425Mux4BInput,	"425Mux4b");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_Xpt4KDCQ1Input,	"4KDCQ1");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_Xpt4KDCQ2Input,	"4KDCQ2");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_Xpt4KDCQ3Input,	"4KDCQ3");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_Xpt4KDCQ4Input,	"4KDCQ4");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptCSC1KeyInput,	"CSC1Key");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptCSC1VidInput,	"CSC1");	//	, "CSC1Vid";
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptCSC2KeyInput,	"CSC2Key");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptCSC2VidInput,	"CSC2");	//	, "CSC2Vid";
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptCSC3KeyInput,	"CSC3Key");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptCSC3VidInput,	"CSC3");	//	, "CSC3Vid";
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptCSC4KeyInput,	"CSC4Key");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptCSC4VidInput,	"CSC4");	//	, "CSC4Vid";
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptCSC5KeyInput,	"CSC5Key");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptCSC5VidInput,	"CSC5");	//	, "CSC5Vid";
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptCSC6KeyInput,	"CSC6Key");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptCSC6VidInput,	"CSC6");	//	, "CSC6Vid";
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptCSC7KeyInput,	"CSC7Key");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptCSC7VidInput,	"CSC7");	//	, "CSC7Vid";
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptCSC8KeyInput,	"CSC8Key");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptCSC8VidInput,	"CSC8");	//	, "CSC8Vid";
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkIn1Input,	"DLIn1");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkIn1DSInput,	"DLIn1DS");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkIn2Input,	"DLIn2");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkIn2DSInput,	"DLIn2DS");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkIn3Input,	"DLIn3");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkIn3DSInput,	"DLIn3DS");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkIn4Input,	"DLIn4");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkIn4DSInput,	"DLIn4DS");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkIn5Input,	"DLIn5");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkIn5DSInput,	"DLIn5DS");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkIn6Input,	"DLIn6");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkIn6DSInput,	"DLIn6DS");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkIn7Input,	"DLIn7");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkIn7DSInput,	"DLIn7DS");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkIn8Input,	"DLIn8");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkIn8DSInput,	"DLIn8DS");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkOut1Input,	"DLOut1");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkOut2Input,	"DLOut2");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkOut3Input,	"DLOut3");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkOut4Input,	"DLOut4");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkOut5Input,	"DLOut5");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkOut6Input,	"DLOut6");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkOut7Input,	"DLOut7");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptDualLinkOut8Input,	"DLOut8");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptFrameBuffer1Input,	"FB1");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptFrameBuffer1BInput,	"FB1B");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptFrameBuffer2Input,	"FB2");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptFrameBuffer2BInput,	"FB2B");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptFrameBuffer3Input,	"FB3");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptFrameBuffer3BInput,	"FB3B");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptFrameBuffer4Input,	"FB4");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptFrameBuffer4BInput,	"FB4B");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptFrameBuffer5Input,	"FB5");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptFrameBuffer5BInput,	"FB5B");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptFrameBuffer6Input,	"FB6");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptFrameBuffer6BInput,	"FB6B");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptFrameBuffer7Input,	"FB7");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptFrameBuffer7BInput,	"FB7B");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptFrameBuffer8Input,	"FB8");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptFrameBuffer8BInput,	"FB8B");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptLUT1Input,	"LUT1");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptLUT2Input,	"LUT2");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptLUT3Input,	"LUT3");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptLUT4Input,	"LUT4");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptLUT5Input,	"LUT5");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptLUT6Input,	"LUT6");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptLUT7Input,	"LUT7");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptLUT8Input,	"LUT8");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptMixer1BGKeyInput,	"Mixer1BGKey");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptMixer1BGVidInput,	"Mixer1BG");	//	, "Mixer1BGVid";
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptMixer1FGKeyInput,	"Mixer1FGKey");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptMixer1FGVidInput,	"Mixer1FG");	//	, "Mixer1FGVid";
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptMixer2BGKeyInput,	"Mixer2BGKey");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptMixer2BGVidInput,	"Mixer2BG");	//	, "Mixer2BGVid";
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptMixer2FGKeyInput,	"Mixer2FGKey");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptMixer2FGVidInput,	"Mixer2FG");	//	, "Mixer2FGVid";
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptMixer3BGKeyInput,	"Mixer3BGKey");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptMixer3BGVidInput,	"Mixer3BG");	//	, "Mixer3BGVid";
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptMixer3FGKeyInput,	"Mixer3FGKey");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptMixer3FGVidInput,	"Mixer3FG");	//	, "Mixer3FGVid";
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptMixer4BGKeyInput,	"Mixer4BGKey");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptMixer4BGVidInput,	"Mixer4BG");	//	, "Mixer4BGVid";
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptMixer4FGKeyInput,	"Mixer4FGKey");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptMixer4FGVidInput,	"Mixer4FG");	//	, "Mixer4FGVid";
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptFrameSync1Input,	"FS1");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptFrameSync2Input,	"FS2");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptCompressionModInput,	"Comp");		//	, "Compress";
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptConversionModInput,	"Conv");		//	, "Convert"
	//	gInputXpt2String	--	widgets with only inputs
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptAnalogOutInput,	"AnalogOut");	//	, "AnlgOut"
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptSDIOut1Input,	"SDIOut1");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptSDIOut1InputDS2,	"SDIOut1DS");	//	, "SDIOut1DS2"
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptSDIOut2Input,	"SDIOut2");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptSDIOut2InputDS2,	"SDIOut2DS");	//	, "SDIOut2DS2"
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptSDIOut3Input,	"SDIOut3");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptSDIOut3InputDS2,	"SDIOut3DS");	//	, "SDIOut3DS2"
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptSDIOut4Input,	"SDIOut4");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptSDIOut4InputDS2,	"SDIOut4DS");	//	, "SDIOut4DS2"
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptSDIOut5Input,	"SDIOut5");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptSDIOut5InputDS2,	"SDIOut5DS");	//	, "SDIOut5DS2"
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptSDIOut6Input,	"SDIOut6");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptSDIOut6InputDS2,	"SDIOut6DS");	//	, "SDIOut6DS2"
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptSDIOut7Input,	"SDIOut7");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptSDIOut7InputDS2,	"SDIOut7DS");	//	, "SDIOut7DS2"
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptSDIOut8Input,	"SDIOut8");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptSDIOut8InputDS2,	"SDIOut8DS");	//	, "SDIOut8DS2"
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptHDMIOutInput,	"HDMIOut");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptHDMIOutQ1Input,	"HDMIOutQ1");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptHDMIOutQ2Input,	"HDMIOutQ2");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptHDMIOutQ3Input,	"HDMIOutQ3");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptHDMIOutQ4Input,	"HDMIOutQ4");
	NTV2SR_ASSIGN_BOTH(gInputXpt2String, gString2InputXpt, NTV2_XptOEInput,	"OEInput");

	//	gString2InputXpt
	for (InputXpt2StringConstIter iter(gInputXpt2String.begin());  iter != gInputXpt2String.end();  ++iter)
	{
		std::string	lowerStr(iter->second);	aja::lower(lowerStr);
		gString2InputXpt [lowerStr] = iter->first;
	}
	//	Aliases:
	gString2InputXpt ["csc1vid"]		=	NTV2_XptCSC1VidInput;
	gString2InputXpt ["csc2vid"]		=	NTV2_XptCSC2VidInput;
	gString2InputXpt ["csc3vid"]		=	NTV2_XptCSC3VidInput;
	gString2InputXpt ["csc4vid"]		=	NTV2_XptCSC4VidInput;
	gString2InputXpt ["csc5vid"]		=	NTV2_XptCSC5VidInput;
	gString2InputXpt ["csc6vid"]		=	NTV2_XptCSC6VidInput;
	gString2InputXpt ["csc7vid"]		=	NTV2_XptCSC7VidInput;
	gString2InputXpt ["csc8vid"]		=	NTV2_XptCSC8VidInput;
	gString2InputXpt ["mixer1bgvid"]	=	NTV2_XptMixer1BGVidInput;
	gString2InputXpt ["mixer1fgvid"]	=	NTV2_XptMixer1FGVidInput;
	gString2InputXpt ["mixer2bgvid"]	=	NTV2_XptMixer2BGVidInput;
	gString2InputXpt ["mixer2fgvid"]	=	NTV2_XptMixer2FGVidInput;
	gString2InputXpt ["mixer3bgvid"]	=	NTV2_XptMixer3BGVidInput;
	gString2InputXpt ["mixer3fgvid"]	=	NTV2_XptMixer3FGVidInput;
	gString2InputXpt ["mixer4bgvid"]	=	NTV2_XptMixer4BGVidInput;
	gString2InputXpt ["mixer4fgvid"]	=	NTV2_XptMixer4FGVidInput;
	gString2InputXpt ["compress"]		=	NTV2_XptCompressionModInput;
	gString2InputXpt ["compressor"]		=	NTV2_XptCompressionModInput;
	gString2InputXpt ["convert"]		=	NTV2_XptConversionModInput;
	gString2InputXpt ["converter"]		=	NTV2_XptConversionModInput;
	gString2InputXpt ["anlgout"]		=	NTV2_XptAnalogOutInput;
	gString2InputXpt ["sdiout1ds2"]		=	NTV2_XptSDIOut1InputDS2;
	gString2InputXpt ["sdiout2ds2"]		=	NTV2_XptSDIOut2InputDS2;
	gString2InputXpt ["sdiout3ds2"]		=	NTV2_XptSDIOut3InputDS2;
	gString2InputXpt ["sdiout4ds2"]		=	NTV2_XptSDIOut4InputDS2;
	gString2InputXpt ["sdiout5ds2"]		=	NTV2_XptSDIOut5InputDS2;
	gString2InputXpt ["sdiout6ds2"]		=	NTV2_XptSDIOut6InputDS2;
	gString2InputXpt ["sdiout7ds2"]		=	NTV2_XptSDIOut7InputDS2;
	gString2InputXpt ["sdiout8ds2"]		=	NTV2_XptSDIOut8InputDS2;
}

void RoutingExpert::InitOutputXpt2String(void)
{
	//	gOutputXpt2String
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_Xpt425Mux1ARGB,	"425Mux1aRGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_Xpt425Mux1AYUV,	"425Mux1aYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_Xpt425Mux1BRGB,	"425Mux1bRGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_Xpt425Mux1BYUV,	"425Mux1bYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_Xpt425Mux2ARGB,	"425Mux2aRGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_Xpt425Mux2AYUV,	"425Mux2aYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_Xpt425Mux2BRGB,	"425Mux2bRGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_Xpt425Mux2BYUV,	"425Mux2bYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_Xpt425Mux3ARGB,	"425Mux3aRGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_Xpt425Mux3AYUV,	"425Mux3aYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_Xpt425Mux3BRGB,	"425Mux3bRGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_Xpt425Mux3BYUV,	"425Mux3bYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_Xpt425Mux4ARGB,	"425Mux4aRGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_Xpt425Mux4AYUV,	"425Mux4aYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_Xpt425Mux4BRGB,	"425Mux4bRGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_Xpt425Mux4BYUV,	"425Mux4bYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_Xpt4KDownConverterOut,	"4KDC");		//	, "4KDCYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_Xpt4KDownConverterOutRGB,	"4KDCRGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC1KeyYUV,	"CSC1Key");	//	, "CSC1KeyYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC1VidRGB,	"CSC1RGB");	//	, "CSC1VidRGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC1VidYUV,	"CSC1");		//	, "CSC1YUV", "CSC1VidYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC2KeyYUV,	"CSC2Key");	//	, "CSC2KeyYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC2VidRGB,	"CSC2RGB");	//	, "CSC2VidRGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC2VidYUV,	"CSC2");		//	, "CSC2YUV", "CSC2VidYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC3KeyYUV,	"CSC3Key");	//	, "CSC3KeyYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC3VidRGB,	"CSC3RGB");	//	, "CSC3VidRGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC3VidYUV,	"CSC3");		//	, "CSC3YUV", "CSC3VidYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC4KeyYUV,	"CSC4Key");	//	, "CSC4KeyYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC4VidRGB,	"CSC4RGB");	//	, "CSC4VidRGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC4VidYUV,	"CSC4");		//	, "CSC4YUV", "CSC4VidYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC5KeyYUV,	"CSC5Key");	//	, "CSC5KeyYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC5VidRGB,	"CSC5RGB");	//	, "CSC5VidRGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC5VidYUV,	"CSC5");		//	, "CSC5YUV", "CSC5VidYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC6KeyYUV,	"CSC6Key");	//	, "CSC6KeyYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC6VidRGB,	"CSC6RGB");	//	, "CSC6VidRGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC6VidYUV,	"CSC6");		//	, "CSC6YUV", "CSC6VidYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC7KeyYUV,	"CSC7Key");	//	, "CSC7KeyYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC7VidRGB,	"CSC7RGB");	//	, "CSC7VidRGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC7VidYUV,	"CSC7");		//	, "CSC7YUV", "CSC7VidYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC8KeyYUV,	"CSC8Key");	//	, "CSC8KeyYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC8VidRGB,	"CSC8RGB");	//	, "CSC8VidRGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCSC8VidYUV,	"CSC8");		//	, "CSC8YUV", "CSC8VidYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkIn1,	"DLIn1");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkIn2,	"DLIn2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkIn3,	"DLIn3");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkIn4,	"DLIn4");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkIn5,	"DLIn5");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkIn6,	"DLIn6");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkIn7,	"DLIn7");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkIn8,	"DLIn8");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkOut1,	"DLOut1");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkOut1DS2,	"DLOut1DS");	//	, "DLOut1DS2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkOut2,	"DLOut2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkOut2DS2,	"DLOut2DS");	//	, "DLOut2DS2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkOut3,	"DLOut3");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkOut3DS2,	"DLOut3DS");	//	, "DLOut3DS2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkOut4,	"DLOut4");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkOut4DS2,	"DLOut4DS");	//	, "DLOut4DS2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkOut5,	"DLOut5");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkOut5DS2,	"DLOut5DS");	//	, "DLOut5DS2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkOut6,	"DLOut6");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkOut6DS2,	"DLOut6DS");	//	, "DLOut6DS2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkOut7,	"DLOut7");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkOut7DS2,	"DLOut7DS");	//	, "DLOut7DS2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkOut8,	"DLOut8");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptDuallinkOut8DS2,	"DLOut8DS");	//	, "DLOut8DS2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer1_DS2RGB,	"FB1RGBDS2");	//	Formerly "FB1RGB425"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer1_DS2YUV,	"FB1YUVDS2");	//	Formerly "FB1YUV425"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer1RGB,	"FB1RGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer1YUV,	"FB1");		//	, "FB1YUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer2_DS2RGB,	"FB2RGBDS2");	//	Formerly "FB2RGB425"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer2_DS2YUV,	"FB2YUVDS2");	//	Formerly "FB2YUV425"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer2RGB,	"FB2RGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer2YUV,	"FB2");		//	, "FB2YUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer3_DS2RGB,	"FB3RGBDS2");	//	Formerly "FB3RGB425"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer3_DS2YUV,	"FB3YUVDS2");	//	Formerly "FB3YUV425"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer3RGB,	"FB3RGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer3YUV,	"FB3");		//	, "FB3YUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer4_DS2RGB,	"FB4RGBDS2");	//	Formerly "FB4RGB425"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer4_DS2YUV,	"FB4YUVDS2");	//	Formerly "FB4YUV425"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer4RGB,	"FB4RGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer4YUV,	"FB4");		//	, "FB4YUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer5_DS2RGB,	"FB5RGBDS2");	//	Formerly "FB5RGB425"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer5_DS2YUV,	"FB5YUVDS2");	//	Formerly "FB5YUV425"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer5RGB,	"FB5RGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer5YUV,	"FB5");		//	, "FB5YUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer6_DS2RGB,	"FB6RGBDS2");	//	Formerly "FB6RGB425"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer6_DS2YUV,	"FB6YUVDS2");	//	Formerly "FB6YUV425"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer6RGB,	"FB6RGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer6YUV,	"FB6");		//	, "FB6YUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer7_DS2RGB,	"FB7RGBDS2");	//	Formerly "FB7RGB425"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer7_DS2YUV,	"FB7YUVDS2");	//	Formerly "FB7YUV425"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer7RGB,	"FB7RGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer7YUV,	"FB7");		//	, "FB7YUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer8_DS2RGB,	"FB8RGBDS2");	//	Formerly "FB8RGB425"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer8_DS2YUV,	"FB8YUVDS2");	//	Formerly "FB8YUV425"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer8RGB,	"FB8RGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameBuffer8YUV,	"FB8");		//	, "FB8YUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptLUT1Out,	"LUT1");	//	Formerly "LUT1RGB"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptLUT1YUV,	"LUT1YUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptLUT2Out,	"LUT2");	//	Formerly "LUT2RGB"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptLUT3Out,	"LUT3");	//	Formerly "LUT3RGB"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptLUT4Out,	"LUT4");	//	Formerly "LUT4RGB"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptLUT5Out,	"LUT5");	//	Formerly "LUT5RGB"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptLUT6Out,	"LUT6");	//	Formerly "LUT6RGB"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptLUT7Out,	"LUT7");	//	Formerly "LUT7RGB"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptLUT8Out,	"LUT8");	//	Formerly "LUT8RGB"
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptMixer1KeyYUV,	"Mixer1Key");	//	, "Mixer1KeyYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptMixer1VidYUV,	"Mixer1");		//	, "Mixer1Vid", "Mixer1VidYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptMixer1VidRGB,	"Mixer1RGB");	//	, "Mixer1VidRGB", "Mixer1VidRGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptMixer2KeyYUV,	"Mixer2Key");	//	, "Mixer2KeyYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptMixer2VidYUV,	"Mixer2");		//	, "Mixer2Vid", "Mixer2VidYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptMixer3KeyYUV,	"Mixer3Key");	//	, "Mixer3KeyYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptMixer3VidYUV,	"Mixer3");		//	, "Mixer3Vid", "Mixer3VidYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptMixer4KeyYUV,	"Mixer4Key");	//	, "Mixer4KeyYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptMixer4VidYUV,	"Mixer4");		//	, "Mixer4Vid", "Mixer4VidYUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameSync1RGB,	"FS1RGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameSync1YUV,	"FS1YUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameSync2RGB,	"FS2RGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptFrameSync2YUV,	"FS2YUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptCompressionModule,	"Comp");		//	, "Compress");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptConversionModule,	"Conv");		//	, "Convert");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptBlack,	"Black");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptAlphaOut,	"AlphaOut");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptAnalogIn,	"AnalogIn");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptSDIIn1,	"SDIIn1");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptSDIIn1DS2,	"SDIIn1DS2");	//	, "SDIIn1DS2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptSDIIn2,	"SDIIn2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptSDIIn2DS2,	"SDIIn2DS2");	//	, "SDIIn2DS2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptSDIIn3,	"SDIIn3");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptSDIIn3DS2,	"SDIIn3DS2");	//	, "SDIIn3DS2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptSDIIn4,	"SDIIn4");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptSDIIn4DS2,	"SDIIn4DS2");	//	, "SDIIn4DS2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptSDIIn5,	"SDIIn5");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptSDIIn5DS2,	"SDIIn5DS2");	//	, "SDIIn5DS2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptSDIIn6,	"SDIIn6");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptSDIIn6DS2,	"SDIIn6DS2");	//	, "SDIIn6DS2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptSDIIn7,	"SDIIn7");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptSDIIn7DS2,	"SDIIn7DS2");	//	, "SDIIn7DS2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptSDIIn8,	"SDIIn8");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptSDIIn8DS2,	"SDIIn8DS2");	//	, "SDIIn8DS2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptHDMIIn1,	"HDMIIn1");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptHDMIIn1Q2,	"HDMIIn1Q2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptHDMIIn1Q2RGB,	"HDMIIn1Q2RGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptHDMIIn1Q3,	"HDMIIn1Q3");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptHDMIIn1Q3RGB,	"HDMIIn1Q3RGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptHDMIIn1Q4,	"HDMIIn1Q4");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptHDMIIn1Q4RGB,	"HDMIIn1Q4RGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptHDMIIn1RGB,	"HDMIIn1RGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptHDMIIn2,	"HDMIIn2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptHDMIIn2Q2,	"HDMIIn2Q2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptHDMIIn2Q2RGB,	"HDMIIn2Q2RGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptHDMIIn2Q3,	"HDMIIn2Q3");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptHDMIIn2Q3RGB,	"HDMIIn2Q3RGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptHDMIIn2Q4,	"HDMIIn2Q4");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptHDMIIn2Q4RGB,	"HDMIIn2Q4RGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptHDMIIn2RGB,	"HDMIIn2RGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptHDMIIn3,	"HDMIIn3");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptHDMIIn3RGB,	"HDMIIn3RGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptHDMIIn4,	"HDMIIn4");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptHDMIIn4RGB,	"HDMIIn4RGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptMultiLinkOut1DS1,	"MLOut1DS1");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptMultiLinkOut1DS2,	"MLOut1DS2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptMultiLinkOut1DS3,	"MLOut1DS3");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptMultiLinkOut1DS4,	"MLOut1DS4");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptMultiLinkOut2DS1,	"MLOut2DS1");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptMultiLinkOut2DS2,	"MLOut2DS2");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptMultiLinkOut2DS3,	"MLOut2DS3");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptMultiLinkOut2DS4,	"MLOut2DS4");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_Xpt3DLUT1RGB,	"3DLUT1RGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_Xpt3DLUT1YUV,	"3DLUT1YUV");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptOEOutRGB,	"OEOutRGB");
	NTV2SR_ASSIGN_BOTH(gOutputXpt2String, gString2OutputXpt, NTV2_XptOEOutYUV,	"OEOutYUV");

	//	gString2OutputXpt
	for (OutputXpt2StringConstIter iter (gOutputXpt2String.begin ());  iter != gOutputXpt2String.end ();  ++iter)
	{
		std::string	lowerStr(iter->second);	aja::lower(lowerStr);
		gString2OutputXpt [lowerStr] = iter->first;
	}
	//	Aliases:
	gString2OutputXpt ["4kdcyuv"]		= NTV2_Xpt4KDownConverterOut;
	gString2OutputXpt ["csc1keyyuv"]	= NTV2_XptCSC1KeyYUV;
	gString2OutputXpt ["csc1vidrgb"]	= NTV2_XptCSC1VidRGB;
	gString2OutputXpt ["csc1yuv"]		= NTV2_XptCSC1VidYUV;
	gString2OutputXpt ["csc1vidyuv"]	= NTV2_XptCSC1VidYUV;
	gString2OutputXpt ["csc2keyyuv"]	= NTV2_XptCSC2KeyYUV;
	gString2OutputXpt ["csc2vidrgb"]	= NTV2_XptCSC2VidRGB;
	gString2OutputXpt ["csc2yuv"]		= NTV2_XptCSC2VidYUV;
	gString2OutputXpt ["csc2vidyuv"]	= NTV2_XptCSC2VidYUV;
	gString2OutputXpt ["csc3keyyuv"]	= NTV2_XptCSC3KeyYUV;
	gString2OutputXpt ["csc3vidrgb"]	= NTV2_XptCSC3VidRGB;
	gString2OutputXpt ["csc3yuv"]		= NTV2_XptCSC3VidYUV;
	gString2OutputXpt ["csc3vidyuv"]	= NTV2_XptCSC3VidYUV;
	gString2OutputXpt ["csc4keyyuv"]	= NTV2_XptCSC4KeyYUV;
	gString2OutputXpt ["csc4vidrgb"]	= NTV2_XptCSC4VidRGB;
	gString2OutputXpt ["csc4yuv"]		= NTV2_XptCSC4VidYUV;
	gString2OutputXpt ["csc4vidyuv"]	= NTV2_XptCSC4VidYUV;
	gString2OutputXpt ["csc5keyyuv"]	= NTV2_XptCSC5KeyYUV;
	gString2OutputXpt ["csc5vidrgb"]	= NTV2_XptCSC5VidRGB;
	gString2OutputXpt ["csc5yuv"]		= NTV2_XptCSC5VidYUV;
	gString2OutputXpt ["csc5vidyuv"]	= NTV2_XptCSC5VidYUV;
	gString2OutputXpt ["csc6keyyuv"]	= NTV2_XptCSC6KeyYUV;
	gString2OutputXpt ["csc6vidrgb"]	= NTV2_XptCSC6VidRGB;
	gString2OutputXpt ["csc6yuv"]		= NTV2_XptCSC6VidYUV;
	gString2OutputXpt ["csc6vidyuv"]	= NTV2_XptCSC6VidYUV;
	gString2OutputXpt ["csc7keyyuv"]	= NTV2_XptCSC7KeyYUV;
	gString2OutputXpt ["csc7vidrgb"]	= NTV2_XptCSC7VidRGB;
	gString2OutputXpt ["csc7yuv"]		= NTV2_XptCSC7VidYUV;
	gString2OutputXpt ["csc7vidyuv"]	= NTV2_XptCSC7VidYUV;
	gString2OutputXpt ["csc8keyyuv"]	= NTV2_XptCSC8KeyYUV;
	gString2OutputXpt ["csc8vidrgb"]	= NTV2_XptCSC8VidRGB;
	gString2OutputXpt ["csc8yuv"]		= NTV2_XptCSC8VidYUV;
	gString2OutputXpt ["csc8VidYUV"]	= NTV2_XptCSC8VidYUV;
	gString2OutputXpt ["dlout1ds2"]		= NTV2_XptDuallinkOut1DS2;
	gString2OutputXpt ["dlout2ds2"]		= NTV2_XptDuallinkOut2DS2;
	gString2OutputXpt ["dlout3ds2"]		= NTV2_XptDuallinkOut3DS2;
	gString2OutputXpt ["dlout4ds2"]		= NTV2_XptDuallinkOut4DS2;
	gString2OutputXpt ["dlout5ds2"]		= NTV2_XptDuallinkOut5DS2;
	gString2OutputXpt ["dlout6ds2"]		= NTV2_XptDuallinkOut6DS2;
	gString2OutputXpt ["dlout7ds2"]		= NTV2_XptDuallinkOut7DS2;
	gString2OutputXpt ["dlout8ds2"]		= NTV2_XptDuallinkOut8DS2;
	gString2OutputXpt ["fb1yuv"]		= NTV2_XptFrameBuffer1YUV;
	gString2OutputXpt ["fb2yuv"]		= NTV2_XptFrameBuffer2YUV;
	gString2OutputXpt ["fb3yuv"]		= NTV2_XptFrameBuffer3YUV;
	gString2OutputXpt ["fb4yuv"]		= NTV2_XptFrameBuffer4YUV;
	gString2OutputXpt ["fb5yuv"]		= NTV2_XptFrameBuffer5YUV;
	gString2OutputXpt ["fb6yuv"]		= NTV2_XptFrameBuffer6YUV;
	gString2OutputXpt ["fb7yuv"]		= NTV2_XptFrameBuffer7YUV;
	gString2OutputXpt ["fb8yuv"]		= NTV2_XptFrameBuffer8YUV;
	gString2OutputXpt ["mixer1keyyuv"]	= NTV2_XptMixer1KeyYUV;
	gString2OutputXpt ["mixer1vid"]		= NTV2_XptMixer1VidYUV;
	gString2OutputXpt ["mixer1vidyuv"]	= NTV2_XptMixer1VidYUV;
	gString2OutputXpt ["mixer1vidrgb"]	= NTV2_XptMixer1VidRGB;
	gString2OutputXpt ["mixer2keyyuv"]	= NTV2_XptMixer2KeyYUV;
	gString2OutputXpt ["mixer2vid"]		= NTV2_XptMixer2VidYUV;
	gString2OutputXpt ["mixer2vidyuv"]	= NTV2_XptMixer2VidYUV;
	gString2OutputXpt ["mixer3keyyuv"]	= NTV2_XptMixer3KeyYUV;
	gString2OutputXpt ["mixer3vid"]		= NTV2_XptMixer3VidYUV;
	gString2OutputXpt ["mixer3vidyuv"]	= NTV2_XptMixer3VidYUV;
	gString2OutputXpt ["mixer4keyyuv"]	= NTV2_XptMixer4KeyYUV;
	gString2OutputXpt ["mixer4vid"]		= NTV2_XptMixer4VidYUV;
	gString2OutputXpt ["mixer4vidyuv"]	= NTV2_XptMixer4VidYUV;
	gString2OutputXpt ["compress"]		= NTV2_XptCompressionModule;
	gString2OutputXpt ["convert"]		= NTV2_XptConversionModule;
	gString2OutputXpt ["sdiin1ds2"]		= NTV2_XptSDIIn1DS2;
	gString2OutputXpt ["sdiin2ds2"]		= NTV2_XptSDIIn2DS2;
	gString2OutputXpt ["sdiin3ds2"]		= NTV2_XptSDIIn3DS2;
	gString2OutputXpt ["sdiin4ds2"]		= NTV2_XptSDIIn4DS2;
	gString2OutputXpt ["sdiin5ds2"]		= NTV2_XptSDIIn5DS2;
	gString2OutputXpt ["sdiin6ds2"]		= NTV2_XptSDIIn6DS2;
	gString2OutputXpt ["sdiin7ds2"]		= NTV2_XptSDIIn7DS2;
	gString2OutputXpt ["sdiin8ds2"]		= NTV2_XptSDIIn8DS2;
}

void RoutingExpert::InitInputXpt2WidgetIDs(void)
{
	//	gInputXpt2WidgetIDs
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_Xpt425Mux1AInput,			NTV2_Wgt425Mux1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_Xpt425Mux1BInput,			NTV2_Wgt425Mux1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_Xpt425Mux2AInput,			NTV2_Wgt425Mux2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_Xpt425Mux2BInput,			NTV2_Wgt425Mux2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_Xpt425Mux3AInput,			NTV2_Wgt425Mux3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_Xpt425Mux3BInput,			NTV2_Wgt425Mux3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_Xpt425Mux4AInput,			NTV2_Wgt425Mux4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_Xpt425Mux4BInput,			NTV2_Wgt425Mux4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_Xpt4KDCQ1Input,				NTV2_Wgt4KDownConverter));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_Xpt4KDCQ2Input,				NTV2_Wgt4KDownConverter));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_Xpt4KDCQ3Input,				NTV2_Wgt4KDownConverter));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_Xpt4KDCQ4Input,				NTV2_Wgt4KDownConverter));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptCSC1KeyInput,			NTV2_WgtCSC1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptCSC1VidInput,			NTV2_WgtCSC1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptCSC2KeyInput,			NTV2_WgtCSC2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptCSC2VidInput,			NTV2_WgtCSC2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptCSC3KeyInput,			NTV2_WgtCSC3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptCSC3VidInput,			NTV2_WgtCSC3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptCSC4KeyInput,			NTV2_WgtCSC4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptCSC4VidInput,			NTV2_WgtCSC4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptCSC5KeyInput,			NTV2_WgtCSC5));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptCSC5VidInput,			NTV2_WgtCSC5));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptCSC6KeyInput,			NTV2_WgtCSC6));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptCSC6VidInput,			NTV2_WgtCSC6));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptCSC7KeyInput,			NTV2_WgtCSC7));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptCSC7VidInput,			NTV2_WgtCSC7));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptCSC8KeyInput,			NTV2_WgtCSC8));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptCSC8VidInput,			NTV2_WgtCSC8));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkIn1Input,		NTV2_WgtDualLinkV2In1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkIn1DSInput,		NTV2_WgtDualLinkV2In1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkIn2Input,		NTV2_WgtDualLinkV2In2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkIn2DSInput,		NTV2_WgtDualLinkV2In2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkIn3Input,		NTV2_WgtDualLinkV2In3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkIn3DSInput,		NTV2_WgtDualLinkV2In3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkIn4Input,		NTV2_WgtDualLinkV2In4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkIn4DSInput,		NTV2_WgtDualLinkV2In4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkIn5Input,		NTV2_WgtDualLinkV2In5));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkIn5DSInput,		NTV2_WgtDualLinkV2In5));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkIn6Input,		NTV2_WgtDualLinkV2In6));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkIn6DSInput,		NTV2_WgtDualLinkV2In6));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkIn7Input,		NTV2_WgtDualLinkV2In7));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkIn7DSInput,		NTV2_WgtDualLinkV2In7));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkIn8Input,		NTV2_WgtDualLinkV2In8));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkIn8DSInput,		NTV2_WgtDualLinkV2In8));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkOut1Input,		NTV2_WgtDualLinkV2Out1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkOut2Input,		NTV2_WgtDualLinkV2Out2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkOut3Input,		NTV2_WgtDualLinkV2Out3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkOut4Input,		NTV2_WgtDualLinkV2Out4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkOut5Input,		NTV2_WgtDualLinkV2Out5));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkOut6Input,		NTV2_WgtDualLinkV2Out6));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkOut7Input,		NTV2_WgtDualLinkV2Out7));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptDualLinkOut8Input,		NTV2_WgtDualLinkV2Out8));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptFrameBuffer1Input,		NTV2_WgtFrameBuffer1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptFrameBuffer1BInput,		NTV2_WgtFrameBuffer1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptFrameBuffer2Input,		NTV2_WgtFrameBuffer2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptFrameBuffer2BInput,		NTV2_WgtFrameBuffer2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptFrameBuffer3Input,		NTV2_WgtFrameBuffer3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptFrameBuffer3BInput,		NTV2_WgtFrameBuffer3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptFrameBuffer4Input,		NTV2_WgtFrameBuffer4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptFrameBuffer4BInput,		NTV2_WgtFrameBuffer4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptFrameBuffer5Input,		NTV2_WgtFrameBuffer5));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptFrameBuffer5BInput,		NTV2_WgtFrameBuffer5));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptFrameBuffer6Input,		NTV2_WgtFrameBuffer6));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptFrameBuffer6BInput,		NTV2_WgtFrameBuffer6));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptFrameBuffer7Input,		NTV2_WgtFrameBuffer7));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptFrameBuffer7BInput,		NTV2_WgtFrameBuffer7));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptFrameBuffer8Input,		NTV2_WgtFrameBuffer8));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptFrameBuffer8BInput,		NTV2_WgtFrameBuffer8));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptLUT1Input,				NTV2_WgtLUT1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptLUT2Input,				NTV2_WgtLUT2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptLUT3Input,				NTV2_WgtLUT3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptLUT4Input,				NTV2_WgtLUT4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptLUT5Input,				NTV2_WgtLUT5));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptLUT6Input,				NTV2_WgtLUT6));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptLUT7Input,				NTV2_WgtLUT7));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptLUT8Input,				NTV2_WgtLUT8));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptMixer1BGKeyInput,		NTV2_WgtMixer1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptMixer1BGVidInput,		NTV2_WgtMixer1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptMixer1FGKeyInput,		NTV2_WgtMixer1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptMixer1FGVidInput,		NTV2_WgtMixer1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptMixer2BGKeyInput,		NTV2_WgtMixer2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptMixer2BGVidInput,		NTV2_WgtMixer2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptMixer2FGKeyInput,		NTV2_WgtMixer2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptMixer2FGVidInput,		NTV2_WgtMixer2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptMixer3BGKeyInput,		NTV2_WgtMixer3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptMixer3BGVidInput,		NTV2_WgtMixer3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptMixer3FGKeyInput,		NTV2_WgtMixer3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptMixer3FGVidInput,		NTV2_WgtMixer3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptMixer4BGKeyInput,		NTV2_WgtMixer4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptMixer4BGVidInput,		NTV2_WgtMixer4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptMixer4FGKeyInput,		NTV2_WgtMixer4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptMixer4FGVidInput,		NTV2_WgtMixer4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptStereoLeftInput,			NTV2_WgtStereoCompressor));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptStereoRightInput,		NTV2_WgtStereoCompressor));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptWaterMarker1Input,		NTV2_WgtWaterMarker1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptWaterMarker2Input,		NTV2_WgtWaterMarker2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptFrameSync1Input,			NTV2_WgtFrameSync1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptFrameSync2Input,			NTV2_WgtFrameSync2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptCompressionModInput,		NTV2_WgtCompression1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptConversionModInput,		NTV2_WgtUpDownConverter1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptAnalogOutInput,			NTV2_WgtAnalogOut1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptAnalogOutCompositeOut,	NTV2_WgtAnalogCompositeOut1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut1Input,			NTV2_WgtSDIOut1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut1Input,			NTV2_Wgt3GSDIOut1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut1InputDS2,			NTV2_Wgt3GSDIOut1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut1Input,			NTV2_Wgt12GSDIOut1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut1InputDS2,			NTV2_Wgt12GSDIOut1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut2Input,			NTV2_WgtSDIOut2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut2Input,			NTV2_Wgt3GSDIOut2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut2InputDS2,			NTV2_Wgt3GSDIOut2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut2Input,			NTV2_Wgt12GSDIOut2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut2InputDS2,			NTV2_Wgt12GSDIOut2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut3Input,			NTV2_WgtSDIOut3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut3Input,			NTV2_Wgt3GSDIOut3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut3InputDS2,			NTV2_Wgt3GSDIOut3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut3Input,			NTV2_Wgt12GSDIOut3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut3InputDS2,			NTV2_Wgt12GSDIOut3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut4Input,			NTV2_WgtSDIOut4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut4Input,			NTV2_Wgt3GSDIOut4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut4InputDS2,			NTV2_Wgt3GSDIOut4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut4Input,			NTV2_Wgt12GSDIOut4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut4InputDS2,			NTV2_Wgt12GSDIOut4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut5Input,			NTV2_Wgt3GSDIOut5));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut5Input,			NTV2_WgtSDIMonOut1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut5InputDS2,			NTV2_WgtSDIMonOut1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut5InputDS2,			NTV2_Wgt3GSDIOut5));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut6Input,			NTV2_Wgt3GSDIOut6));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut6InputDS2,			NTV2_Wgt3GSDIOut6));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut7Input,			NTV2_Wgt3GSDIOut7));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut7InputDS2,			NTV2_Wgt3GSDIOut7));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut8Input,			NTV2_Wgt3GSDIOut8));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptSDIOut8InputDS2,			NTV2_Wgt3GSDIOut8));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptHDMIOutInput,			NTV2_WgtHDMIOut1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptHDMIOutInput,			NTV2_WgtHDMIOut1v2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptHDMIOutQ1Input,			NTV2_WgtHDMIOut1v2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptHDMIOutQ2Input,			NTV2_WgtHDMIOut1v2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptHDMIOutQ3Input,			NTV2_WgtHDMIOut1v2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptHDMIOutQ4Input,			NTV2_WgtHDMIOut1v2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptHDMIOutInput,			NTV2_WgtHDMIOut1v3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptHDMIOutQ1Input,			NTV2_WgtHDMIOut1v3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptHDMIOutQ2Input,			NTV2_WgtHDMIOut1v3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptHDMIOutQ3Input,			NTV2_WgtHDMIOut1v3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptHDMIOutQ4Input,			NTV2_WgtHDMIOut1v3));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptHDMIOutInput,			NTV2_WgtHDMIOut1v4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptHDMIOutQ1Input,			NTV2_WgtHDMIOut1v4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptHDMIOutQ2Input,			NTV2_WgtHDMIOut1v4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptHDMIOutQ3Input,			NTV2_WgtHDMIOut1v4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptHDMIOutQ4Input,			NTV2_WgtHDMIOut1v4));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptHDMIOutInput,			NTV2_WgtHDMIOut1v5));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_Xpt3DLUT1Input,				NTV2_Wgt3DLUT1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptMultiLinkOut1Input,		NTV2_WgtMultiLinkOut1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptMultiLinkOut1InputDS2,	NTV2_WgtMultiLinkOut1));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptMultiLinkOut2Input,		NTV2_WgtMultiLinkOut2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptMultiLinkOut2InputDS2,	NTV2_WgtMultiLinkOut2));
	gInputXpt2WidgetIDs.insert (InputXpt2WidgetIDPair (NTV2_XptOEInput,					NTV2_WgtOE1));

	//	gWidget2InputXpts
	for (InputXpt2WidgetIDsConstIter iter (gInputXpt2WidgetIDs.begin ());  iter != gInputXpt2WidgetIDs.end ();  ++iter)
		gWidget2InputXpts.insert (Widget2InputXptPair (iter->second, iter->first));

	//	gRGBOnlyInputXpts
	gRGBOnlyInputXpts.insert (NTV2_XptLUT1Input);
	gRGBOnlyInputXpts.insert (NTV2_XptLUT2Input);
	gRGBOnlyInputXpts.insert (NTV2_XptLUT3Input);
	gRGBOnlyInputXpts.insert (NTV2_XptLUT4Input);
	gRGBOnlyInputXpts.insert (NTV2_XptLUT5Input);
	gRGBOnlyInputXpts.insert (NTV2_XptLUT6Input);
	gRGBOnlyInputXpts.insert (NTV2_XptLUT7Input);
	gRGBOnlyInputXpts.insert (NTV2_XptLUT8Input);
	gRGBOnlyInputXpts.insert (NTV2_XptDualLinkOut1Input);
	gRGBOnlyInputXpts.insert (NTV2_XptDualLinkOut2Input);
	gRGBOnlyInputXpts.insert (NTV2_XptDualLinkOut3Input);
	gRGBOnlyInputXpts.insert (NTV2_XptDualLinkOut4Input);
	gRGBOnlyInputXpts.insert (NTV2_XptDualLinkOut5Input);
	gRGBOnlyInputXpts.insert (NTV2_XptDualLinkOut6Input);
	gRGBOnlyInputXpts.insert (NTV2_XptDualLinkOut7Input);
	gRGBOnlyInputXpts.insert (NTV2_XptDualLinkOut8Input);

	//	gYUVOnlyInputXpts
	gYUVOnlyInputXpts.insert (NTV2_XptMixer1BGKeyInput);
	gYUVOnlyInputXpts.insert (NTV2_XptMixer1BGVidInput);
	gYUVOnlyInputXpts.insert (NTV2_XptMixer1FGKeyInput);
	gYUVOnlyInputXpts.insert (NTV2_XptMixer1FGVidInput);
	gYUVOnlyInputXpts.insert (NTV2_XptMixer2BGKeyInput);
	gYUVOnlyInputXpts.insert (NTV2_XptMixer2BGVidInput);
	gYUVOnlyInputXpts.insert (NTV2_XptMixer2FGKeyInput);
	gYUVOnlyInputXpts.insert (NTV2_XptMixer2FGVidInput);
	gYUVOnlyInputXpts.insert (NTV2_XptMixer3BGKeyInput);
	gYUVOnlyInputXpts.insert (NTV2_XptMixer3BGVidInput);
	gYUVOnlyInputXpts.insert (NTV2_XptMixer3FGKeyInput);
	gYUVOnlyInputXpts.insert (NTV2_XptMixer3FGVidInput);
	gYUVOnlyInputXpts.insert (NTV2_XptMixer4BGKeyInput);
	gYUVOnlyInputXpts.insert (NTV2_XptMixer4BGVidInput);
	gYUVOnlyInputXpts.insert (NTV2_XptMixer4FGKeyInput);
	gYUVOnlyInputXpts.insert (NTV2_XptMixer4FGVidInput);
	gYUVOnlyInputXpts.insert (NTV2_XptDualLinkIn1Input);
	gYUVOnlyInputXpts.insert (NTV2_XptDualLinkIn1DSInput);
	gYUVOnlyInputXpts.insert (NTV2_XptDualLinkIn2Input);
	gYUVOnlyInputXpts.insert (NTV2_XptDualLinkIn2DSInput);
	gYUVOnlyInputXpts.insert (NTV2_XptDualLinkIn3Input);
	gYUVOnlyInputXpts.insert (NTV2_XptDualLinkIn3DSInput);
	gYUVOnlyInputXpts.insert (NTV2_XptDualLinkIn4Input);
	gYUVOnlyInputXpts.insert (NTV2_XptDualLinkIn4DSInput);
	gYUVOnlyInputXpts.insert (NTV2_XptDualLinkIn5Input);
	gYUVOnlyInputXpts.insert (NTV2_XptDualLinkIn5DSInput);
	gYUVOnlyInputXpts.insert (NTV2_XptDualLinkIn6Input);
	gYUVOnlyInputXpts.insert (NTV2_XptDualLinkIn6DSInput);
	gYUVOnlyInputXpts.insert (NTV2_XptDualLinkIn7Input);
	gYUVOnlyInputXpts.insert (NTV2_XptDualLinkIn7DSInput);
	gYUVOnlyInputXpts.insert (NTV2_XptDualLinkIn8Input);
	gYUVOnlyInputXpts.insert (NTV2_XptDualLinkIn8DSInput);
	gYUVOnlyInputXpts.insert (NTV2_XptConversionModInput);
	gYUVOnlyInputXpts.insert (NTV2_XptAnalogOutInput);
	gYUVOnlyInputXpts.insert (NTV2_XptAnalogOutCompositeOut);

	gKeyInputXpts.insert (NTV2_XptCSC1KeyInput);
	gKeyInputXpts.insert (NTV2_XptCSC2KeyInput);
	gKeyInputXpts.insert (NTV2_XptCSC3KeyInput);
	gKeyInputXpts.insert (NTV2_XptCSC4KeyInput);
	gKeyInputXpts.insert (NTV2_XptCSC5KeyInput);
	gKeyInputXpts.insert (NTV2_XptCSC6KeyInput);
	gKeyInputXpts.insert (NTV2_XptCSC7KeyInput);
	gKeyInputXpts.insert (NTV2_XptCSC8KeyInput);
	gKeyInputXpts.insert (NTV2_XptMixer1BGKeyInput);
	gKeyInputXpts.insert (NTV2_XptMixer1FGKeyInput);
	gKeyInputXpts.insert (NTV2_XptMixer2BGKeyInput);
	gKeyInputXpts.insert (NTV2_XptMixer2FGKeyInput);
	gKeyInputXpts.insert (NTV2_XptMixer3BGKeyInput);
	gKeyInputXpts.insert (NTV2_XptMixer3FGKeyInput);
	gKeyInputXpts.insert (NTV2_XptMixer4BGKeyInput);
	gKeyInputXpts.insert (NTV2_XptMixer4FGKeyInput);
	gKeyInputXpts.insert (NTV2_XptCSC1KeyFromInput2);

}

void RoutingExpert::InitOutputXpt2WidgetIDs(void)
{
	//	gOutputXpt2WidgetIDs
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptAnalogIn,				NTV2_WgtAnalogIn1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptTestPatternYUV,		NTV2_WgtTestPattern1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptConversionModule,		NTV2_WgtUpDownConverter1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn1,				NTV2_WgtSDIIn1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn1,				NTV2_Wgt3GSDIIn1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn1DS2,				NTV2_Wgt3GSDIIn1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn1,				NTV2_Wgt12GSDIIn1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn1DS2,				NTV2_Wgt12GSDIIn1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn2,				NTV2_WgtSDIIn2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn2,				NTV2_Wgt3GSDIIn2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn2DS2,				NTV2_Wgt3GSDIIn2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn2,				NTV2_Wgt12GSDIIn2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn2DS2,				NTV2_Wgt12GSDIIn2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn3,				NTV2_Wgt3GSDIIn3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn3DS2,				NTV2_Wgt3GSDIIn3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn3,				NTV2_Wgt12GSDIIn3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn3DS2,				NTV2_Wgt12GSDIIn3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn4,				NTV2_Wgt3GSDIIn4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn4DS2,				NTV2_Wgt3GSDIIn4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn4,				NTV2_Wgt12GSDIIn4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn4DS2,				NTV2_Wgt12GSDIIn4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn5,				NTV2_Wgt3GSDIIn5));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn5DS2,				NTV2_Wgt3GSDIIn5));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn6,				NTV2_Wgt3GSDIIn6));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn6DS2,				NTV2_Wgt3GSDIIn6));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn7,				NTV2_Wgt3GSDIIn7));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn7DS2,				NTV2_Wgt3GSDIIn7));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn8,				NTV2_Wgt3GSDIIn8));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptSDIIn8DS2,				NTV2_Wgt3GSDIIn8));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn1,				NTV2_WgtHDMIIn1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn1,				NTV2_WgtHDMIIn1v2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn1,				NTV2_WgtHDMIIn1v3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn1Q2,				NTV2_WgtHDMIIn1v3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn1Q2RGB,			NTV2_WgtHDMIIn1v3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn1Q3,				NTV2_WgtHDMIIn1v3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn1Q3RGB,			NTV2_WgtHDMIIn1v3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn1Q4,				NTV2_WgtHDMIIn1v3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn1Q4RGB,			NTV2_WgtHDMIIn1v3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn1RGB,			NTV2_WgtHDMIIn1v3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn1,				NTV2_WgtHDMIIn1v4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn1Q2,				NTV2_WgtHDMIIn1v4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn1Q2RGB,			NTV2_WgtHDMIIn1v4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn1Q3,				NTV2_WgtHDMIIn1v4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn1Q3RGB,			NTV2_WgtHDMIIn1v4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn1Q4,				NTV2_WgtHDMIIn1v4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn1Q4RGB,			NTV2_WgtHDMIIn1v4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn1RGB,			NTV2_WgtHDMIIn1v4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn2,				NTV2_WgtHDMIIn2v4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn2Q2,				NTV2_WgtHDMIIn2v4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn2Q2RGB,			NTV2_WgtHDMIIn2v4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn2Q3,				NTV2_WgtHDMIIn2v4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn2Q3RGB,			NTV2_WgtHDMIIn2v4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn2Q4,				NTV2_WgtHDMIIn2v4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn2Q4RGB,			NTV2_WgtHDMIIn2v4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn2RGB,			NTV2_WgtHDMIIn2v4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn3,				NTV2_WgtHDMIIn3v4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn3RGB,			NTV2_WgtHDMIIn3v4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn4,				NTV2_WgtHDMIIn4v4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptHDMIIn4RGB,			NTV2_WgtHDMIIn4v4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_Xpt425Mux1ARGB,			NTV2_Wgt425Mux1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_Xpt425Mux1AYUV,			NTV2_Wgt425Mux1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_Xpt425Mux1BRGB,			NTV2_Wgt425Mux1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_Xpt425Mux1BYUV,			NTV2_Wgt425Mux1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_Xpt425Mux2ARGB,			NTV2_Wgt425Mux2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_Xpt425Mux2AYUV,			NTV2_Wgt425Mux2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_Xpt425Mux2BRGB,			NTV2_Wgt425Mux2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_Xpt425Mux2BYUV,			NTV2_Wgt425Mux2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_Xpt425Mux3ARGB,			NTV2_Wgt425Mux3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_Xpt425Mux3AYUV,			NTV2_Wgt425Mux3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_Xpt425Mux3BRGB,			NTV2_Wgt425Mux3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_Xpt425Mux3BYUV,			NTV2_Wgt425Mux3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_Xpt425Mux4ARGB,			NTV2_Wgt425Mux4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_Xpt425Mux4AYUV,			NTV2_Wgt425Mux4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_Xpt425Mux4BRGB,			NTV2_Wgt425Mux4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_Xpt425Mux4BYUV,			NTV2_Wgt425Mux4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_Xpt4KDownConverterOut,	NTV2_Wgt4KDownConverter));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_Xpt4KDownConverterOutRGB,	NTV2_Wgt4KDownConverter));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC1KeyYUV,			NTV2_WgtCSC1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC1VidRGB,			NTV2_WgtCSC1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC1VidYUV,			NTV2_WgtCSC1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC2KeyYUV,			NTV2_WgtCSC2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC2VidRGB,			NTV2_WgtCSC2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC2VidYUV,			NTV2_WgtCSC2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC3KeyYUV,			NTV2_WgtCSC3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC3VidRGB,			NTV2_WgtCSC3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC3VidYUV,			NTV2_WgtCSC3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC4KeyYUV,			NTV2_WgtCSC4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC4VidRGB,			NTV2_WgtCSC4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC4VidYUV,			NTV2_WgtCSC4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC5KeyYUV,			NTV2_WgtCSC5));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC5VidRGB,			NTV2_WgtCSC5));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC5VidYUV,			NTV2_WgtCSC5));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC6KeyYUV,			NTV2_WgtCSC6));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC6VidRGB,			NTV2_WgtCSC6));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC6VidYUV,			NTV2_WgtCSC6));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC7KeyYUV,			NTV2_WgtCSC7));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC7VidRGB,			NTV2_WgtCSC7));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC7VidYUV,			NTV2_WgtCSC7));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC8KeyYUV,			NTV2_WgtCSC8));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC8VidRGB,			NTV2_WgtCSC8));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCSC8VidYUV,			NTV2_WgtCSC8));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkIn1,			NTV2_WgtDualLinkV2In1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkIn2,			NTV2_WgtDualLinkV2In2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkIn3,			NTV2_WgtDualLinkV2In3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkIn4,			NTV2_WgtDualLinkV2In4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkIn5,			NTV2_WgtDualLinkV2In5));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkIn6,			NTV2_WgtDualLinkV2In6));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkIn7,			NTV2_WgtDualLinkV2In7));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkIn8,			NTV2_WgtDualLinkV2In8));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkOut1,			NTV2_WgtDualLinkV2Out1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkOut1DS2,		NTV2_WgtDualLinkV2Out1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkOut2,			NTV2_WgtDualLinkV2Out2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkOut2DS2,		NTV2_WgtDualLinkV2Out2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkOut3,			NTV2_WgtDualLinkV2Out3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkOut3DS2,		NTV2_WgtDualLinkV2Out3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkOut4,			NTV2_WgtDualLinkV2Out4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkOut4DS2,		NTV2_WgtDualLinkV2Out4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkOut5,			NTV2_WgtDualLinkV2Out5));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkOut5DS2,		NTV2_WgtDualLinkV2Out5));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkOut6,			NTV2_WgtDualLinkV2Out6));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkOut6DS2,		NTV2_WgtDualLinkV2Out6));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkOut7,			NTV2_WgtDualLinkV2Out7));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkOut7DS2,		NTV2_WgtDualLinkV2Out7));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkOut8,			NTV2_WgtDualLinkV2Out8));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptDuallinkOut8DS2,		NTV2_WgtDualLinkV2Out8));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer1_DS2RGB,	NTV2_WgtFrameBuffer1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer1_DS2YUV,	NTV2_WgtFrameBuffer1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer1RGB,		NTV2_WgtFrameBuffer1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer1YUV,		NTV2_WgtFrameBuffer1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer2_DS2RGB,	NTV2_WgtFrameBuffer2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer2_DS2YUV,	NTV2_WgtFrameBuffer2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer2RGB,		NTV2_WgtFrameBuffer2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer2YUV,		NTV2_WgtFrameBuffer2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer3_DS2RGB,	NTV2_WgtFrameBuffer3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer3_DS2YUV,	NTV2_WgtFrameBuffer3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer3RGB,		NTV2_WgtFrameBuffer3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer3YUV,		NTV2_WgtFrameBuffer3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer4_DS2RGB,	NTV2_WgtFrameBuffer4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer4_DS2YUV,	NTV2_WgtFrameBuffer4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer4RGB,		NTV2_WgtFrameBuffer4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer4YUV,		NTV2_WgtFrameBuffer4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer5_DS2RGB,	NTV2_WgtFrameBuffer5));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer5_DS2YUV,	NTV2_WgtFrameBuffer5));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer5RGB,		NTV2_WgtFrameBuffer5));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer5YUV,		NTV2_WgtFrameBuffer5));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer6_DS2RGB,	NTV2_WgtFrameBuffer6));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer6_DS2YUV,	NTV2_WgtFrameBuffer6));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer6RGB,		NTV2_WgtFrameBuffer6));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer6YUV,		NTV2_WgtFrameBuffer6));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer7_DS2RGB,	NTV2_WgtFrameBuffer7));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer7_DS2YUV,	NTV2_WgtFrameBuffer7));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer7RGB,		NTV2_WgtFrameBuffer7));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer7YUV,		NTV2_WgtFrameBuffer7));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer8_DS2RGB,	NTV2_WgtFrameBuffer8));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer8_DS2YUV,	NTV2_WgtFrameBuffer8));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer8RGB,		NTV2_WgtFrameBuffer8));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameBuffer8YUV,		NTV2_WgtFrameBuffer8));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptLUT1Out,				NTV2_WgtLUT1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptLUT2Out,				NTV2_WgtLUT2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptLUT3Out,				NTV2_WgtLUT3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptLUT4Out,				NTV2_WgtLUT4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptLUT5Out,				NTV2_WgtLUT5));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptLUT6Out,				NTV2_WgtLUT6));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptLUT7Out,				NTV2_WgtLUT7));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptLUT8Out,				NTV2_WgtLUT8));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptMixer1KeyYUV,			NTV2_WgtMixer1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptMixer1VidYUV,			NTV2_WgtMixer1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptMixer1VidRGB,			NTV2_WgtMixer1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptMixer2KeyYUV,			NTV2_WgtMixer2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptMixer2VidYUV,			NTV2_WgtMixer2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptMixer3KeyYUV,			NTV2_WgtMixer3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptMixer3VidYUV,			NTV2_WgtMixer3));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptMixer4KeyYUV,			NTV2_WgtMixer4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptMixer4VidYUV,			NTV2_WgtMixer4));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptIICTRGB,				NTV2_WgtIICT1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptIICT2RGB,				NTV2_WgtIICT2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptStereoCompressorOut,	NTV2_WgtStereoCompressor));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameSync1RGB,			NTV2_WgtFrameSync1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameSync1YUV,			NTV2_WgtFrameSync1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameSync2RGB,			NTV2_WgtFrameSync2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptFrameSync2YUV,			NTV2_WgtFrameSync2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptCompressionModule,		NTV2_WgtCompression1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptMultiLinkOut1DS1,		NTV2_WgtMultiLinkOut1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptMultiLinkOut1DS2,		NTV2_WgtMultiLinkOut1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptMultiLinkOut1DS3,		NTV2_WgtMultiLinkOut1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptMultiLinkOut1DS4,		NTV2_WgtMultiLinkOut1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptMultiLinkOut2DS1,		NTV2_WgtMultiLinkOut2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptMultiLinkOut2DS2,		NTV2_WgtMultiLinkOut2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptMultiLinkOut2DS3,		NTV2_WgtMultiLinkOut2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptMultiLinkOut2DS4,		NTV2_WgtMultiLinkOut2));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_Xpt3DLUT1YUV,				NTV2_Wgt3DLUT1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_Xpt3DLUT1RGB,				NTV2_Wgt3DLUT1));
	gOutputXpt2WidgetIDs.insert (OutputXpt2WidgetIDPair (NTV2_XptOEOutRGB,				NTV2_WgtOE1));

	//	gWidget2OutputXpts
	for (OutputXpt2WidgetIDsConstIter iter (gOutputXpt2WidgetIDs.begin ());  iter != gOutputXpt2WidgetIDs.end ();  ++iter)
		gWidget2OutputXpts.insert (Widget2OutputXptPair (iter->second, iter->first));
}

void RoutingExpert::InitWidgetIDToChannels(void)
{
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtFrameBuffer1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtFrameBuffer2, NTV2_CHANNEL2));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtFrameBuffer3, NTV2_CHANNEL3));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtFrameBuffer4, NTV2_CHANNEL4));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtCSC1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtCSC2, NTV2_CHANNEL2));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtLUT1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtLUT2, NTV2_CHANNEL2));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtFrameSync1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtFrameSync2, NTV2_CHANNEL2));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtSDIIn1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtSDIIn2, NTV2_CHANNEL2));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt3GSDIIn1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt3GSDIIn2, NTV2_CHANNEL2));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt3GSDIIn3, NTV2_CHANNEL3));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt3GSDIIn4, NTV2_CHANNEL4));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtSDIOut1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtSDIOut2, NTV2_CHANNEL2));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtSDIOut3, NTV2_CHANNEL3));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtSDIOut4, NTV2_CHANNEL4));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt3GSDIOut1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt3GSDIOut2, NTV2_CHANNEL2));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt3GSDIOut3, NTV2_CHANNEL3));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt3GSDIOut4, NTV2_CHANNEL4));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtDualLinkIn1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtDualLinkV2In1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtDualLinkV2In2, NTV2_CHANNEL2));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtDualLinkOut1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtDualLinkOut2, NTV2_CHANNEL2));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtDualLinkV2Out1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtDualLinkV2Out2, NTV2_CHANNEL2));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtAnalogIn1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtAnalogOut1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtAnalogCompositeOut1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtHDMIIn1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtHDMIOut1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtUpDownConverter1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtUpDownConverter2, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtMixer1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtCompression1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtProcAmp1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtWaterMarker1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtWaterMarker2, NTV2_CHANNEL2));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtIICT1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtIICT2, NTV2_CHANNEL2));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtTestPattern1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtGenLock, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtDCIMixer1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtMixer2, NTV2_CHANNEL2));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtStereoCompressor, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtLUT3, NTV2_CHANNEL3));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtLUT4, NTV2_CHANNEL4));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtDualLinkV2In3, NTV2_CHANNEL3));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtDualLinkV2In4, NTV2_CHANNEL4));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtDualLinkV2Out3, NTV2_CHANNEL3));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtDualLinkV2Out4, NTV2_CHANNEL4));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtCSC3, NTV2_CHANNEL3));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtCSC4, NTV2_CHANNEL4));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtHDMIIn1v2, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtHDMIOut1v2, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtSDIMonOut1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtCSC5, NTV2_CHANNEL5));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtLUT5, NTV2_CHANNEL5));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtDualLinkV2Out5, NTV2_CHANNEL5));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt4KDownConverter, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt3GSDIIn5, NTV2_CHANNEL5));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt3GSDIIn6, NTV2_CHANNEL6));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt3GSDIIn7, NTV2_CHANNEL7));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt3GSDIIn8, NTV2_CHANNEL8));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt3GSDIOut5, NTV2_CHANNEL5));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt3GSDIOut6, NTV2_CHANNEL6));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt3GSDIOut7, NTV2_CHANNEL7));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt3GSDIOut8, NTV2_CHANNEL8));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtDualLinkV2In5, NTV2_CHANNEL5));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtDualLinkV2In6, NTV2_CHANNEL6));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtDualLinkV2In7, NTV2_CHANNEL7));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtDualLinkV2In8, NTV2_CHANNEL8));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtDualLinkV2Out6, NTV2_CHANNEL6));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtDualLinkV2Out7, NTV2_CHANNEL7));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtDualLinkV2Out8, NTV2_CHANNEL8));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtCSC6, NTV2_CHANNEL6));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtCSC7, NTV2_CHANNEL7));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtCSC8, NTV2_CHANNEL8));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtLUT6, NTV2_CHANNEL6));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtLUT7, NTV2_CHANNEL7));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtLUT8, NTV2_CHANNEL8));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtMixer3, NTV2_CHANNEL3));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtMixer4, NTV2_CHANNEL4));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtFrameBuffer5, NTV2_CHANNEL5));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtFrameBuffer6, NTV2_CHANNEL6));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtFrameBuffer7, NTV2_CHANNEL7));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtFrameBuffer8, NTV2_CHANNEL8));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtHDMIIn1v3, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtHDMIOut1v3, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt425Mux1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt425Mux2, NTV2_CHANNEL2));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt425Mux3, NTV2_CHANNEL3));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt425Mux4, NTV2_CHANNEL4));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt12GSDIIn1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt12GSDIIn2, NTV2_CHANNEL2));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt12GSDIIn3, NTV2_CHANNEL3));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt12GSDIIn4, NTV2_CHANNEL4));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt12GSDIOut1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt12GSDIOut2, NTV2_CHANNEL2));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt12GSDIOut3, NTV2_CHANNEL3));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt12GSDIOut4, NTV2_CHANNEL4));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtHDMIIn1v4, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtHDMIIn2v4, NTV2_CHANNEL2));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtHDMIIn3v4, NTV2_CHANNEL3));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtHDMIIn4v4, NTV2_CHANNEL4));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtHDMIOut1v4, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtHDMIOut1v5, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtMultiLinkOut1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_Wgt3DLUT1, NTV2_CHANNEL1));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtMultiLinkOut2, NTV2_CHANNEL2));
	gWidget2Channels.insert(Widget2ChannelPair(NTV2_WgtOE1, NTV2_CHANNEL1));
}

void RoutingExpert::InitWidgetIDToWidgetTypes(void)
{
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtFrameBuffer1, NTV2WidgetType_FrameStore));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtFrameBuffer2, NTV2WidgetType_FrameStore));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtFrameBuffer3, NTV2WidgetType_FrameStore));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtFrameBuffer4, NTV2WidgetType_FrameStore));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtCSC1, NTV2WidgetType_CSC));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtCSC2, NTV2WidgetType_CSC));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtLUT1, NTV2WidgetType_LUT));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtLUT2, NTV2WidgetType_LUT));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtFrameSync1, NTV2WidgetType_FrameSync));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtFrameSync2, NTV2WidgetType_FrameSync));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtSDIIn1, NTV2WidgetType_SDIIn));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtSDIIn2, NTV2WidgetType_SDIIn));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt3GSDIIn1, NTV2WidgetType_SDIIn3G));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt3GSDIIn2, NTV2WidgetType_SDIIn3G));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt3GSDIIn3, NTV2WidgetType_SDIIn3G));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt3GSDIIn4, NTV2WidgetType_SDIIn3G));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtSDIOut1, NTV2WidgetType_SDIOut));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtSDIOut2, NTV2WidgetType_SDIOut));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtSDIOut3, NTV2WidgetType_SDIOut));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtSDIOut4, NTV2WidgetType_SDIOut));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt3GSDIOut1, NTV2WidgetType_SDIOut3G));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt3GSDIOut2, NTV2WidgetType_SDIOut3G));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt3GSDIOut3, NTV2WidgetType_SDIOut3G));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt3GSDIOut4, NTV2WidgetType_SDIOut3G));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtDualLinkIn1, NTV2WidgetType_DualLinkV1In));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtDualLinkV2In1, NTV2WidgetType_DualLinkV2In));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtDualLinkV2In2, NTV2WidgetType_DualLinkV2In));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtDualLinkOut1, NTV2WidgetType_DualLinkV1Out));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtDualLinkOut2, NTV2WidgetType_DualLinkV1Out));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtDualLinkV2Out1, NTV2WidgetType_DualLinkV2Out));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtDualLinkV2Out2, NTV2WidgetType_DualLinkV2Out));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtAnalogIn1, NTV2WidgetType_AnalogIn));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtAnalogOut1, NTV2WidgetType_AnalogOut));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtAnalogCompositeOut1, NTV2WidgetType_AnalogCompositeOut));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtHDMIIn1, NTV2WidgetType_HDMIInV1));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtHDMIOut1, NTV2WidgetType_HDMIOutV1));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtUpDownConverter1, NTV2WidgetType_UpDownConverter));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtUpDownConverter2, NTV2WidgetType_UpDownConverter));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtMixer1, NTV2WidgetType_Mixer));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtCompression1, NTV2WidgetType_Compression));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtProcAmp1, NTV2WidgetType_ProcAmp));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtWaterMarker1, NTV2WidgetType_WaterMarker));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtWaterMarker2, NTV2WidgetType_WaterMarker));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtIICT1, NTV2WidgetType_IICT));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtIICT2, NTV2WidgetType_IICT));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtTestPattern1, NTV2WidgetType_TestPattern));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtGenLock, NTV2WidgetType_GenLock));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtDCIMixer1, NTV2WidgetType_DCIMixer));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtMixer2, NTV2WidgetType_Mixer));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtStereoCompressor, NTV2WidgetType_StereoCompressor));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtLUT3, NTV2WidgetType_LUT));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtLUT4, NTV2WidgetType_LUT));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtDualLinkV2In3, NTV2WidgetType_DualLinkV2In));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtDualLinkV2In4, NTV2WidgetType_DualLinkV2In));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtDualLinkV2Out3, NTV2WidgetType_DualLinkV2Out));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtDualLinkV2Out4, NTV2WidgetType_DualLinkV2Out));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtCSC3, NTV2WidgetType_CSC));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtCSC4, NTV2WidgetType_CSC));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtHDMIIn1v2, NTV2WidgetType_HDMIInV2));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtHDMIOut1v2, NTV2WidgetType_HDMIOutV2));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtSDIMonOut1, NTV2WidgetType_SDIMonOut));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtCSC5, NTV2WidgetType_CSC));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtLUT5, NTV2WidgetType_LUT));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtDualLinkV2Out5, NTV2WidgetType_DualLinkV2Out));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt4KDownConverter, NTV2WidgetType_4KDownConverter));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt3GSDIIn5, NTV2WidgetType_SDIIn3G));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt3GSDIIn6, NTV2WidgetType_SDIIn3G));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt3GSDIIn7, NTV2WidgetType_SDIIn3G));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt3GSDIIn8, NTV2WidgetType_SDIIn3G));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt3GSDIOut5, NTV2WidgetType_SDIOut3G));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt3GSDIOut6, NTV2WidgetType_SDIOut3G));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt3GSDIOut7, NTV2WidgetType_SDIOut3G));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt3GSDIOut8, NTV2WidgetType_SDIOut3G));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtDualLinkV2In5, NTV2WidgetType_DualLinkV2In));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtDualLinkV2In6, NTV2WidgetType_DualLinkV2In));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtDualLinkV2In7, NTV2WidgetType_DualLinkV2In));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtDualLinkV2In8, NTV2WidgetType_DualLinkV2In));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtDualLinkV2Out6, NTV2WidgetType_DualLinkV2Out));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtDualLinkV2Out7, NTV2WidgetType_DualLinkV2Out));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtDualLinkV2Out8, NTV2WidgetType_DualLinkV2Out));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtCSC6, NTV2WidgetType_CSC));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtCSC7, NTV2WidgetType_CSC));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtCSC8, NTV2WidgetType_CSC));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtLUT6, NTV2WidgetType_LUT));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtLUT7, NTV2WidgetType_LUT));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtLUT8, NTV2WidgetType_LUT));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtMixer3, NTV2WidgetType_Mixer));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtMixer4, NTV2WidgetType_Mixer));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtFrameBuffer5, NTV2WidgetType_FrameStore));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtFrameBuffer6, NTV2WidgetType_FrameStore));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtFrameBuffer7, NTV2WidgetType_FrameStore));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtFrameBuffer8, NTV2WidgetType_FrameStore));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtHDMIIn1v3, NTV2WidgetType_HDMIInV3));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtHDMIOut1v3, NTV2WidgetType_HDMIOutV3));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt425Mux1, NTV2WidgetType_SMPTE425Mux));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt425Mux2, NTV2WidgetType_SMPTE425Mux));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt425Mux3, NTV2WidgetType_SMPTE425Mux));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt425Mux4, NTV2WidgetType_SMPTE425Mux));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt12GSDIIn1, NTV2WidgetType_SDIIn12G));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt12GSDIIn2, NTV2WidgetType_SDIIn12G));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt12GSDIIn3, NTV2WidgetType_SDIIn12G));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt12GSDIIn4, NTV2WidgetType_SDIIn12G));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt12GSDIOut1, NTV2WidgetType_SDIOut12G));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt12GSDIOut2, NTV2WidgetType_SDIOut12G));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt12GSDIOut3, NTV2WidgetType_SDIOut12G));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt12GSDIOut4, NTV2WidgetType_SDIOut12G));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtHDMIIn1v4, NTV2WidgetType_HDMIInV4));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtHDMIIn2v4, NTV2WidgetType_HDMIInV4));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtHDMIIn3v4, NTV2WidgetType_HDMIInV4));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtHDMIIn4v4, NTV2WidgetType_HDMIInV4));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtHDMIOut1v4, NTV2WidgetType_HDMIOutV4));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtHDMIOut1v5, NTV2WidgetType_HDMIOutV5));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtMultiLinkOut1, NTV2WidgetType_MultiLinkOut));
	gWidget2Types.insert(Widget2TypePair(NTV2_Wgt3DLUT1, NTV2WidgetType_LUT3D));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtMultiLinkOut2, NTV2WidgetType_MultiLinkOut));
	gWidget2Types.insert(Widget2TypePair(NTV2_WgtOE1, NTV2WidgetType_OE));

	// SDI Widget Types
	gSDIWidgetTypes.insert(NTV2WidgetType_SDIIn);
	gSDIWidgetTypes.insert(NTV2WidgetType_SDIOut);
	gSDIWidgetTypes.insert(NTV2WidgetType_SDIIn3G);
	gSDIWidgetTypes.insert(NTV2WidgetType_SDIOut3G);
	gSDIWidgetTypes.insert(NTV2WidgetType_SDIIn12G);
	gSDIWidgetTypes.insert(NTV2WidgetType_SDIOut12G);
	gSDIWidgetTypes.insert(NTV2WidgetType_SDIMonOut);
	// SDI Input Widget Types
	gSDIInWidgetTypes.insert(NTV2WidgetType_SDIIn);
	gSDIInWidgetTypes.insert(NTV2WidgetType_SDIIn3G);
	gSDIInWidgetTypes.insert(NTV2WidgetType_SDIIn12G);
	// SDI Output Widget Types
	gSDIOutWidgetTypes.insert(NTV2WidgetType_SDIOut);
	gSDIOutWidgetTypes.insert(NTV2WidgetType_SDIOut3G);
	gSDIOutWidgetTypes.insert(NTV2WidgetType_SDIOut12G);
	gSDIOutWidgetTypes.insert(NTV2WidgetType_SDIMonOut);
	// 3G SDI Widget Types
	gSDI3GWidgetTypes.insert(NTV2WidgetType_SDIIn3G);
	gSDI3GWidgetTypes.insert(NTV2WidgetType_SDIOut3G);
	// 12G SDI Widget Types
	gSDI12GWidgetTypes.insert(NTV2WidgetType_SDIIn12G);
	gSDI12GWidgetTypes.insert(NTV2WidgetType_SDIOut12G);
	// DualLink Widget Types
	gDualLinkWidgetTypes.insert(NTV2WidgetType_DualLinkV1In);
	gDualLinkWidgetTypes.insert(NTV2WidgetType_DualLinkV2In);
	gDualLinkWidgetTypes.insert(NTV2WidgetType_DualLinkV1Out);
	gDualLinkWidgetTypes.insert(NTV2WidgetType_DualLinkV2Out);
	// DualLink Input Widget Types
	gDualLinkInWidgetTypes.insert(NTV2WidgetType_DualLinkV1In);
	gDualLinkInWidgetTypes.insert(NTV2WidgetType_DualLinkV2In);
	// DualLink Output Widget Types
	gDualLinkOutWidgetTypes.insert(NTV2WidgetType_DualLinkV1Out);
	gDualLinkOutWidgetTypes.insert(NTV2WidgetType_DualLinkV2Out);
	// HDMI Widget Types
	gHDMIWidgetTypes.insert(NTV2WidgetType_HDMIInV1);
	gHDMIWidgetTypes.insert(NTV2WidgetType_HDMIInV2);
	gHDMIWidgetTypes.insert(NTV2WidgetType_HDMIInV3);
	gHDMIWidgetTypes.insert(NTV2WidgetType_HDMIInV4);
	gHDMIWidgetTypes.insert(NTV2WidgetType_HDMIOutV1);
	gHDMIWidgetTypes.insert(NTV2WidgetType_HDMIOutV2);
	gHDMIWidgetTypes.insert(NTV2WidgetType_HDMIOutV3);
	gHDMIWidgetTypes.insert(NTV2WidgetType_HDMIOutV4);
	gHDMIWidgetTypes.insert(NTV2WidgetType_HDMIOutV5);
	// HDMI Input Widget Types
	gHDMIInWidgetTypes.insert(NTV2WidgetType_HDMIInV1);
	gHDMIInWidgetTypes.insert(NTV2WidgetType_HDMIInV2);
	gHDMIInWidgetTypes.insert(NTV2WidgetType_HDMIInV3);
	gHDMIInWidgetTypes.insert(NTV2WidgetType_HDMIInV4);
	// HDMI Output Widget Types
	gHDMIOutWidgetTypes.insert(NTV2WidgetType_HDMIOutV1);
	gHDMIOutWidgetTypes.insert(NTV2WidgetType_HDMIOutV2);
	gHDMIOutWidgetTypes.insert(NTV2WidgetType_HDMIOutV3);
	gHDMIOutWidgetTypes.insert(NTV2WidgetType_HDMIOutV4);
	gHDMIOutWidgetTypes.insert(NTV2WidgetType_HDMIOutV5);
	// Analog Widget Types
	gAnalogWidgetTypes.insert(NTV2WidgetType_AnalogIn);
	gAnalogWidgetTypes.insert(NTV2WidgetType_AnalogOut);
	gAnalogWidgetTypes.insert(NTV2WidgetType_AnalogCompositeOut);
}
