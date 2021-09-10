/* SPDX-License-Identifier: MIT */
/**
    @file		ntv2registerexpert.h
    @brief		Declares the CNTV2RegisterExpert class.
    @copyright	(C) 2016-2021 AJA Video Systems, Inc.
**/

#ifndef NTV2REGEXPERT_H
#define NTV2REGEXPERT_H

#include "ajaexport.h"
#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2publicinterface.h"
#include "ntv2utils.h"
#include <string>
#include <iostream>
#include <vector>
#if defined (AJALinux)
    #include <stdint.h>
#endif


//	Streaming Helpers
#define	YesNo(__x__)					((__x__) ? "Y"				: "N")
#define	OnOff(__x__)					((__x__) ? "On"				: "Off")
#define	SetNotset(__x__)				((__x__) ? "Set"			: "Not Set")
#define	EnabDisab(__x__)				((__x__) ? "Enabled"		: "Disabled")
#define	DisabEnab(__x__)				((__x__) ? "Disabled"		: "Enabled")
#define	ActInact(__x__)					((__x__) ? "Active"			: "Inactive")
#define	SuppNotsupp(__x__)				((__x__) ? "Supported"		: "Unsupported")
#define	PresNotPres(__x__)				((__x__) ? "Present"		: "Not Present")
#define	ThruDeviceOrBypassed(__x__)		((__x__) ? "Thru Device"	: "Device Bypassed")


//	Register classifier keys
#define	kRegClass_NULL		std::string ()
#define	kRegClass_Audio		std::string ("kRegClass_Audio")
#define	kRegClass_Video		std::string ("kRegClass_Video")
#define	kRegClass_Anc		std::string ("kRegClass_Anc")
#define	kRegClass_DMA		std::string ("kRegClass_DMA")
#define	kRegClass_Mixer		std::string ("kRegClass_Mixer")
#define	kRegClass_Serial	std::string ("kRegClass_Serial")
#define	kRegClass_Timecode	std::string ("kRegClass_Timecode")
#define	kRegClass_Routing	std::string ("kRegClass_Routing")
#define	kRegClass_Input		std::string ("kRegClass_Input")
#define	kRegClass_Output	std::string ("kRegClass_Output")
#define	kRegClass_CSC		std::string ("kRegClass_CSC")
#define	kRegClass_LUT		std::string ("kRegClass_LUT")
#define	kRegClass_Analog	std::string ("kRegClass_Analog")
#define	kRegClass_AES		std::string ("kRegClass_AES")
#define	kRegClass_HDMI		std::string ("kRegClass_HDMI")
#define	kRegClass_HDR		std::string ("kRegClass_HDR")
#define	kRegClass_VPID		std::string ("kRegClass_VPID")
#define	kRegClass_SDIError	std::string ("kRegClass_SDIError")
#define	kRegClass_Timing	std::string ("kRegClass_Timing")
#define	kRegClass_Channel1	std::string ("kRegClass_Channel1")
#define	kRegClass_Channel2	std::string ("kRegClass_Channel2")
#define	kRegClass_Channel3	std::string ("kRegClass_Channel3")
#define	kRegClass_Channel4	std::string ("kRegClass_Channel4")
#define	kRegClass_Channel5	std::string ("kRegClass_Channel5")
#define	kRegClass_Channel6	std::string ("kRegClass_Channel6")
#define	kRegClass_Channel7	std::string ("kRegClass_Channel7")
#define	kRegClass_Channel8	std::string ("kRegClass_Channel8")
#define	kRegClass_ReadOnly	std::string ("kRegClass_ReadOnly")
#define	kRegClass_WriteOnly	std::string ("kRegClass_WriteOnly")
#define	kRegClass_Virtual	std::string ("kRegClass_Virtual")
#define kRegClass_Interrupt	std::string ("kRegClass_Interrupt")


/**
    @brief	I provide "one-stop shopping" for information about registers and their values.
	@bug	This class currently excludes bank-selected registers, so support logs for \ref konaip and
			\ref ioip will be incomplete. This will be addressed in a future SDK.
**/
class AJAExport CNTV2RegisterExpert
{
    public:
        /**
            @param[in]	inRegNum	Specifies the register number.
            @return		A string that contains the name of the register (or empty if unknown).
        **/
        static std::string		GetDisplayName	(const uint32_t inRegNum);

        /**
            @param[in]	inRegNum	Specifies the register number.
            @param[in]	inRegValue	Specifies the 32-bit register value.
            @param[in]	inDeviceID	Optionally specifies an NTV2DeviceID. Defaults to DEVICE_ID_NOTFOUND.
            @return		A string that contains the human-readable rendering of the value of the given register
                        (or empty if unknown).
        **/
        static std::string		GetDisplayValue	(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID = DEVICE_ID_NOTFOUND);

        /**
            @param[in]	inRegNum	Specifies the register number.
            @param[in]	inClassName	Specifies the class name.
            @return		True if the register is a member of the class;  otherwise false.
        **/
        static bool				IsRegisterInClass (const uint32_t inRegNum, const std::string & inClassName);

        /**
            @param[in]	inRegNum	Specifies the register number.
            @return		True if the register is read-only (i.e., it cannot be written);  otherwise false.
        **/
        static inline bool		IsReadOnly		(const uint32_t inRegNum)	{return IsRegisterInClass (inRegNum, kRegClass_ReadOnly);}

        /**
            @param[in]	inRegNum	Specifies the register number.
            @return		True if the register is write-only (i.e., it cannot be read);  otherwise false.
        **/
        static inline bool		IsWriteOnly		(const uint32_t inRegNum)	{return IsRegisterInClass (inRegNum, kRegClass_WriteOnly);}

        /**
            @return		A set of strings containing the names of all known register classes.
        **/
        static NTV2StringSet	GetAllRegisterClasses (void);

        /**
            @return		A set of strings containing the names of all register classes the given register belongs to.
        **/
        static NTV2StringSet	GetRegisterClasses (const uint32_t inRegNum);

        /**
            @param[in]	inClassName	Specifies the register class.
            @return		A set of register numbers that belong to the specified class. Will be empty if none found.
        **/
        static NTV2RegNumSet	GetRegistersForClass	(const std::string & inClassName);

        /**
            @param[in]	inChannel	Specifies a valid NTV2Channel.
            @return		A set of register numbers that are associated with the given NTV2Channel (class kRegClass_ChannelN).
                        Will be empty if an invalid NTV2Channel is specified.
        **/
        static NTV2RegNumSet	GetRegistersForChannel	(const NTV2Channel inChannel);

        /**
            @param[in]	inDeviceID			Specifies a valid NTV2DeviceID.
            @param[in]	inIncludeVirtuals	Specify true to include virtual registers;  otherwise virtual registers
                                            will be excluded (the default).
            @return		A set of register numbers that are legal for the device having the given NTV2DeviceID.
                        Will be empty if an invalid NTV2DeviceID is specified.
        **/
        static NTV2RegNumSet	GetRegistersForDevice (const NTV2DeviceID inDeviceID, const bool inIncludeVirtuals = false);

        /**
            @param[in]	inName			Specifies a non-empty string that contains all or part of a register name.
            @param[in]	inSearchStyle	Specifies the search style. Must be EXACTMATCH (the default), CONTAINS,
                                        STARTSWITH or ENDSWITH.
            @return		A set of register numbers that match all or part of the given name.
                        Empty if none match.
            @note		All searching is performed case-insensitively.
        **/
        static NTV2RegNumSet	GetRegistersWithName (const std::string & inName, const int inSearchStyle = EXACTMATCH);

        /**
            @param[in]	inXptRegNum		Specifies the crosspoint select group register number. Note that it only makes
                                        sense to pass register numbers having names that start with "kRegXptSelectGroup".
            @param[in]	inMaskIndex		Specifies the mask index, an unsigned value that must be less than 4.
                                        (0 specifies the least significant byte of the crosspoint register value,
                                        3 specifies the most significant byte, etc.)
            @return		The NTV2InputCrosspointID of the widget input crosspoint associated with the given "Xpt"
                        register and mask index, or NTV2_INPUT_CROSSPOINT_INVALID if there isn't one.
        **/
        static NTV2InputCrosspointID	GetInputCrosspointID (const uint32_t inXptRegNum, const uint32_t inMaskIndex);

        /**
            @brief		Answers with the crosspoint select register and mask information for a given widget input.
            @param[in]	inInputXpt		Specifies the NTV2InputCrosspointID of interest.
            @param[out]	outXptRegNum	Receives the crosspoint select group register number.
            @param[out]	outMaskIndex	Receives the mask index (where 0=0x000000FF, 1=0x0000FF00, 2=0x00FF0000, 3=0xFF000000).
            @return		True if successful;  otherwise false.
        **/
        static bool						GetCrosspointSelectGroupRegisterInfo (const NTV2InputCrosspointID inInputXpt, uint32_t & outXptRegNum, uint32_t & outMaskIndex);

        static const int	CONTAINS	=	0;	///< @brief	The name must contain the search string (see CNTV2RegisterExpert::GetRegistersWithName)
        static const int	STARTSWITH	=	1;	///< @brief	The name must start with the search string (see CNTV2RegisterExpert::GetRegistersWithName)
        static const int	ENDSWITH	=	2;	///< @brief	The name must end with the search string (see CNTV2RegisterExpert::GetRegistersWithName)
        static const int	EXACTMATCH	=	3;	///< @brief	The name must exactly match the search string (see CNTV2RegisterExpert::GetRegistersWithName)
    	static bool			IsAllocated(void);	///< @return	True if the Register Expert singleton has been allocated/created;  otherwise false.

        /**
            @brief		Explicitly allocates the Register Expert singleton.
            @return		True if successful;  otherwise false.
            @note		Normally, there is no need to call this function, as the RegisterExpert singleton is
            			automatically allocated.
        **/
    	static bool Allocate(void);

        /**
            @brief		Explicitly deallocates the Register Expert singleton.
            @return		True if successful;  otherwise false.
            @note		Normally, there is no need to call this function, as the RegisterExpert singleton is
            			automatically deallocated.
        **/
    	static bool Deallocate(void);

};	//	CNTV2RegisterExpert

#endif	//	NTV2REGEXPERT_H
