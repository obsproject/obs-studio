/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2driverinterface.h
	@brief		Declares the CNTV2DriverInterface base class.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/

#ifndef NTV2DRIVERINTERFACE_H
#define NTV2DRIVERINTERFACE_H

#include "ajaexport.h"
#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2videodefines.h"
#include "ntv2audiodefines.h"
#include "ntv2nubaccess.h"
#include "ntv2publicinterface.h"
#include "ntv2utils.h"
#include "ntv2devicefeatures.h"
#if defined(NTV2_WRITEREG_PROFILING)	//	Register Write Profiling
	#include "ajabase/system/lock.h"
#endif	//	NTV2_WRITEREG_PROFILING		Register Write Profiling
#include <string>

//	Check consistent use of AJA_USE_CPLUSPLUS11 and NTV2_USE_CPLUSPLUS11
#ifdef AJA_USE_CPLUSPLUS11
	#ifndef NTV2_USE_CPLUSPLUS11
		#error "AJA_USE_CPLUSPLUS11 && !NTV2_USE_CPLUSPLUS11"
	#else
		//#warning "AJA_USE_CPLUSPLUS11 && NTV2_USE_CPLUSPLUS11"
	#endif
#else
	#ifdef NTV2_USE_CPLUSPLUS11
		#error "!AJA_USE_CPLUSPLUS11 && NTV2_USE_CPLUSPLUS11"
	#else
		//#warning "!AJA_USE_CPLUSPLUS11 && !NTV2_USE_CPLUSPLUS11"
	#endif
#endif

#if defined(AJALinux ) || defined(AJAMac)
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <unistd.h>
#endif

#ifdef MSWindows
	#include <WinSock2.h>
	#include <assert.h>
#endif


#define	AsNTV2DriverInterfaceRef(_x_)	reinterpret_cast<CNTV2DriverInterface&>(_x_)
#define	AsNTV2DriverInterfacePtr(_x_)	reinterpret_cast<CNTV2DriverInterface*>(_x_)


typedef struct
{
	std::string		buildNumber;
	std::string		packageNumber;
	std::string		date;
	std::string		time;
} PACKAGE_INFO_STRUCT, *PPACKAGE_INFO_STRUCT;


/**
	@brief	I'm the base class that undergirds the platform-specific derived classes (from which ::CNTV2Card is ultimately derived).
**/
class AJAExport CNTV2DriverInterface
{
	//	STATIC (CLASS) METHODS
	public:
		static NTV2StringList	GetLegalSchemeNames (void);	///< @return	A list of legal scheme names that can be used to open remote or non-physical devices. (New in SDK 16.0)
		static inline UWord		MaxNumDevices (void)	{return 32;}	///< @return	Maximum number of local/physical device connections. (New in SDK 16.0)

		/**
			@brief		Specifies if subsequent Open calls should open the device in shared mode or not.
			@note		On some platforms, this function may have no effect.
			@param[in]	inSharedMode	Specify true for shared mode;  otherwise use false.
		**/
		static void				SetShareMode (const bool inSharedMode);
		static bool				GetShareMode (void);	///< @return	True if local devices will be opened in shared mode; otherwise false. (New in SDK 16.0)

		/**
			@brief		Specifies if the next Open call should try to open the device in shared mode or not.
			@note		On some platforms,  this function may have no effect.
			@param[in]	inOverlapMode	Specify true for overlapped mode;  otherwise use false.
		**/
		static void				SetOverlappedMode (const bool inOverlapMode);
		static bool				GetOverlappedMode (void);	///< @return	True if local devices will be opened in overlapped mode; otherwise false. (New in SDK 16.0)

	/**
		@name	Construction, destruction, assignment
	**/
	///@{
	public:
				CNTV2DriverInterface();		///< @brief	My default constructor.
		virtual	~CNTV2DriverInterface();	///< @brief	My destructor.

	private:
		/**
			@brief	My assignment operator.
			@note	We have intentionally disabled this capability because historically it was never implemented properly.
			@param[in]	inRHS	The rvalue to be assigned to the lvalue.
			@return	A non-constant reference to the lvalue.
		**/
		AJA_VIRTUAL CNTV2DriverInterface & operator = (const CNTV2DriverInterface & inRHS);

		/**
			@brief	My copy constructor.
			@note	We have intentionally disabled this capability because historically it was never implemented properly.
			@param[in]	inObjToCopy		The object to be copied.
		**/
		CNTV2DriverInterface (const CNTV2DriverInterface & inObjToCopy);
	///@}

	public:
	/**
		@name	Inquiry
	**/
	///@{
		AJA_VIRTUAL NTV2DeviceID	GetDeviceID (void);	///< @return	The 4-byte value that identifies the kind of AJA device this is.
		AJA_VIRTUAL inline UWord	GetIndexNumber (void) const	{return _boardNumber;}	///< @return	My zero-based index number (relative to other devices attached to the host).
		AJA_VIRTUAL inline bool		IsOpen (void) const			{return _boardOpened;}	///< @return	True if I'm able to communicate with the device I represent;  otherwise false.

		/**
			@return		True if the device is ready to be fully operable;  otherwise false.
			@param[in]	inCheckValid	If true, additionally checks CNTV2Card::IsMBSystemValid. Defaults to false.
			@note		Some devices have processors that require a lot of time (~30 to ~90 seconds) to start up after a PCIe bus reset,
						power-up or wake from sleep. Calls to CNTV2Card::IsOpen, CNTV2Card::ReadRegister and CNTV2Card::WriteRegister
						will all succeed, but the device won't be capable of either ingesting or playing video or performing DMA operations.
		**/
		AJA_VIRTUAL bool		IsDeviceReady (const bool inCheckValid = false);
		AJA_VIRTUAL bool		IsMBSystemValid (void);	///< @return	True if microblaze system exists (is valid); otherwise false.
		AJA_VIRTUAL bool		IsMBSystemReady (void);	///< @return	True if microblaze system is in ready state; otherwise false.
		AJA_VIRTUAL inline bool	IsIPDevice (void)	{return ::NTV2DeviceCanDoIP(GetDeviceID());}	///< @return	True if I am an IP device; otherwise false.
	///@}

	/**
		@name	Open/Close, Connect/Disconnect
	**/
	///@{
		/**
			@brief		Opens a local/physical AJA device so it can be monitored/controlled.
			@param[in]	inDeviceIndex	Specifies a zero-based index number of the AJA device to open.
			@result		True if successful; otherwise false.
			@note		Before attempting the Open, if I'm currently open, Close will be called first.
		**/
		AJA_VIRTUAL bool		Open (const UWord inDeviceIndex);

		/**
			@brief		Opens the specified local, remote or software device.
			@param[in]	inURLSpec	Specifies the local, remote or software device to be opened.
			@result		True if successful; otherwise false.
			@note		Before attempting the Open, if I'm currently open, Close will be called first.
		**/
		AJA_VIRTUAL bool		Open (const std::string & inURLSpec);

		/**
			@brief		Closes me, releasing host resources that may have been allocated in a previous Open call.
			@result		True if successful; otherwise false.
			@details	This function closes any onnection to an AJA device.
						Once closed, the device can no longer be queried or controlled by this instance.
		**/
		AJA_VIRTUAL bool		Close (void);
	///@}

	/**
		@name	Register Read/Write
	**/
	///@{

		/**
			@brief		Updates or replaces all or part of the 32-bit contents of a specific register (real or virtual)
						on the AJA device. Using the optional mask and shift parameters, it's possible to set or clear
						any number of specific bits in a real register without altering any of the register's other bits.
			@result		True if successful; otherwise false.
			@param[in]	inRegNum	Specifies the register number of interest.
			@param[in]	inValue		Specifies the desired new register value. If the "inShift" parameter is non-zero,
									this value is shifted left by the designated number of bit positions before being
									masked and applied to the real register contents.
			@param[in]	inMask		Optionally specifies a bit mask to be applied to the new (shifted) value before
									updating the register. Defaults to 0xFFFFFFFF, which does not perform any masking.
									On Windows and MacOS, a zero mask is treated the same as 0xFFFFFFFF.
			@param[in]	inShift		Optionally specifies the number of bits to left-shift the specified value before applying
									it to the register. Defaults to zero, which does not perform any shifting.
									On MacOS, this parameter is ignored.
									On Windows, a shift value of 0xFFFFFFFF is treated the same as a zero shift value.
			@note		This function should be used only when there is no higher-level function available to accomplish the desired task.
			@note		The mask and shift parameters are ignored when setting a virtual register.
			@bug		On MacOS, "holes" in the mask (i.e., one or more runs of 0-bits lying between more-significant and
						less-significant 1-bits) were not handled correctly.
		**/
		AJA_VIRTUAL bool	WriteRegister (const ULWord inRegNum,  const ULWord inValue,  const ULWord inMask = 0xFFFFFFFF,  const ULWord inShift = 0);

		/**
			@brief		Reads all or part of the 32-bit contents of a specific register (real or virtual) on the AJA device.
						Using the optional mask and shift parameters, it's possible to read any number of specific bits in a register
						while ignoring the register's other bits.
			@result		True if successful; otherwise false.
			@param[in]	inRegNum	Specifies the register number of interest.
			@param[out]	outValue	Receives the register value obtained from the device.
			@param[in]	inMask		Optionally specifies a bit mask to be applied after reading the device register.
									Zero and 0xFFFFFFFF masks are ignored. Defaults to 0xFFFFFFFF (no masking).
			@param[in]	inShift		Optionally specifies the number of bits to right-shift the value obtained
									from the device register after the mask has been applied. Defaults to zero (no shift).
			@note		This function should be used only when there is no higher-level function available to accomplish the desired task.
			@note		The mask and shift parameters are ignored when reading a virtual register.
		**/
		AJA_VIRTUAL bool	ReadRegister (const ULWord inRegNum,  ULWord & outValue,  const ULWord inMask = 0xFFFFFFFF,  const ULWord inShift = 0);

		/**
			@brief		This template function reads all or part of the 32-bit contents of a specific register (real or virtual)
						from the AJA device, and if successful, returns its value automatically casted to the scalar type of the
						"outValue" parameter.
			@result		True if successful; otherwise false.
			@param[in]	inRegNum	Specifies the register number of interest.
			@param[out]	outValue	Receives the register value obtained from the device, automatically casted to the parameter's type.
									Its type must be statically castable from ULWord (i.e. it must be a scalar).
			@param[in]	inMask		Optionally specifies a bit mask to be applied after reading the device register.
									Zero and 0xFFFFFFFF masks are ignored. Defaults to 0xFFFFFFFF (no masking).
			@param[in]	inShift		Optionally specifies the number of bits to right-shift the value obtained
									from the device register after the mask has been applied. Defaults to zero (no shift).
			@note		This function should be used only when there is no higher-level function available to accomplish the desired task.
			@note		The mask and shift parameters are ignored when reading a virtual register.
		**/
		template<typename T> bool	ReadRegister(const ULWord inRegNum,  T & outValue,  const ULWord inMask = 0xFFFFFFFF,  const ULWord inShift = 0)
									{
										ULWord regValue(0);
										bool result (ReadRegister(inRegNum, regValue, inMask, inShift));
										if (result)
											outValue = T(regValue);
										return result;
									}

#if !defined(READREGMULTICHANGE)
		/**
			@brief		Reads the register(s) specified by the given ::NTV2RegInfo sequence.
			@param[in]	inOutValues		Specifies the register(s) to be read, and upon return, contains their values.
			@return		True only if all registers were read successfully;  otherwise false.
			@note		This operation is not guaranteed to be performed atomically.
		**/
		AJA_VIRTUAL bool	ReadRegisters (NTV2RegisterReads & inOutValues);
#endif	//	!defined(READREGMULTICHANGE)
		AJA_VIRTUAL bool	RestoreHardwareProcampRegisters() = 0;
	///@}

	/**
		@name	DMA Transfer
	**/
	///@{
		/**
			@brief		Transfers data between the AJA device and the host. This function will block and not return to the caller until
						the transfer has finished or failed.
			@param[in]	inDMAEngine			Specifies the device DMA engine to use. Use NTV2_DMA_FIRST_AVAILABLE for most applications.
											(Use the ::NTV2DeviceGetNumDMAEngines function to determine how many are available.)
			@param[in]	inIsRead			Specifies the transfer direction. Use 'true' for reading (device-to-host).
											Use 'false' for writing (host-to-device).
			@param[in]	inFrameNumber		Specifies the zero-based frame number of the starting frame to be transferred to/from the device.
			@param		pFrameBuffer		Specifies a valid, non-NULL address of the host buffer. If reading (device-to-host), this memory
											will be written into. If writing (host-to-device), this memory will be read from.
			@param[in]	inOffsetBytes		Specifies the byte offset into the device frame buffer where the data transfer will start.
			@param[in]	inTotalByteCount	Specifies the total number of bytes to be transferred.
			@param[in]	inSynchronous		This parameter is obsolete, and ignored.
			@return		True if successful; otherwise false.
			@note		The host buffer must be at least inByteCount + inOffsetBytes in size; otherwise, host memory will be corrupted,
						or a bus error or other runtime exception may occur.
		**/
		AJA_VIRTUAL bool	DmaTransfer (const NTV2DMAEngine	inDMAEngine,
										const bool				inIsRead,
										const ULWord			inFrameNumber,
										ULWord *				pFrameBuffer,
										const ULWord			inOffsetBytes,
										const ULWord			inTotalByteCount,
										const bool				inSynchronous = true);

		/**
			@brief		Transfers data in segments between the AJA device and the host. This function will block
						and not return to the caller until the transfer has finished or failed.
			@param[in]	inDMAEngine			Specifies the device DMA engine to use. Use ::NTV2_DMA_FIRST_AVAILABLE
											for most applications. (Use the ::NTV2DeviceGetNumDMAEngines function
											to determine how many are available.)
			@param[in]	inIsRead			Specifies the transfer direction. Use 'true' for reading (device-to-host).
											Use 'false' for writing (host-to-device).
			@param[in]	inFrameNumber		Specifies the zero-based frame number of the starting frame to be
											transferred to/from the device.
			@param		pFrameBuffer		Specifies a valid, non-NULL address of the host buffer. If reading
											(device-to-host), this memory will be written into. If writing
											(host-to-device), this memory will be read from.
			@param[in]	inCardOffsetBytes	Specifies the byte offset into the device frame buffer where the data
											transfer will start.
			@param[in]	inTotalByteCount	Specifies the total number of bytes to be transferred.
			@param[in]	inNumSegments		Specifies the number of segments to transfer. Note that this determines
											the number of bytes per segment (by dividing into <i>inTotalByteCount</i>).
			@param[in]	inHostPitchPerSeg	Specifies the number of bytes to increment the host memory pointer
											after each segment is transferred.
			@param[in]	inCardPitchPerSeg	Specifies the number of bytes to increment the on-device memory pointer
											after each segment is transferred.
			@param[in]	inSynchronous		This parameter is obsolete, and ignored.
			@return		True if successful; otherwise false.
			@note		The host buffer must be at least inByteCount + inOffsetBytes in size; otherwise, host memory will be corrupted,
						or a bus error or other runtime exception may occur.
		**/
		AJA_VIRTUAL bool	DmaTransfer (const NTV2DMAEngine	inDMAEngine,
										const bool				inIsRead,
										const ULWord			inFrameNumber,
										ULWord *				pFrameBuffer,
										const ULWord			inCardOffsetBytes,
										const ULWord			inTotalByteCount,
										const ULWord			inNumSegments,
										const ULWord			inHostPitchPerSeg,
										const ULWord			inCardPitchPerSeg,
										const bool				inSynchronous = true)	= 0;

		AJA_VIRTUAL bool	DmaTransfer (const NTV2DMAEngine		inDMAEngine,
										const NTV2Channel			inDMAChannel,
										const bool					inTarget,
										const ULWord				inFrameNumber,
										const ULWord				inCardOffsetBytes,
										const ULWord				inByteCount,
										const ULWord				inNumSegments,
										const ULWord				inSegmentHostPitch,
										const ULWord				inSegmentCardPitch,
										const PCHANNEL_P2P_STRUCT &	inP2PData)	= 0;
	///@}

	/**
		@name	Interrupts
	**/
	///@{
		AJA_VIRTUAL bool	ConfigureSubscription (const bool bSubscribe, const INTERRUPT_ENUMS inInterruptType, PULWord & outSubcriptionHdl);
		AJA_VIRTUAL bool	ConfigureInterrupt (const bool bEnable,  const INTERRUPT_ENUMS eInterruptType) = 0;
		/**
			@brief	Answers with the number of interrupts of the given type processed by the driver.
			@param[in]	eInterrupt	The interrupt type of interest.
			@param[out]	outCount	Receives the count value.
			@return	True if successful;  otherwise false.
		**/
		AJA_VIRTUAL bool	GetInterruptCount (const INTERRUPT_ENUMS eInterrupt,  ULWord & outCount) = 0;

		AJA_VIRTUAL bool	WaitForInterrupt (const INTERRUPT_ENUMS eInterrupt, const ULWord timeOutMs = 68);

		AJA_VIRTUAL HANDLE	GetInterruptEvent (const INTERRUPT_ENUMS eInterruptType);
		/**
			@brief		Answers with the number of interrupt events that I successfully waited for.
			@param[in]	inEventCode		Specifies the interrupt of interest.
			@param[out]	outCount		Receives the number of interrupt events that I successfully waited for.
			@return		True if successful;  otherwise false.
			@see		CNTV2DriverInterface::SetInterruptEventCount, \ref fieldframeinterrupts
		**/
		AJA_VIRTUAL bool	GetInterruptEventCount (const INTERRUPT_ENUMS inEventCode, ULWord & outCount);

		/**
			@brief		Resets my interrupt event tally for the given interrupt type. (This is my count of the number of successful event waits.)
			@param[in]	inEventCode		Specifies the interrupt type.
			@param[in]	inCount			Specifies the new count value. Use zero to reset the tally.
			@return		True if successful;  otherwise false.
			@see		CNTV2DriverInterface::GetInterruptEventCount, \ref fieldframeinterrupts
		**/
		AJA_VIRTUAL bool	SetInterruptEventCount (const INTERRUPT_ENUMS inEventCode, const ULWord inCount);
	///@}

	/**
		@name	Control/Messaging
	**/
	///@{
		/**
			@brief	Sends an AutoCirculate command to the NTV2 driver.
			@param	pAutoCircData	Points to the AUTOCIRCULATE_DATA that contains the AutoCirculate API command and parameters.
			@return	True if successful;  otherwise false.
		**/
		AJA_VIRTUAL bool		AutoCirculate (AUTOCIRCULATE_DATA & pAutoCircData);

		/**
			@brief	Sends a message to the NTV2 driver (the new, improved, preferred way).
			@param	pInMessage	Points to the message to pass to the driver.
								Valid messages start with an NTV2_HEADER and end with an NTV2_TRAILER.
			@return	True if successful;  otherwise false.
		**/
		AJA_VIRTUAL bool		NTV2Message (NTV2_HEADER * pInMessage);

		/**
			@brief	Sends an HEVC message to the NTV2 driver.
			@param	pMessage	Points to the HevcMessageHeader that contains the HEVC message.
			@return	False. This must be implemented by the platform-specific subclass.
		**/
	    AJA_VIRTUAL inline bool	HevcSendMessage (HevcMessageHeader * pMessage)		{(void) pMessage; return false;}

		AJA_VIRTUAL bool	ControlDriverDebugMessages (NTV2_DriverDebugMessageSet msgSet,  bool enable) = 0;
	///@}

		AJA_VIRTUAL inline ULWord	GetNumFrameBuffers (void) const				{return _ulNumFrameBuffers;}
		AJA_VIRTUAL inline ULWord	GetFrameBufferSize (void) const				{return _ulFrameBufferSize;}

		/**
			@brief		Answers with the currently-installed bitfile information.
			@param[out]	outBitFileInfo	Receives the bitfile info.
			@param[in]	inBitFileType	Optionally specifies the bitfile type of interest. Defaults to NTV2_VideoProcBitFile.
			@return		True if successful;  otherwise false.
		**/
		AJA_VIRTUAL bool DriverGetBitFileInformation (BITFILE_INFO_STRUCT & outBitFileInfo,  const NTV2BitFileType inBitFileType = NTV2_VideoProcBitFile);

		/**
			@brief		Answers with the driver's build information.
			@param[out]	outBuildInfo	Receives the build information.
			@return		True if successful;  otherwise false.
		**/
		AJA_VIRTUAL bool DriverGetBuildInformation (BUILD_INFO_STRUCT & outBuildInfo);

		/**
			@brief		Answers with the IP device's package information.
			@param[out]	outPkgInfo	Receives the package information.
			@return		True if successful;  otherwise false.
		**/
		AJA_VIRTUAL bool GetPackageInformation (PACKAGE_INFO_STRUCT & outPkgInfo);

		AJA_VIRTUAL bool BitstreamWrite (const NTV2_POINTER & inBuffer, const bool inFragment, const bool inSwap);
		AJA_VIRTUAL bool BitstreamReset (const bool inConfiguration, const bool inInterface);
		AJA_VIRTUAL bool BitstreamStatus (NTV2ULWordVector & outRegValues);

	/**
		@name	Device Ownership
	**/
	///@{
		/**
			@brief		A reference-counted version of CNTV2DriverInterface::AcquireStreamForApplication useful for
						process groups.
			@result		True if successful; otherwise false.
			@param[in]	inAppType		A 32-bit value that uniquely and globally identifies the calling application
										or process that is requesting exclusive use of the device.
			@param[in]	inProcessID		Specifies the OS-specific process identifier that uniquely identifies the running
										process on the host machine that is requesting exclusive use of the device.
			@details	This method reserves exclusive use of the AJA device by the given running host process,
						recording both the "process ID" and 4-byte "application type" specified by the caller, and if
						already reserved for the given application, increments a reference counter. If a different host
						process has already reserved the device, the function will fail.
			@note		AJA recommends saving the device's ::NTV2EveryFrameTaskMode when this function is called, and
						restoring the task mode after releasing the device (when CNTV2DriverInterface::ReleaseStreamForApplicationWithReference
						is called).
			@note		Each call to CNTV2DriverInterface::AcquireStreamForApplicationWithReference should be balanced by
						the same number of calls to CNTV2DriverInterface::ReleaseStreamForApplicationWithReference.
		**/
		AJA_VIRTUAL bool AcquireStreamForApplicationWithReference (const ULWord inAppType, const int32_t inProcessID);

		/**
			@brief		A reference-counted version of CNTV2DriverInterface::ReleaseStreamForApplication useful for
						process groups.
			@result		True if successful; otherwise false.
			@param[in]	inAppType		A 32-bit value that uniquely and globally identifies the calling application
										or process that is releasing exclusive use of the device.
			@param[in]	inProcessID		Specifies the OS-specific process identifier that uniquely identifies the running
										process on the host machine that is releasing exclusive use of the device.
			@details	This method releases exclusive use of the AJA device by the given host process once the reference
						count goes to zero. This method will fail if the specified application type or process ID values
						don't match those used in the prior call to CNTV2DriverInterface::AcquireStreamForApplication.
			@note		AJA recommends restoring the device's original ::NTV2EveryFrameTaskMode as it was when
						CNTV2DriverInterface::AcquireStreamForApplicationWithReference was first called, and restoring it
						after the final call to this function (when the device has truly been released).
			@note		Each call to CNTV2DriverInterface::AcquireStreamForApplicationWithReference should be balanced by
						the same number of calls to CNTV2DriverInterface::ReleaseStreamForApplicationWithReference.
		**/
		AJA_VIRTUAL bool ReleaseStreamForApplicationWithReference (const ULWord inAppType, const int32_t inProcessID);

		/**
			@brief		Reserves exclusive use of the AJA device for a given process, preventing other processes on the host
						from acquiring it until subsequently released.
			@result		True if successful; otherwise false.
			@param[in]	inAppType		A 32-bit value that uniquely and globally identifies the calling application
										or process that is requesting exclusive use of the device.
			@param[in]	inProcessID		Specifies the OS-specific process identifier that uniquely identifies the running
										process on the host machine that is requesting exclusive use of the device.
			@details	This method reserves exclusive use of the AJA device by the given running host process.
						The AJA device records both the "process ID" and 4-byte "application type". If another host process
						has already reserved the device, this function will fail.
			@note		AJA recommends saving the device's ::NTV2EveryFrameTaskMode when this function is called, and
						restoring it after CNTV2DriverInterface::ReleaseStreamForApplication is called to release the device.
			@note		A call to CNTV2DriverInterface::AcquireStreamForApplication should be balanced by a call to
						CNTV2DriverInterface::ReleaseStreamForApplication.
		**/
		AJA_VIRTUAL bool AcquireStreamForApplication (const ULWord inAppType, const int32_t inProcessID);

		/**
			@brief		Releases exclusive use of the AJA device for a given process, permitting other processes to acquire it.
			@result		True if successful; otherwise false.
			@param[in]	inAppType		A 32-bit value that uniquely and globally identifies the calling application
										or process that is requesting exclusive use of the device.
			@param[in]	inProcessID		Specifies the OS-specific process identifier that uniquely identifies the running
										process on the host machine that is requesting exclusive use of the device.
			@details	This method releases exclusive use of the AJA device by the given host process. This method will
						fail if the specified application type or process ID values don't match those used in the prior
						call to CNTV2DriverInterface::AcquireStreamForApplication.
			@note		AJA recommends saving the device's ::NTV2EveryFrameTaskMode at the time
						CNTV2DriverInterface::AcquireStreamForApplication is called, and restoring it after releasing
						the device.
			@note		A call to CNTV2DriverInterface::AcquireStreamForApplication should be balanced by a call to
						CNTV2DriverInterface::ReleaseStreamForApplication.
		**/
		AJA_VIRTUAL bool ReleaseStreamForApplication (const ULWord inAppType, const int32_t inProcessID);
	
		AJA_VIRTUAL bool SetStreamingApplication (const ULWord inAppType, const int32_t inProcessID);
		AJA_VIRTUAL bool GetStreamingApplication (ULWord & outAppType, int32_t & outProcessID);

		/**
			@brief		Associates this device as the default or "native" device to use with the given host process.
			@result		True if successful; otherwise false.
			@param[in]	inProcessID		Specifies the OS-specific process identifier that uniquely identifies a running
										process on the host machine.
			@note		Not implemented on Windows or Linux.
			@see		CNTV2DriverInterface::IsDefaultDeviceForPID
		**/
		AJA_VIRTUAL bool SetDefaultDeviceForPID (const int32_t inProcessID)		{(void)inProcessID; return false;}

		/**
			@return		True if the given host process is associated with this device;  otherwise false.
			@param[in]	inProcessID		Specifies the OS-specific process identifier that uniquely identifies a running
										process on the host machine.
			@note		Not implemented on Windows or Linux.
			@see		CNTV2DriverInterface::SetDefaultDeviceForPID
		**/
		AJA_VIRTUAL bool IsDefaultDeviceForPID (const int32_t inProcessID)		{(void)inProcessID; return false;}
	///@}

		AJA_VIRTUAL bool			ReadRP188Registers (const NTV2Channel inChannel, RP188_STRUCT * pRP188Data);
		AJA_VIRTUAL std::string		GetHostName (void) const;	///< @return	String containing the name of the host (if connected to a remote device) or the name of the non-physical device;  otherwise an empty string.
		AJA_VIRTUAL bool			IsRemote (void) const;	///< @return	True if I'm connected to a non-local or non-physical device;  otherwise false.
#if defined (NTV2_NUB_CLIENT_SUPPORT)
		AJA_VIRTUAL inline NTV2NubProtocolVersion	GetNubProtocolVersion (void) const	{return _pRPCAPI ? _pRPCAPI->NubProtocolVersion() : ntv2NubProtocolVersionNone;}	///< @return	My nub protocol version.
#endif

		//	DEPRECATED FUNCTIONS	
#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED_f(NTV2BoardType	GetCompileFlag (void));
		AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool BoardOpened (void) const)		{return IsOpen();}
#endif	//	!NTV2_DEPRECATE
#if !defined(NTV2_DEPRECATE_12_7)
		// For devices that support more than one bitfile
		AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool SwitchBitfile (NTV2DeviceID boardID, NTV2BitfileType bitfile))	{ (void) boardID; (void) bitfile; return false; }	///< @deprecated	This function is obsolete.
#endif
#if !defined(NTV2_DEPRECATE_14_3)
		AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool	ReadRegister (const ULWord inRegNum, ULWord * pOutValue, const ULWord inRegMask = 0xFFFFFFFF, const ULWord inRegShift = 0x0))
		{
			return pOutValue ? ReadRegister(inRegNum, *pOutValue, inRegMask, inRegShift) : false;
		}
		AJA_VIRTUAL bool	Open (const UWord inDeviceIndex, const bool inDisplayError, const NTV2DeviceType eDeviceType, const char * pInHostName);
#endif	//	!defined(NTV2_DEPRECATE_14_3)
#if !defined(NTV2_DEPRECATE_15_0)
		AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool IsKonaIPDevice(void))	{return IsIPDevice();}	///< @deprecated	Call CNTV2Card::IsIPDevice instead.
#endif //	!defined(NTV2_DEPRECATE_15_0)
#if !defined(NTV2_DEPRECATE_15_1)
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool SetOutputTimecodeOffset(ULWord frames))	{return WriteRegister(kVRegOutputTimecodeOffset, frames);}	///< @deprecated	Obsolete starting after SDK 15.0.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool GetOutputTimecodeOffset(ULWord* pFrames))	{return pFrames ? ReadRegister(kVRegOutputTimecodeOffset, *pFrames) : false;}	///< @deprecated	Obsolete starting after SDK 15.0.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool SetOutputTimecodeType(ULWord type))		{return WriteRegister(kVRegOutputTimecodeType, type);}	///< @deprecated	Obsolete starting after SDK 15.0.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool GetOutputTimecodeType(ULWord* pType))		{return pType ? ReadRegister(kVRegOutputTimecodeType, *pType) : false;}	///< @deprecated	Obsolete starting after SDK 15.0.
#endif	//	!defined(NTV2_DEPRECATE_15_1)
#if !defined(NTV2_DEPRECATE_15_6)
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool LockFormat(void))	{return false;}	///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool StartDriver(DriverStartPhase phase))		{(void)phase; return false;}	///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool SetUserModeDebugLevel(ULWord level))		{(void)level; return false;}	///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool GetUserModeDebugLevel(ULWord*level))		{(void)level; return false;}	///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool SetKernelModeDebugLevel(ULWord level))	{(void)level; return false;}	///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool GetKernelModeDebugLevel(ULWord* level))	{(void)level; return false;}	///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool SetUserModePingLevel(ULWord level))		{(void)level; return false;}	///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool GetUserModePingLevel(ULWord* level))		{(void)level; return false;}	///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool SetKernelModePingLevel(ULWord level))		{(void)level; return false;}	///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool GetKernelModePingLevel(ULWord* level))	{(void)level; return false;}	///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool SetLatencyTimerValue (ULWord value))		{(void)value; return false;}	///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool GetLatencyTimerValue (ULWord* value))		{(void)value; return false;}	///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool SetDebugFilterStrings(const char* inclStr, const char* exclStr))	{(void)inclStr; (void)exclStr; return false;}	///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool GetDebugFilterStrings(char* inclStr, char* exclStr))	{(void)inclStr; (void)exclStr; return false;}	///< @deprecated	Obsolete starting after SDK 15.5.
#endif	//	!defined(NTV2_DEPRECATE_15_6)
#if !defined(NTV2_DEPRECATE_16_0)
		// SuspendAudio/ResumeAudio were only implemented on MacOS
		AJA_VIRTUAL inline NTV2_SHOULD_BE_DEPRECATED(bool SuspendAudio(void))	{return true;}
		AJA_VIRTUAL inline NTV2_SHOULD_BE_DEPRECATED(bool ResumeAudio(const ULWord inFBSize))	{(void) inFBSize; return true;}
		//	Memory Mapping/Unmapping
		AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool MapFrameBuffers(void))	{return false;}	///< @deprecated	Obsolete starting in SDK 16.0.
		AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool UnmapFrameBuffers(void))	{return true;}	///< @deprecated	Obsolete starting in SDK 16.0.
		AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool MapRegisters(void))		{return false;}	///< @deprecated	Obsolete starting in SDK 16.0.
		AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool UnmapRegisters(void))	{return true;}	///< @deprecated	Obsolete starting in SDK 16.0.
		AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool MapXena2Flash(void))		{return false;}	///< @deprecated	Obsolete starting in SDK 16.0.
		AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool UnmapXena2Flash(void))	{return true;}	///< @deprecated	Obsolete starting in SDK 16.0.
		//	Others
		AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool DmaUnlock(void))			{return false;}	///< @deprecated	Obsolete starting in SDK 16.0.
		AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool CompleteMemoryForDMA(ULWord * pHostBuffer))	{(void)pHostBuffer; return false;}	///< @deprecated	Obsolete starting in SDK 16.0.
		AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool PrepareMemoryForDMA(ULWord * pHostBuffer, const ULWord inNumBytes))	{(void)pHostBuffer; (void)inNumBytes; return false;}	///< @deprecated	Obsolete starting in SDK 16.0.
		AJA_VIRTUAL inline NTV2_SHOULD_BE_DEPRECATED(bool GetInterruptCount(const INTERRUPT_ENUMS eInt, ULWord *pCnt))	{return pCnt ? GetInterruptCount(eInt, *pCnt) : false;}	///< @deprecated	Use version of this function that accepts a non-const reference.
		AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool ReadRegisterMulti(const ULWord numRegs, ULWord * pOutWhichRegFailed, NTV2RegInfo aRegs[]));	///< @deprecated	Use CNTV2DriverInterface::ReadRegisters instead.
		AJA_VIRTUAL inline NTV2_SHOULD_BE_DEPRECATED(ULWord	GetPCISlotNumber(void) const)	{return _pciSlot;}			///< @deprecated	Obsolete starting in SDK 16.0.
		AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(Word SleepMs(const LWord msec));	///< @deprecated	Obsolete starting in SDK 16.0. Use AJATime::Sleep instead.
		AJA_VIRTUAL inline NTV2_SHOULD_BE_DEPRECATED(ULWord	GetAudioFrameBufferNumber(void) const)	{return GetNumFrameBuffers() - 1;}	///< @deprecated	Obsolete starting in SDK 16.0.
#endif	//	!defined(NTV2_DEPRECATE_16_0)

#if defined(NTV2_WRITEREG_PROFILING)	//	Register Write Profiling
		/**
			@name	WriteRegister Profiling
		**/
		///@{
		AJA_VIRTUAL bool			GetRecordedRegisterWrites (NTV2RegisterWrites & outRegWrites) const;	///< @brief	Answers with the recorded register writes.
		AJA_VIRTUAL bool			StartRecordRegisterWrites (const bool inSkipActualWrites = false);	///< @brief	Starts recording all WriteRegister calls.
		AJA_VIRTUAL bool			IsRecordingRegisterWrites (void) const;		///< @return	True if WriteRegister calls are currently being recorded (and not paused);  otherwise false.
		AJA_VIRTUAL bool			StopRecordRegisterWrites (void);			///< @brief		Stops recording all WriteRegister calls.
		AJA_VIRTUAL bool			PauseRecordRegisterWrites (void);			///< @brief		Pauses recording WriteRegister calls.
		AJA_VIRTUAL bool			ResumeRecordRegisterWrites (void);			///< @brief		Resumes recording WriteRegister calls (after a prior call to PauseRecordRegisterWrites).
		AJA_VIRTUAL ULWord			GetNumRecordedRegisterWrites (void) const;	///< @return	The number of recorded WriteRegister calls.
		///@}
#endif	//	NTV2_WRITEREG_PROFILING		//	Register Write Profiling


	//	PROTECTED METHODS
	protected:
#if !defined(NTV2_DEPRECATE_12_7)
		AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool DisplayNTV2Error(const char* pInStr))	{(void)pInStr; return false;}	///< @deprecated	This function is obsolete.
#endif
#if defined (NTV2_NUB_CLIENT_SUPPORT)
		/**
			@brief		Peforms the housekeeping details of opening the specified local, remote or software device.
			@param[in]	inURLSpec	Specifies the local, remote or software device to be opened.
			@result		True if successful; otherwise false.
		**/
		AJA_VIRTUAL bool			OpenRemote (const std::string & inURLSpec);
		AJA_VIRTUAL bool			CloseRemote (void);	///< @brief	Releases host resources associated with the remote/special device connection.
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
		AJA_VIRTUAL bool			OpenLocalPhysical (const UWord inDeviceIndex) = 0;	///< @brief	Opens the local/physical device connection.
		AJA_VIRTUAL bool			CloseLocalPhysical (void) = 0;	///< @brief	Releases host resources associated with the local/physical device connection.
		AJA_VIRTUAL bool			ParseFlashHeader (BITFILE_INFO_STRUCT & outBitfileInfo);

		/**
			@brief		Atomically increments the event count tally for the given interrupt type.
			@param[in]	eInterruptType	Specifies the interrupt type of interest.
		**/
		AJA_VIRTUAL void			BumpEventCount (const INTERRUPT_ENUMS eInterruptType);

		/**
			@brief		Initializes my member variables after a successful Open.
		**/
		AJA_VIRTUAL void			FinishOpen (void);
		AJA_VIRTUAL bool			ReadFlashULWord (const ULWord inAddress, ULWord & outValue, const ULWord inRetryCount = 1000);


	//	PRIVATE TYPES
	protected:
		typedef std::vector<ULWord>		_EventCounts;
		typedef std::vector<PULWord>	_EventHandles;


	//	MEMBER DATA
	protected:
		UWord				_boardNumber;			///< @brief	My device index number.
		NTV2DeviceID		_boardID;				///< @brief	My cached device ID.
		bool				_boardOpened;			///< @brief	True if I'm open and connected to the device.
#if defined(NTV2_WRITEREG_PROFILING)
		bool				mRecordRegWrites;		///< @brief	True if recording; otherwise false when not recording
		bool				mSkipRegWrites;			///< @brief	True if actual register writes are skipped while recording
#endif	//	NTV2_WRITEREG_PROFILING
		ULWord				_programStatus;
#if defined (NTV2_NUB_CLIENT_SUPPORT)
		NTV2RPCAPI *		_pRPCAPI;				///< @brief	Points to remote or software device interface; otherwise NULL for local physical device.
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
		_EventHandles		mInterruptEventHandles;	///< @brief	For subscribing to each possible event, one for each interrupt type
		_EventCounts		mEventCounts;			///< @brief	My event tallies, one for each interrupt type. Note that these
#if defined(NTV2_WRITEREG_PROFILING)
		NTV2RegisterWrites	mRegWrites;				///< @brief	Stores WriteRegister data
		mutable AJALock		mRegWritesLock;			///< @brief	Guard mutex for mRegWrites
#endif	//	NTV2_WRITEREG_PROFILING
#if !defined(NTV2_DEPRECATE_16_0)
		ULWord *			_pFrameBaseAddress;			///< @deprecated	Obsolete starting in SDK 16.0.
		ULWord *			_pRegisterBaseAddress;		///< @deprecated	Obsolete starting in SDK 16.0.
		ULWord				_pRegisterBaseAddressLength;///< @deprecated	Obsolete starting in SDK 16.0.
		ULWord *			_pXena2FlashBaseAddress;	///< @deprecated	Obsolete starting in SDK 16.0.
		ULWord *			_pCh1FrameBaseAddress;		///< @deprecated	Obsolete starting in SDK 16.0.
		ULWord *			_pCh2FrameBaseAddress;		///< @deprecated	Obsolete starting in SDK 16.0.
#endif	//	!defined(NTV2_DEPRECATE_16_0)
		ULWord				_ulNumFrameBuffers;
		ULWord				_ulFrameBufferSize;
		ULWord				_pciSlot;					//	DEPRECATE!

};	//	CNTV2DriverInterface

#endif	//	NTV2DRIVERINTERFACE_H
