/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2winpublicinterface.h
	@brief		Defines & structs shared between user-space and Windows kernel driver.
	@copyright	(C) 2003-2021 AJA Video Systems, Inc.
**/
#ifndef NTV2WINPUBLICINTERFACE_H
#define NTV2WINPUBLICINTERFACE_H


//This file includes all the necessary defines, enums etc... to access the 
//AJA Video custom kernel streaming property set(s)

#include "ntv2publicinterface.h"
#include "ks.h"

#ifdef KSD
//{3280A641-5159-4f43-B55D-E05ABB47C350}
#define STATIC_AJAVIDEO_PROPSET\
    0x3280A641L, 0x5159, 0x4f43, 0xB5, 0x5D, 0xE0, 0x5A, 0xBB, 0x47, 0xC3, 0x50
DEFINE_GUIDSTRUCT("3280A641-5159-4f43-B55D-E05ABB47C350", AJAVIDEO_PROPSET);
#define AJAVIDEO_PROPSET DEFINE_GUIDNAMED(AJAVIDEO_PROPSET)



#elif defined KHD

//{84963F56-67FC-461c-8040-CC891B87195B}
#define STATIC_AJAVIDEO_PROPSET\
    0x84963f56, 0x67fc, 0x461c,  0x80, 0x40, 0xcc, 0x89, 0x1b, 0x87, 0x19, 0x5b
DEFINE_GUIDSTRUCT("84963F56-67FC-461c-8040-CC891B87195B", AJAVIDEO_PROPSET);
#define AJAVIDEO_PROPSET DEFINE_GUIDNAMED(AJAVIDEO_PROPSET)

#elif defined XENA2

// {2BFA1669-17F7-4cf9-8E05-500B8CB81497}
#define STATIC_AJAVIDEO_PROPSET\
	0x2bfa1669, 0x17f7, 0x4cf9, 0x8e, 0x5, 0x50, 0xb, 0x8c, 0xb8, 0x14, 0x97
DEFINE_GUIDSTRUCT("2BFA1669-17F7-4cf9-8E05-500B8CB81497", AJAVIDEO_PROPSET);
#define AJAVIDEO_PROPSET DEFINE_GUIDNAMED(AJAVIDEO_PROPSET)

#endif

#define IOCTL_AJAPROPS_GETSETREGISTER			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x600, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AJAPROPS_GETSETLOGLEVEL			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x601, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AJAPROPS_MAPMEMORY				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x602, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AJAPROPS_INTERRUPTS				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x603, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AJAPROPS_SUBSCRIPTIONS			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x604, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AJAPROPS_DMA						CTL_CODE(FILE_DEVICE_UNKNOWN, 0x605, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AJAPROPS_AUTOCIRC_CONTROL			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x606, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AJAPROPS_AUTOCIRC_STATUS			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x607, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AJAPROPS_AUTOCIRC_FRAME			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x608, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AJAPROPS_AUTOCIRC_TRANSFER		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x609, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AJAPROPS_DT_CONFIGURE				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x60A, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AJAPROPS_NEWSUBSCRIPTIONS			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x60B, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AJAPROPS_AUTOCIRC_TRANSFER_EX		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x60C, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AJAPROPS_DMA_EX					CTL_CODE(FILE_DEVICE_UNKNOWN, 0x60D, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AJAPROPS_AUTOCIRC_TRANSFER_EX2	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x60E, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AJAPROPS_AUTOCIRC_FRAME_EX2		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x60F, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AJAPROPS_AUTOCIRC_CAPTURE_TASK	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x610, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AJAPROPS_GETSETBITFILEINFO		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x611, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AJAPROPS_AUTOCIRC_CONTROL_EX		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x612, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AJAPROPS_DMA_P2P					CTL_CODE(FILE_DEVICE_UNKNOWN, 0x613, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AJANTV2_MESSAGE					CTL_CODE(FILE_DEVICE_UNKNOWN, 0x614, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AJAHEVC_MESSAGE					CTL_CODE(FILE_DEVICE_UNKNOWN, 0x615, METHOD_BUFFERED, FILE_ANY_ACCESS)



	// Explicit values are attached to these values,
        // to try to prevent anyone inadvertently changing old members (risking incompatibility with older apps)
        // when adding new members!
typedef enum {
    KSPROPERTY_AJAPROPS_GETSETREGISTER = 0x0, // RW
	KSPROPERTY_AJAPROPS_GETSETLOGLEVEL = 0x01,  // RW
	KSPROPERTY_AJAPROPS_MAPMEMORY = 0x02,	
	KSPROPERTY_AJAPROPS_INTERRUPTS = 0x03,
	KSPROPERTY_AJAPROPS_SUBSCRIPTIONS = 0x04,
	KSPROPERTY_AJAPROPS_DMA = 0x05,
	KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL = 0x06,
	KSPROPERTY_AJAPROPS_AUTOCIRC_STATUS = 0x07,
	KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME = 0x08,
	KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER = 0x09,
	KSPROPERTY_AJAPROPS_DT_CONFIGURE = 0x0A,
	KSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS = 0x0B, // needed for 64 bit windows.
	KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX = 0x0C,
	KSPROPERTY_AJAPROPS_DMA_EX = 0x0D,
	KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX2 = 0x0E,
	KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_EX2 = 0x0F,
	KSPROPERTY_AJAPROPS_AUTOCIRC_CAPTURE_TASK = 0x10,
	KSPROPERTY_AJAPROPS_GETSETBITFILEINFO = 0x011,      // attempt to maintain compatibility between application and driver versions.
	KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_EX = 0x12,
	KSPROPERTY_AJAPROPS_DMA_P2P = 0x13,

        // DURIAN
	KSPROPERTY_AJAPROPS_DT_GET_XENA_DXT_FIRMWARE_DESIRED = 0x20,
	KSPROPERTY_AJAPROPS_DT_ESTABLISH_XENA_DXT_FIRMWARE_HD = 0x021,
	KSPROPERTY_AJAPROPS_DT_ESTABLISH_XENA_DXT_FIRMWARE_SD = 0x022,
	KSPROPERTY_AJAPROPS_DT_GETSETVIDEOCAPTUREPINCONNECTED = 0x023,
	KSPROPERTY_AJAPROPS_DT_GETSETAUDIOCAPTUREPINCONNECTED = 0x024,
	KSPROPERTY_AJAPROPS_DT_GETSETAUDIOMUX0 = 0x025,


} KSPROPERTY_AJAPROPS;

typedef struct { //this is the structure used for the KSPROPERTY_AJAPROPS_GETSETREGISTER property.
    KSPROPERTY Property;
    ULONG      RegisterID;		// ID of the targeted register.
    ULONG      ulRegisterValue; // For Read, set by the driver on the way out, for Write set by the requestor on the way in!
    ULONG      ulRegisterMask;	// For post AND
	ULONG	   ulRegisterShift; // For pre  OR
} KSPROPERTY_AJAPROPS_GETSETREGISTER_S, *PKSPROPERTY_AJAPROPS_GETSETREGISTER_S;

//add any other required structures for other properties here!

// Structure used to request an address range for memory mapping
typedef enum _MAP_MEMORY_TYPE {
	NTV2_MAPMEMORY_FRAMEBUFFER = 0,
	NTV2_MAPMEMORY_REGISTER = 1,
	NTV2_MAPMEMORY_PCIFLASHPROGRAM = 2,
} MAP_MEMORY_TYPE, *PMAP_MEMORY_TYPE;

typedef struct  {
    void*   Address;
    ULONG   Length;
} MAP_MEMORY, *PMAP_MEMORY;

typedef struct {
    void*   POINTER_32 Address;
    ULONG   Length;
} MAP_MEMORY_32, *PMAP_MEMORY_32;

// Memory map property structure - includes MAP_MEMORY structure
typedef struct {
	KSPROPERTY Property;	// boilerplate Properties stuff
	MAP_MEMORY mapMemory;	// set by kernel if mapping, set by user if unmapping
	UByte	   bMapType;	// true if un/map registers (BA0), false if un/map framebuffer (BA1)
} KSPROPERTY_AJAPROPS_MAPMEMORY_S, *PKSPROPERTY_AJAPROPS_MAPMEMORY_S;

// Memory map property structure - includes MAP_MEMORY structure
typedef struct {
	KSPROPERTY Property;	// boilerplate Properties stuff
	MAP_MEMORY_32 mapMemory;	// set by kernel if mapping, set by user if unmapping
	UByte	   bMapType;	// true if un/map registers (BA0), false if un/map framebuffer (BA1)
} KSPROPERTY_AJAPROPS_MAPMEMORY_S_32, *PKSPROPERTY_AJAPROPS_MAPMEMORY_S_32;
	
// Interrupt property structure - includes INTERRUPT_ENUMS
typedef struct {
	KSPROPERTY		Property;	// boilerplate Properties stuff
	ULONG			ulMask;		// used only by GetCurrentInterruptMask()
	INTERRUPT_ENUMS eInterrupt;	// specifies the interrupt type
} KSPROPERTY_AJAPROPS_INTERRUPTS_S, *PKSPROPERTY_AJAPROPS_INTERRUPTS_S;


//////////////////////////////////////////////////////////////////////////////////////
// Subscription property structure - includes INTERRUPT_ENUMS
typedef struct {
	KSPROPERTY		Property;	// boilerplate Properties stuff	
	HANDLE *		pHandle;	// address of userspace created notification event
	INTERRUPT_ENUMS eInterrupt;	// specifies subscription type
	ULONG			ulIntCount;	// set by kernel in method getInterruptCount()
} KSPROPERTY_AJAPROPS_SUBSCRIPTIONS_S, *PKSPROPERTY_AJAPROPS_SUBSCRIPTIONS_S;

typedef struct {
	KSPROPERTY		Property;	// boilerplate Properties stuff	
	HANDLE * POINTER_32		pHandle;	// address of userspace created notification event
	INTERRUPT_ENUMS eInterrupt;	// specifies subscription type
	ULONG			ulIntCount;	// set by kernel in method getInterruptCount()
} KSPROPERTY_AJAPROPS_SUBSCRIPTIONS_S_32, *PKSPROPERTY_AJAPROPS_SUBSCRIPTIONS_S_32;

//////////////////////////////////////////////////////////////////////////////////////
//  New Subscription property structure - includes INTERRUPT_ENUMS
/// Added to support 64 bit driver.
typedef struct {
	KSPROPERTY		Property;	// boilerplate Properties stuff	
	HANDLE			Handle;	// address of userspace created notification event
	INTERRUPT_ENUMS eInterrupt;	// specifies subscription type
	ULONG			ulIntCount;	// set by kernel in method getInterruptCount()
} KSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS_S, *PKSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS_S;

typedef struct {
	KSPROPERTY		Property;	// boilerplate Properties stuff	
	VOID * POINTER_32		Handle;	// address of userspace created notification event
	INTERRUPT_ENUMS eInterrupt;	// specifies subscription type
	ULONG			ulIntCount;	// set by kernel in method getInterruptCount()
} KSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS_S_32, *PKSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS_S_32;

////////////////////////////////////////////////////////////////////////////////////
// DMA 

typedef struct {
	KSPROPERTY		Property;			// boilerplate Properties stuff
	NTV2DMAEngine	dmaEngine;
	PVOID			pvVidUserVa;		// void pointer to user virtual address
	ULWord			ulFrameNumber;
	ULWord			ulVidNumBytes;		// transfer size in bytes
	ULWord			ulFrameOffset;		// offset into frame to start transfer
	PULWord			pulAudUserVa;
	ULWord			ulAudNumBytes;
	ULWord			ulAudStartSample;
	bool			bSync;				// if true, block in kernel until DMA complete
} KSPROPERTY_AJAPROPS_DMA_S, *PKSPROPERTY_AJAPROPS_DMA_S;

typedef struct {
	KSPROPERTY		Property;			// boilerplate Properties stuff
	NTV2DMAEngine	dmaEngine;
	void* POINTER_32	pvVidUserVa;		// void pointer to user virtual address
	ULWord			ulFrameNumber;
	ULWord			ulVidNumBytes;		// transfer size in bytes
	ULWord			ulFrameOffset;		// offset into frame to start transfer
	ULWord* POINTER_32			pulAudUserVa;
	ULWord			ulAudNumBytes;
	ULWord			ulAudStartSample;
	bool			bSync;				// if true, block in kernel until DMA complete
} KSPROPERTY_AJAPROPS_DMA_S_32, *PKSPROPERTY_AJAPROPS_DMA_S_32;

typedef struct {
	KSPROPERTY		Property;			// boilerplate Properties stuff
	NTV2DMAEngine	dmaEngine;
	PVOID			pvVidUserVa;		// void pointer to user virtual address
	ULWord			ulFrameNumber;
	ULWord			ulVidNumBytes;		// transfer size in bytes
	ULWord			ulFrameOffset;		// offset into frame to start transfer
	PULWord			pulAudUserVa;
	ULWord			ulAudNumBytes;
	ULWord			ulAudStartSample;
	ULWord			ulVidNumSegments;
	ULWord			ulVidSegmentHostPitch;
	ULWord			ulVidSegmentCardPitch;
	bool			bSync;				// if true, block in kernel until DMA complete
} KSPROPERTY_AJAPROPS_DMA_EX_S, *PKSPROPERTY_AJAPROPS_DMA_EX_S;

typedef struct {
	KSPROPERTY		Property;			// boilerplate Properties stuff
	NTV2DMAEngine	dmaEngine;
	void* POINTER_32	pvVidUserVa;		// void pointer to user virtual address
	ULWord			ulFrameNumber;
	ULWord			ulVidNumBytes;		// transfer size in bytes
	ULWord			ulFrameOffset;		// offset into frame to start transfer
	ULWord* POINTER_32			pulAudUserVa;
	ULWord			ulAudNumBytes;
	ULWord			ulAudStartSample;
	ULWord			ulVidNumSegments;
	ULWord			ulVidSegmentHostPitch;
	ULWord			ulVidSegmentCardPitch;
	bool			bSync;				// if true, block in kernel until DMA complete
} KSPROPERTY_AJAPROPS_DMA_EX_S_32, *PKSPROPERTY_AJAPROPS_DMA_EX_S_32;

typedef struct {
	KSPROPERTY		Property;				// boilerplate Properties stuff
	NTV2DMAEngine	dmaEngine;				// engine for transfer
	NTV2Channel		dmaChannel;				// frame buffer channel for message
	ULWord			ulFrameNumber;			// frame number for target/transfer
	ULWord			ulFrameOffset;			// offset into frame to start transfer
	ULWord			ulVidNumBytes;			// transfer size in bytes
	ULWord			ulVidNumSegments;		// number of video segments to transfer
	ULWord			ulVidSegmentHostPitch;	// segment host pitch
	ULWord			ulVidSegmentCardPitch;	// segment card pitch
	ULWord64		ullVideoBusAddress;		// frame buffer bus address
	ULWord64		ullMessageBusAddress;	// message register bus address (0 if not required)
	ULWord			ulVideoBusSize;			// size of the video aperture (bytes)
	ULWord			ulMessageData;			// message data (write to message bus address to complete video transfer)
} KSPROPERTY_AJAPROPS_DMA_P2P_S, *PKSPROPERTY_AJAPROPS_DMA_P2P_S;

////////////////////////////////////////////////////////////////////////////////////
// AutoCirculate
////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////
// Control
typedef struct {
	KSPROPERTY			Property;
	AUTO_CIRC_COMMAND	eCommand;
	NTV2Crosspoint		channelSpec;
	LWord				lVal1;
	LWord				lVal2;
    LWord               lVal3;
	bool				bVal1;
	bool				bVal2;
    bool                bVal3;
    bool                bVal4;
    bool                bVal5;
    bool                bVal6;
    bool                bVal7;
    bool                bVal8;
} KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_S, *PKSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_S;

typedef struct {
	KSPROPERTY			Property;
	AUTO_CIRC_COMMAND	eCommand;
	NTV2Crosspoint		channelSpec;
	LWord				lVal1;
	LWord				lVal2;
	LWord               lVal3;
	LWord				lVal4;
	LWord				lVal5;
	LWord				lVal6;
	LWord				lVal7;
	LWord				lVal8;
	LWord				lVal9;
	LWord				lVal10;
	bool				bVal1;
	bool				bVal2;
	bool                bVal3;
	bool                bVal4;
	bool                bVal5;
	bool                bVal6;
	bool                bVal7;
	bool                bVal8;
	bool                bVal9;
	bool                bVal10;
	bool                bVal11;
	bool                bVal12;
	bool                bVal13;
	bool                bVal14;
} KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_EX_S, *PKSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_EX_S;

/////////////////////////////////////////////////////////////////////////////////////
// GetAutoCirculate
typedef struct {
	KSPROPERTY			Property;
	AUTO_CIRC_COMMAND	eCommand;
	NTV2Crosspoint		channelSpec;
	AUTOCIRCULATE_STATUS_STRUCT autoCircStatus;
} KSPROPERTY_AJAPROPS_AUTOCIRC_STATUS_S, *PKSPROPERTY_AJAPROPS_AUTOCIRC_STATUS_S;

/////////////////////////////////////////////////////////////////////////////////////////
// GetFrameStamp
typedef struct {
	KSPROPERTY			Property;
	AUTO_CIRC_COMMAND	eCommand;
	NTV2Crosspoint		channelSpec;
	LWord				lFrameNum;
	FRAME_STAMP_STRUCT  frameStamp;
} KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_S, *PKSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_S;

typedef struct {
	KSPROPERTY						Property;
	AUTO_CIRC_COMMAND				eCommand;
	NTV2Crosspoint					channelSpec;
	LWord							lFrameNum;
	FRAME_STAMP_STRUCT				frameStamp;
	AUTOCIRCULATE_TASK_STRUCT		acTask;
} KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_EX2_S, *PKSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_EX2_S;

typedef struct {
	KSPROPERTY						Property;
	AUTO_CIRC_COMMAND				eCommand;
	NTV2Crosspoint					channelSpec;
	LWord							lFrameNum;
	FRAME_STAMP_STRUCT				frameStamp;
	AUTOCIRCULATE_TASK_STRUCT_32	acTask;
} KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_EX2_S_32, *PKSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_EX2_S_32;

//////////////////////////////////////////////////////////////////////////////////////////////
//  Transfer
typedef struct {
	KSPROPERTY			Property;
	AUTO_CIRC_COMMAND	eCommand;
	AUTOCIRCULATE_TRANSFER_STRUCT			acTransfer;
	AUTOCIRCULATE_TRANSFER_STATUS_STRUCT	acStatus;
} KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_S, *PKSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_S;

typedef struct {
	KSPROPERTY			Property;
	AUTO_CIRC_COMMAND	eCommand;
	AUTOCIRCULATE_TRANSFER_STRUCT_32		acTransfer;
	AUTOCIRCULATE_TRANSFER_STATUS_STRUCT	acStatus;
} KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_S_32, *PKSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_S_32;

//////////////////////////////////////////////////////////////////////////////////////////////
//  TransferEx  - Extended version for Xena2....
typedef struct {
	KSPROPERTY			Property;
	AUTO_CIRC_COMMAND	eCommand;
	AUTOCIRCULATE_TRANSFER_STRUCT			acTransfer;
	AUTOCIRCULATE_TRANSFER_STATUS_STRUCT	acStatus;
	NTV2RoutingTable						acTransferRoute;
} KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX_S, *PKSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX_S;

typedef struct {
	KSPROPERTY			Property;
	AUTO_CIRC_COMMAND	eCommand;
	AUTOCIRCULATE_TRANSFER_STRUCT_32		acTransfer;
	AUTOCIRCULATE_TRANSFER_STATUS_STRUCT	acStatus;
	NTV2RoutingTable						acTransferRoute;
} KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX_S_32, *PKSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX_S_32;

//////////////////////////////////////////////////////////////////////////////////////////////
//  TransferEx2  - Extended version for autocirculate tasks....
typedef struct {
	KSPROPERTY			Property;
	AUTO_CIRC_COMMAND	eCommand;
	AUTOCIRCULATE_TRANSFER_STRUCT			acTransfer;
	AUTOCIRCULATE_TRANSFER_STATUS_STRUCT	acStatus;
	NTV2RoutingTable						acTransferRoute;
	AUTOCIRCULATE_TASK_STRUCT				acTask;
} KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX2_S, *PKSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX2_S;

typedef struct {
	KSPROPERTY			Property;
	AUTO_CIRC_COMMAND	eCommand;
	AUTOCIRCULATE_TRANSFER_STRUCT_32		acTransfer;
	AUTOCIRCULATE_TRANSFER_STATUS_STRUCT	acStatus;
	NTV2RoutingTable						acTransferRoute;
	AUTOCIRCULATE_TASK_STRUCT_32			acTask;
} KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX2_S_32, *PKSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX2_S_32;

// Get/SetBitFileInformation
typedef struct { 
    KSPROPERTY Property;
	BITFILE_INFO_STRUCT bitFileInfoStruct;
} KSPROPERTY_AJAPROPS_GETSETBITFILEINFO_S, *PKSPROPERTY_AJAPROPS_GETSETBITFILEINFO_S;

typedef enum {
	PERFCOUNTER_TIMESTAMP_100NS,
	PERFCOUNTER_TIMESTAMP_RAW,
} PerfCounterTimestampMode;

#endif
