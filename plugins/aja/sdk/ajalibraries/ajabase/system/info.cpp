/* SPDX-License-Identifier: MIT */
/**
    @file		info.cpp
    @brief		Implements the AJASystemInfo class.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

// include the system dependent implementation class
#if defined(AJA_WINDOWS)
    #include "ajabase/system/windows/infoimpl.h"
#endif
#if defined(AJA_LINUX)
    #include "ajabase/system/linux/infoimpl.h"
#endif
#if defined(AJA_MAC)
    #include "ajabase/system/mac/infoimpl.h"
#endif

#include "ajabase/system/info.h"
#include <cstring>
#include <iomanip>
#include <iostream>

using namespace std;


//	STATIC
string AJASystemInfo::ToString (const AJALabelValuePairs & inLabelValuePairs, const size_t inMaxValWidth, const size_t inGutterWidth)
{
	typedef std::vector<string>			ValueLines;
	typedef ValueLines::const_iterator	ValueLinesConstIter;
	const string	gutterStr	(inGutterWidth, ' ');

	//	Measure longest label length...
	//	BUGBUGBUG	Multi-byte UTF8 characters should only be counted as one character
    size_t longestLabelLen(0);
    for (AJALabelValuePairsConstIter it(inLabelValuePairs.begin());  it != inLabelValuePairs.end();  ++it)
        if (it->first.length() > longestLabelLen)
            longestLabelLen = it->first.length();
    longestLabelLen++;	//	Plus the ':'

	//	Iterate over everything again, this time "printing" the map's contents...
    ostringstream oss;
    for (AJALabelValuePairsConstIter it(inLabelValuePairs.begin());  it != inLabelValuePairs.end();  ++it)
    {
		static const string	lineBreakChars("\r\n");
        string label(it->first), value(it->second);
        const bool	hasLineBreaks (value.find_first_of(lineBreakChars) != string::npos);
		if (value.empty()  &&  it != inLabelValuePairs.begin())	//	Empty value string is a special case...
			oss << endl;	//	...don't append ':' and prepend an extra blank line
		else
			label += ":";

		if (!hasLineBreaks  &&  !inMaxValWidth)
		{	//	No wrapping or line-breaks -- just output the line...
			oss << setw(int(longestLabelLen)) << left << label << gutterStr << value << endl;
			continue;	//	...and move on to the next
		}

		//	Wrapping/line-breaking:
		ValueLines	valueLines, finalLines;
		if (hasLineBreaks)
		{
			static const string	lineBreakDelims[]	=	{"\r\n", "\r", "\n"};
			aja::replace(value, lineBreakDelims[0], lineBreakDelims[2]);	//	CRLF => LF
			aja::replace(value, lineBreakDelims[1], lineBreakDelims[2]);	//	CR => LF
			valueLines = aja::split(value, lineBreakDelims[2][0]);	//	Split on LF
		}
		else
			valueLines.push_back(value);
		if (inMaxValWidth)
			for (ValueLinesConstIter iter(valueLines.begin());  iter != valueLines.end();  ++iter)
			{
				const string &	lineStr(*iter);
				size_t	pos(0);
				do
				{
					finalLines.push_back(lineStr.substr(pos, inMaxValWidth));
					pos += inMaxValWidth;
				} while (pos < lineStr.length());
			}	//	for each valueLine
		else
			finalLines = valueLines;

		const string	wrapIndentStr	(longestLabelLen + inGutterWidth,  ' ');
		for (size_t ndx(0);  ndx < finalLines.size();  ndx++)
		{
			const string &	valStr(finalLines.at(ndx));
			if (ndx)
				oss << wrapIndentStr << valStr << endl;
			else
				oss << setw(int(longestLabelLen)) << left << label << gutterStr << valStr << endl;
		}
    }	//	for each label/value pair

    return oss.str();
}


AJASystemInfo::AJASystemInfo(AJASystemInfoMemoryUnit units, AJASystemInfoSections sections)
{
	// create the implementation class
    mpImpl = new AJASystemInfoImpl(units);

    Rescan(sections);
}

AJASystemInfo::~AJASystemInfo()
{
    if(mpImpl)
	{
        delete mpImpl;
        mpImpl = NULL;
	}
}

AJAStatus AJASystemInfo::Rescan (AJASystemInfoSections sections)
{
    AJAStatus ret = AJA_STATUS_FAIL;
    if(mpImpl)
    {
        // labels
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_System_Model)] = "System Model";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_System_Bios)] = "System BIOS";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_System_Name)] = "System Name";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_System_BootTime)] = "System Boot Time";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_OS_ProductName)] = "OS Product Name";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_OS_Version)] = "OS Version";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_OS_VersionBuild)] = "OS Build";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_OS_KernelVersion)] = "OS Kernel Version";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_CPU_Type)] = "CPU Type";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_CPU_NumCores)] = "CPU Num Cores";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_Mem_Total)] = "Memory Total";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_Mem_Used)] = "Memory Used";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_Mem_Free)] = "Memory Free";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_GPU_Type)] = "GPU Type";

        mpImpl->mLabelMap[int(AJA_SystemInfoTag_Path_UserHome)] = "User Home Path";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_Path_PersistenceStoreUser)] = "User Persistence Store Path";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_Path_PersistenceStoreSystem)] = "System Persistence Store Path";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_Path_Applications)] = "AJA Applications Path";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_Path_Utilities)] = "AJA Utilities Path";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_Path_Firmware)] = "AJA Firmware Path";

        ret = mpImpl->Rescan(sections);
    }

    return ret;
}

AJAStatus AJASystemInfo::GetValue (const AJASystemInfoTag tag, string & outValue) const
{
	outValue = "";
	if (mpImpl && mpImpl->mValueMap.count(int(tag)) != 0)
	{
		outValue = mpImpl->mValueMap[int(tag)];
		return AJA_STATUS_SUCCESS;
	}
	return AJA_STATUS_FAIL;
}

AJAStatus AJASystemInfo::GetLabel (const AJASystemInfoTag tag, string & outLabel) const
{
	outLabel = "";
	if (mpImpl && mpImpl->mLabelMap.count(int(tag)) != 0)
	{
		outLabel = mpImpl->mLabelMap[int(tag)];
		return AJA_STATUS_SUCCESS;
	}
	return AJA_STATUS_FAIL;
}

string AJASystemInfo::ToString (const size_t inValueWrapLen, const size_t inGutterWidth) const
{
	AJALabelValuePairs	infoTable;
	append(infoTable, "HOST INFO");
    for (AJASystemInfoTag tag(AJASystemInfoTag(0));  tag < AJA_SystemInfoTag_LAST;  tag = AJASystemInfoTag(tag+1))
    {
        string label, value;
        if (AJA_SUCCESS(GetLabel(tag, label)) && AJA_SUCCESS(GetValue(tag, value)))
			if (!label.empty())
				append(infoTable, label, value);
    }
	return ToString(infoTable, inValueWrapLen, inGutterWidth);
}

void AJASystemInfo::ToString(string& allLabelsAndValues) const
{
    allLabelsAndValues = ToString();
}

ostream & operator << (ostream & outStream, const AJASystemInfo & inData)
{
    outStream << inData.ToString();
    return outStream;
}

ostream & operator << (ostream & outStream, const AJALabelValuePair & inData)
{
	string			label(inData.first);
	const string &	value(inData.second);
	if (label.empty())
		return outStream;
	aja::strip(label);
	if (label.at(label.length()-1) == ':')
		label.resize(label.length()-1);
	aja::replace(label, " ", "_");
	outStream << label << "=" << value;
	return outStream;
}

ostream & operator << (ostream & outStream, const AJALabelValuePairs & inData)
{
	for (AJALabelValuePairsConstIter it(inData.begin());  it != inData.end();  )
	{
		outStream << *it;
		if (++it != inData.end())
			outStream << ", ";
	}
	return outStream;
}
