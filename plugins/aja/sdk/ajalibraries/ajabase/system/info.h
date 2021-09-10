/* SPDX-License-Identifier: MIT */
/**
    @file		info.h
    @brief		Declares the AJASystemInfo class.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_INFO_H
#define AJA_INFO_H

#include "ajabase/common/public.h"
#include <vector>
#include <map>
#include <string>

// forward declarations
class AJASystemInfoImpl;

enum AJASystemInfoMemoryUnit
{
    AJA_SystemInfoMemoryUnit_Bytes,
    AJA_SystemInfoMemoryUnit_Kilobytes,
    AJA_SystemInfoMemoryUnit_Megabytes,
    AJA_SystemInfoMemoryUnit_Gigabytes,

    AJA_SystemInfoMemoryUnit_LAST
};

enum AJASystemInfoTag
{
    AJA_SystemInfoTag_System_Model,
    AJA_SystemInfoTag_System_Bios,
    AJA_SystemInfoTag_System_Name,
    AJA_SystemInfoTag_System_BootTime,
    AJA_SystemInfoTag_OS_ProductName,
    AJA_SystemInfoTag_OS_Version,
    AJA_SystemInfoTag_OS_VersionBuild,
    AJA_SystemInfoTag_OS_KernelVersion,
    AJA_SystemInfoTag_CPU_Type,
    AJA_SystemInfoTag_CPU_NumCores,
    AJA_SystemInfoTag_Mem_Total,
    AJA_SystemInfoTag_Mem_Used,
    AJA_SystemInfoTag_Mem_Free,
    AJA_SystemInfoTag_GPU_Type,
    AJA_SystemInfoTag_Path_UserHome,
    AJA_SystemInfoTag_Path_PersistenceStoreUser,
    AJA_SystemInfoTag_Path_PersistenceStoreSystem,
    AJA_SystemInfoTag_Path_Applications,
    AJA_SystemInfoTag_Path_Utilities,
    AJA_SystemInfoTag_Path_Firmware,

    AJA_SystemInfoTag_LAST
};

enum AJASystemInfoSections
{
    AJA_SystemInfoSection_CPU    = 0x00000001 << 0,
    AJA_SystemInfoSection_GPU    = 0x00000001 << 1,
    AJA_SystemInfoSection_Mem    = 0x00000001 << 2,
    AJA_SystemInfoSection_OS     = 0x00000001 << 3,
    AJA_SystemInfoSection_Path   = 0x00000001 << 4,
    AJA_SystemInfoSection_System = 0x00000001 << 5,

    AJA_SystemInfoSection_None   = 0x00000000,
    AJA_SystemInfoSection_All    = 0xFFFFFFFF
};

typedef std::pair<std::string, std::string>		AJALabelValuePair;				///< @brief	A pair of strings comprising a label and a value
typedef std::vector<AJALabelValuePair>			AJALabelValuePairs;				///< @brief	An ordered sequence of label/value pairs
typedef AJALabelValuePairs::const_iterator		AJALabelValuePairsConstIter;

/**
 *	Class for getting common information about the system.
 *	@ingroup AJAGroupSystem
 *
 *
 */
class AJA_EXPORT AJASystemInfo
{
public:	//	Instance Methods
    /**
     *  @brief       Default constructor that instantiates the platform-specific implementation,
     *               then calls Rescan.
     *  @param[in]   inUnits     Optionally specifies the AJASystemInfoMemoryUnit to use.
     *                           Defaults to "megabytes".
     *  @param[in]   sections    The sections of the info structure to rescan, bitwise OR the desired sections
     *                           or use AJA_SystemInfoSection_All (the default) for all sections or
     *                           AJA_SystemInfoSection_None for no sections. In the case of AJA_SystemInfoSection_None
     *                           only the labels are set, no scanning is done at initialization.
     */
    AJASystemInfo (const AJASystemInfoMemoryUnit inUnits = AJA_SystemInfoMemoryUnit_Megabytes,
                   AJASystemInfoSections sections = AJA_SystemInfoSection_All);

    virtual ~AJASystemInfo();

    /**
     *	Rescans the host system.
     *  @param[in]   sections    The sections of the info structure to rescan, bitwise OR the desired sections
     *                           or use AJA_SystemInfoSection_All (the default) for all sections or
     *                           AJA_SystemInfoSection_None for no sections. In the case of AJA_SystemInfoSection_None
     *                           only the labels are set, no scanning is done.
     */
    virtual AJAStatus Rescan (AJASystemInfoSections sections = AJA_SystemInfoSection_All);
	
    /**
     *  @brief       Answers with the host system info value string for the given AJASystemInfoTag.
     *  @param[in]   inTag      The AJASystemInfoTag of interest
     *  @param[out]  outValue   Receives the value string
     *  @return      AJA_SUCCESS if successful
     */
    virtual AJAStatus GetValue(const AJASystemInfoTag inTag, std::string & outValue) const;
	
    /**
     *  @brief       Answers with the host system info label string for the given AJASystemInfoTag.
     *  @param[in]   inTag      The AJASystemInfoTag of interest
     *  @param[out]  outLabel   Receives the label string
     *  @return      AJA_SUCCESS if successful
     */
    virtual AJAStatus GetLabel(const AJASystemInfoTag inTag, std::string & outLabel) const;

    /**
     *  @brief       Answers with a multi-line string that contains the complete host system info table.
     *  @param[out]  outAllLabelsAndValues   Receives the string
     */
    virtual void ToString (std::string & outAllLabelsAndValues) const;

    /**
     *  @return      A multi-line string containing the complete host system info table.
     *  @param[in]   inValueWrapLen       Optionally specifies the maximum width of the "value" column,
     *                                    in characters. Zero, the default, disables wrapping.
     *                                    (However, wrapping always occurs on line-breaks (e.g. CRLF, LF, CR, etc.)).
     *  @param[in]   inGutterWidth        Optionally specifies the gap ("gutter") between the "label"
     *                                    and "value" columns, in character spaces. Defaults to 3 spaces.
     */
    virtual std::string ToString (const size_t inValueWrapLen = 0, const size_t inGutterWidth = 3) const;

public:	//	Class Methods
    /**
     *  @brief       Generates a multi-line string from a "table" of label/value pairs that, when displayed
     *               in a monospaced font, appears as a neat, two-column table.
     *  @param[in]   inLabelValuePairs    The AJALabelValuePairs to be formatted as a table.
     *  @param[in]   inValueWrapLen       Optionally specifies the maximum width of the "value" column,
     *                                    in characters. Zero, the default, disables any/all wrapping.
     *                                    (However, wrapping always occurs on line-breaks (e.g. CRLF, LF, CR, etc.)).
     *  @param[in]   inGutterWidth        Optionally specifies the gap ("gutter") between the "label"
     *                                    and "value" columns, in character spaces. Defaults to 3 spaces.
     *  @return      A multi-line string containing the formatted AJALabelValuePairs table.
     *  @bug         Multi-byte UTF8 characters need to be counted as one character.
     */
	static std::string ToString (const AJALabelValuePairs & inLabelValuePairs, const size_t inValueWrapLen = 0, const size_t inGutterWidth = 3);

    /**
     *  @brief       A convenience function that appends the given label and value strings to the
     *               provided AJALabelValuePairs table.
     *  @param       inOutTable   The AJALabelValuePairs table to be modified.
     *  @param[in]   inLabel      Specifies the label string to use.
     *  @param[in]   inValue      Specifies the value string to use. If empty, treat "inLabel" as a section heading.
     *  @return      A reference to the given AJALabelValuePairs.
     */
	static inline AJALabelValuePairs & append (AJALabelValuePairs & inOutTable, const std::string & inLabel, const std::string & inValue = std::string())
												{inOutTable.push_back(AJALabelValuePair(inLabel,inValue)); return inOutTable;}

private:	//	Instance Data
    AJASystemInfoImpl* mpImpl;
};


/**
	@brief		Streams a human-readable representation of the AJASystemInfo into the given output stream.
	@param		outStream		The stream into which the given AJASystemInfo will be printed.
	@param[in]	inData			The AJASystemInfo to be streamed.
	@return		A reference to the given "outStream".
**/
AJA_EXPORT std::ostream & operator << (std::ostream & outStream, const AJASystemInfo & inData);

/**
	@brief		Streams a human-readable representation of the AJALabelValuePair into the given output stream.
	@param		outStream		The stream into which the given AJALabelValuePair will be printed.
	@param[in]	inData			The AJALabelValuePair to be streamed.
	@return		A reference to the given "outStream".
**/
AJA_EXPORT std::ostream & operator << (std::ostream & outStream, const AJALabelValuePair & inData);

/**
	@brief		Streams a human-readable representation of the AJALabelValuePairs into the given output stream.
	@param		outStream		The stream into which the given AJALabelValuePairs will be printed.
	@param[in]	inData			The AJALabelValuePairs to be streamed.
	@return		A reference to the given "outStream".
**/
AJA_EXPORT std::ostream & operator << (std::ostream & outStream, const AJALabelValuePairs & inData);

#endif	//	AJA_INFO_H
