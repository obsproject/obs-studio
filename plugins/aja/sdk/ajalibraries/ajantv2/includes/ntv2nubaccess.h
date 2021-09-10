/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2nubaccess.h
	@brief		Declares functions to connect/open/close/send/receive data via the NTV2 "nub".
	@copyright	(C) 2006-2021 AJA Video Systems, Inc.
**/

#ifndef NTV2NUBACCESS_H
#define NTV2NUBACCESS_H

#include "ajaexport.h"
#include "ntv2nubtypes.h"
#include <string>

#define NTV2_REMOTE_ACCESS_SUCCESS						  	 0
#define NTV2_REMOTE_ACCESS_NOT_CONNECTED  				 	-1
#define NTV2_REMOTE_ACCESS_OUT_OF_MEMORY				 	-2
#define NTV2_REMOTE_ACCESS_SEND_ERR						 	-3
#define NTV2_REMOTE_ACCESS_CONNECTION_CLOSED 			 	-4
#define NTV2_REMOTE_ACCESS_RECV_ERR						 	-5
#define NTV2_REMOTE_ACCESS_TIMEDOUT				  		 	-6
#define NTV2_REMOTE_ACCESS_NO_CARD						 	-7
#define NTV2_REMOTE_ACCESS_NOT_OPEN_RESP				 	-8
#define NTV2_REMOTE_ACCESS_NON_NUB_PKT					 	-9
#define NTV2_REMOTE_ACCESS_NOT_READ_REGISTER_RESP			-10
#define NTV2_REMOTE_ACCESS_NOT_WRITE_REGISTER_RESP			-11
#define NTV2_REMOTE_ACCESS_NOT_AUTOCIRC_RESP				-12
#define NTV2_REMOTE_ACCESS_NOT_WAIT_FOR_INTERRUPT_RESP		-13
#define NTV2_REMOTE_ACCESS_WAIT_FOR_INTERRUPT_FAILED		-14
#define NTV2_REMOTE_AUTOCIRC_FAILED							-15
#define NTV2_REMOTE_ACCESS_DRIVER_GET_BITFILE_INFO_FAILED	-16
#define NTV2_REMOTE_ACCESS_NOT_DRIVER_GET_BITFILE_INFO		-17
#define NTV2_REMOTE_ACCESS_NOT_DOWNLOAD_TEST_PATTERN		-18
#define NTV2_REMOTE_ACCESS_DOWNLOAD_TEST_PATTERN_FAILED		-19
#define NTV2_REMOTE_ACCESS_READ_REG_MULTI_FAILED			-20
#define NTV2_REMOTE_ACCESS_NOT_READ_REG_MULTI				-21
#define NTV2_REMOTE_ACCESS_GET_DRIVER_VERSION_FAILED		-22
#define NTV2_REMOTE_ACCESS_NOT_GET_DRIVER_VERSION_RESP		-23
#define NTV2_REMOTE_ACCESS_READ_REG_FAILED					-24
#define NTV2_REMOTE_ACCESS_DRIVER_GET_BUILD_INFO_FAILED		-25
#define NTV2_REMOTE_ACCESS_NOT_DRIVER_GET_BUILD_INFO		-26
#define NTV2_REMOTE_ACCESS_UNIMPLEMENTED					-27


/**
	@brief	Interface to remote or fake devices.
**/
class AJAExport NTV2RPCAPI
{
	public:
		static NTV2RPCAPI *	MakeNTV2NubRPCAPI (const std::string & inSpec, const std::string & inPort = "");
		static NTV2RPCAPI *	FindNTV2SoftwareDevice (const std::string & inName, const std::string & inParams = "");

	public:
										NTV2RPCAPI ();
		virtual							~NTV2RPCAPI();
		//	Inquiry
		virtual bool					IsConnected	(void) const;
		virtual std::string				Name (void) const;
		virtual std::ostream &			Print (std::ostream & oss) const;
		virtual NTV2NubProtocolVersion	NubProtocolVersion (void) const;
		virtual uint32_t				Version (void) const;
		virtual NTV2_POINTER &			localStorage (void);
		virtual const NTV2_POINTER &	localStorage (void) const;

//		virtual int		NTV2Connect		(const std::string & inHostname, const UWord inDeviceIndex);
		virtual int		NTV2Disconnect	(void);

		virtual int		NTV2ReadRegisterRemote	(const ULWord regNum, ULWord & outRegValue, const ULWord regMask, const ULWord regShift);
	
		virtual int		NTV2WriteRegisterRemote	(const ULWord regNum, const ULWord regValue, const ULWord regMask, const ULWord regShift);
	
		virtual int		NTV2AutoCirculateRemote	(AUTOCIRCULATE_DATA & autoCircData);
	
		virtual int		NTV2WaitForInterruptRemote	(const INTERRUPT_ENUMS eInterrupt, const ULWord timeOutMs);
	
		virtual int		NTV2DriverGetBitFileInformationRemote	(BITFILE_INFO_STRUCT & bitFileInfo, const NTV2BitFileType bitFileType);
	
		virtual int		NTV2DriverGetBuildInformationRemote	(BUILD_INFO_STRUCT & buildInfo);
	
		virtual int		NTV2DownloadTestPatternRemote	(const NTV2Channel channel, const NTV2PixelFormat testPatternFBF,
														const UWord signalMask, const bool testPatDMAEnb, const ULWord testPatNum);
	
		virtual int		NTV2ReadRegisterMultiRemote	(const ULWord numRegs, ULWord & outFailedRegNum, NTV2RegInfo outRegs[]);
	
		virtual int		NTV2GetDriverVersionRemote	(ULWord & outDriverVersion);
	
		virtual int		NTV2DMATransferRemote	(const NTV2DMAEngine inDMAEngine,	const bool inIsRead,
												const ULWord inFrameNumber,			ULWord * pFrameBuffer,
												const ULWord inCardOffsetBytes,		const ULWord inTotalByteCount,
												const ULWord inNumSegments,			const ULWord inSegmentHostPitch,
												const ULWord inSegmentCardPitch,	const bool inSynchronous);
	
		virtual int		NTV2MessageRemote	(NTV2_HEADER *	pInMessage);

	protected:
		virtual int		NTV2OpenRemote	(const UWord inDeviceIndex);
		virtual int		NTV2CloseRemote	(void);

	protected:
		std::string		_hostname;
		uint32_t		_instanceData[1024];	//	private storage
		NTV2_POINTER	_pvt;
};	//	NTV2RPCAPI

inline std::ostream & operator << (std::ostream & oss, const NTV2RPCAPI & inObj)	{return inObj.Print(oss);}

extern "C" {
	typedef NTV2RPCAPI* (*fpCreateNTV2SoftwareDevice) (void * /*pInDLLHandle*/, const std::string & /*inParams*/, const uint32_t /*inHostSDKVersion*/);
}

#endif	//	NTV2NUBACCESS_H
