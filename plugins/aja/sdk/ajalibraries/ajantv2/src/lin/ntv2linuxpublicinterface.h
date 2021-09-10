/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2linuxpublicinterface.h
	@brief		Types and defines shared between NTV2 user application interface and Linux device driver.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/
#ifndef NTV2LINUXPUBLICINTERFACE_H
#define NTV2LINUXPUBLICINTERFACE_H

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2publicinterface.h"

#define NTV2_DEVICE_TYPE               0xBB

#define NTV2_LINUX_DRIVER_VERSION  	NTV2DriverVersionEncode(AJA_NTV2_SDK_VERSION_MAJOR, AJA_NTV2_SDK_VERSION_MINOR, AJA_NTV2_SDK_VERSION_POINT, AJA_NTV2_SDK_BUILD_NUMBER)


// Read,Write IOCTL's
    // Input:  REGISTER_ACCESS structure
    // Output: NONE
#define IOCTL_NTV2_WRITE_REGISTER \
            _IOW(NTV2_DEVICE_TYPE, 48, REGISTER_ACCESS)

    // Input:  REGISTER_ACCESS structure
    // Output: REGISTER_ACCESS structure
#define IOCTL_NTV2_READ_REGISTER \
            _IOWR(NTV2_DEVICE_TYPE, 49 , REGISTER_ACCESS)

// DMA Ioctls use METHOD_IN_DIRECT ... thus special macro NTV2_MAKE_IN_DIRECT_IOCTL
#define IOCTL_NTV2_DMA_READ_FRAME \
            _IOW(NTV2_DEVICE_TYPE, 146,NTV2_DMA_CONTROL_STRUCT)

#define IOCTL_NTV2_DMA_WRITE_FRAME \
            _IOW(NTV2_DEVICE_TYPE, 147,NTV2_DMA_CONTROL_STRUCT)

#define IOCTL_NTV2_DMA_READ_FRAME_SEGMENT \
            _IOW(NTV2_DEVICE_TYPE, 148,NTV2_DMA_SEGMENT_CONTROL_STRUCT)

#define IOCTL_NTV2_DMA_WRITE_FRAME_SEGMENT \
            _IOW(NTV2_DEVICE_TYPE, 149,NTV2_DMA_SEGMENT_CONTROL_STRUCT)

// DMA Ioctls use METHOD_IN_DIRECT ... thus special macro NTV2_MAKE_IN_DIRECT_IOCTL
#define IOCTL_NTV2_DMA_READ \
            _IOW(NTV2_DEVICE_TYPE, 175,NTV2_DMA_CONTROL_STRUCT)

#define IOCTL_NTV2_DMA_WRITE \
            _IOW(NTV2_DEVICE_TYPE, 176,NTV2_DMA_CONTROL_STRUCT)

#define IOCTL_NTV2_DMA_READ_SEGMENT \
            _IOW(NTV2_DEVICE_TYPE, 177,NTV2_DMA_SEGMENT_CONTROL_STRUCT)

#define IOCTL_NTV2_DMA_WRITE_SEGMENT \
            _IOW(NTV2_DEVICE_TYPE, 178,NTV2_DMA_SEGMENT_CONTROL_STRUCT)

#define IOCTL_NTV2_DMA_P2P \
            _IOW(NTV2_DEVICE_TYPE, 179,NTV2_DMA_P2P_CONTROL_STRUCT)


// Interrupt control (enable, disable, get count)
//
#define IOCTL_NTV2_INTERRUPT_CONTROL \
            _IOW(NTV2_DEVICE_TYPE, 220, NTV2_INTERRUPT_CONTROL_STRUCT)

// Wait for interrupt
//
#define IOCTL_NTV2_WAITFOR_INTERRUPT \
            _IOW(NTV2_DEVICE_TYPE, 221, NTV2_WAITFOR_INTERRUPT_STRUCT)

// Control debug messages.
//
#define IOCTL_NTV2_CONTROL_DRIVER_DEBUG_MESSAGES \
            _IOW(NTV2_DEVICE_TYPE, 230, NTV2_CONTROL_DRIVER_DEBUG_MESSAGES_STRUCT)

// Put board interrupts and LEDs in known state
//
#define IOCTL_NTV2_SETUP_BOARD \
            _IO(NTV2_DEVICE_TYPE, 231)

// Reload procamp hardware registers from software copy after ADC chip reset
#define IOCTL_NTV2_RESTORE_HARDWARE_PROCAMP_REGISTERS \
            _IO(NTV2_DEVICE_TYPE, 232)

//
// Downloaded Xilinx bitfile management IOCTLs
//
//
#define IOCTL_NTV2_SET_BITFILE_INFO \
            _IOWR(NTV2_DEVICE_TYPE, 240, BITFILE_INFO_STRUCT)

#define IOCTL_NTV2_GET_BITFILE_INFO \
            _IOWR(NTV2_DEVICE_TYPE, 241, BITFILE_INFO_STRUCT)

//
// Autocirculate IOCTLs
//
//
#define IOCTL_NTV2_AUTOCIRCULATE_CONTROL \
            _IOW(NTV2_DEVICE_TYPE, 250, AUTOCIRCULATE_DATA)

#define IOCTL_NTV2_AUTOCIRCULATE_STATUS \
            _IOWR(NTV2_DEVICE_TYPE, 251, AUTOCIRCULATE_STATUS_STRUCT)

#define IOCTL_NTV2_AUTOCIRCULATE_FRAMESTAMP \
            _IOWR(NTV2_DEVICE_TYPE, 252, AUTOCIRCULATE_FRAME_STAMP_COMBO_STRUCT)

#define IOCTL_NTV2_AUTOCIRCULATE_TRANSFER \
            _IOWR(NTV2_DEVICE_TYPE, 253, AUTOCIRCULATE_TRANSFER_COMBO_STRUCT)

#define IOCTL_NTV2_AUTOCIRCULATE_CAPTURETASK \
            _IOWR(NTV2_DEVICE_TYPE, 254, AUTOCIRCULATE_FRAME_STAMP_COMBO_STRUCT)

#define IOCTL_AJANTV2_MESSAGE \
            _IOWR(NTV2_DEVICE_TYPE, 255, AUTOCIRCULATE_STATUS)

//
// UART Write/Read IOCTLs
//
#define IOCTL_NTV2_WRITE_UART_TX \
            _IOWR(NTV2_DEVICE_TYPE, 201, NTV2_UART_STRUCT)

#if 0							/* not yet */
#define IOCTL_NTV2_READ_UART_RX \
            _IOWR(NTV2_DEVICE_TYPE, 202, NTV2_UART_STRUCT)
#endif


// When adding new IOCTLs, note that the number is only 8 bits.
// So don't go above 255.

// Structure used to request an address range
typedef struct _MAP_MEMORY {
    void*    Address;
    ULWord   Length;
} MAP_MEMORY, *PMAP_MEMORY;


typedef struct {
    ULWord	RegisterNumber;
    ULWord	RegisterValue;
	ULWord	RegisterMask;
	ULWord	RegisterShift;
} REGISTER_ACCESS, *PREGISTER_ACCESS;

// Virtual registers
#include "ntv2virtualregisters.h"

// Structure used to request a DMA transfer
typedef struct
{
	NTV2DMAEngine	 engine;
	NTV2Channel      dmaChannel;
	ULWord           frameNumber;      // 0-NUM_FRAMEBUFFERS-1
	PULWord          frameBuffer;      // if small integer, then interpreted as DMA driver buffer # in
	//    driver(i.e. GetDMABufferAddress)
	ULWord           frameOffsetSrc;   // For Audio DMA, we want to write from a specific location within the 'frame'
	ULWord           frameOffsetDest;  // For Audio DMA, we want to write to a specific location within the 'frame'
	ULWord           numBytes;         // number of bytes to transfer
	ULWord           downSample;       // applies only to KHD, used for 1/4 size preview
	ULWord           linePitch;        // applies only if downSample true, gets every linePitch lines
	ULWord           poll;             // if poll = true it doesn't block, needs to be checked manually
} NTV2_DMA_CONTROL_STRUCT, *P_NTV2_DMA_CONTROL_STRUCT;

// Structure used to request a DMA transfer
typedef struct
{
	NTV2DMAEngine    engine;
	NTV2Channel      dmaChannel;
	ULWord           frameNumber;      // 0-NUM_FRAMEBUFFERS-1
	PULWord          frameBuffer;      // if small integer, then interpreted as DMA driver buffer # in
	//    driver(i.e. GetDMABufferAddress)
	ULWord           frameOffsetSrc;   // For Audio DMA, we want to write from a specific location within the 'frame'
	ULWord           frameOffsetDest;  // For Audio DMA, we want to write to a specific location within the 'frame'
	ULWord           numBytes;         // number of bytes to transfer
	ULWord           poll;             // if poll = true it doesn't block, needs to be checked manually
	ULWord           videoNumSegments;  // partial segment transfer - number of segment of size numBytes
	ULWord           videoSegmentHostPitch;	//
	ULWord           videoSegmentCardPitch;	//
} NTV2_DMA_SEGMENT_CONTROL_STRUCT, *P_NTV2_DMA_SEGMENT_CONTROL_STRUCT;

// Structure used to request a P2P transfer
typedef struct {
	bool			bRead;					// True for a Target operation, false for a Transfer
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
} NTV2_DMA_P2P_CONTROL_STRUCT, *P_NTV2_DMA_P2P_CONTROL_STRUCT;

// Structure to enable/disable interrupts
typedef struct
{
   INTERRUPT_ENUMS	eInterruptType;	// Which interrupt to enable/disable.  Not all interrupts supported.
   ULWord			enable;			// 0: disable, nonzero: enable
   ULWord			interruptCount;	// In: interrupt type.  Out: interrupt count
} NTV2_INTERRUPT_CONTROL_STRUCT, *P_NTV2_INTERRUPT_CONTROL_STRUCT;

// Structure to wait for interrupts
typedef struct
{
   INTERRUPT_ENUMS	eInterruptType;	// Which interrupt to wait on.
   ULWord			timeOutMs;		// Timeout in milliseconds
   ULWord			success;		// On return, nonzero if interrupt occured
} NTV2_WAITFOR_INTERRUPT_STRUCT, *P_NTV2_WAITFOR_INTERRUPT_STRUCT;

// Structure to control driver debug messages
typedef struct
{
	NTV2_DriverDebugMessageSet	msgSet;	//	Which set of message to turn on/off
	bool						enable;	//	If true, turn messages on
	ULWord						success;// On return, nonzero on failure
} NTV2_CONTROL_DRIVER_DEBUG_MESSAGES_STRUCT, *P_NTV2_CONTROL_DRIVER_DEBUG_MESSAGES_STRUCT;

typedef struct
{
	NTV2Crosspoint							channelSpec;// Which channel
	AUTOCIRCULATE_TRANSFER_STRUCT			acTransfer;	// Transfer ctl
	AUTOCIRCULATE_TRANSFER_STATUS_STRUCT	acStatus;	// Results
	NTV2RoutingTable						acXena2RoutingTable;	// Xena2 crosspoint table
	AUTOCIRCULATE_TASK_STRUCT				acTask;		// driver tasks
} AUTOCIRCULATE_TRANSFER_COMBO_STRUCT, *P_AUTOCIRCULATE_TRANSFER_COMBO_STRUCT;

typedef struct
{
	FRAME_STAMP_STRUCT			acFrameStamp;	// frame stamp
	AUTOCIRCULATE_TASK_STRUCT	acTask;			// driver tasks
} AUTOCIRCULATE_FRAME_STAMP_COMBO_STRUCT, *P_AUTOCIRCULATE_FRAME_STAMP_COMBO_STRUCT;

/*
typedef struct
{
	ULWord  requestedByteCount;  // number of bytes to write OR MAX number of bytes to read
	ULWord  actualByteCount;	 // actual number of byte successfully written or read
	UByte bytes[];				 // WRITE: an array of bytes to write of size requestedByteCount
	                             // READ:  an empty array of MAX size  requestedByteCount into which the read goes
} NTV2_UART_STRUCT, *P_NTV2_UART_STRUCT;
*/

// Driver buffers are numbered 0 .. whatever
#define ntv2DMADriverbuffer(n)	(n)


#define IOCTL_HEVC_MESSAGE \
            _IOWR(NTV2_DEVICE_TYPE, 120, unsigned long)

#endif
