/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2hevcfirmwareinstallerthread.h
	@brief		Declaration of CNTV2HEVCFirmwareInstallerThread class.
	@copyright	(C) 2015-2021 AJA Video Systems, Inc.  All rights reserved.
**/
#ifndef __NTV2HEVCFIRMWAREINSTALLERTHREAD_H__
#define __NTV2HEVCFIRMWAREINSTALLERTHREAD_H__

#include "ntv2card.h"
#include "ntv2devicescanner.h"
#include "ajabase/system/thread.h"

// Bar offsets
#define BAR0_OFFSET						(0xF1E00000)
#define BAR2_OFFSET						(0x08000000)
#define BAR5_OFFSET						(0xF0000000)

// PCIE LINK Register
#define PCIE_BOOT_FLAG                  (0x00000000)

// HSIO SYSOC Register
#define HSIO_SYSOC_BASE_ADDR            (0xF1E00000)
#define PCIE_INT_REG_SET_OFFSET         (0x00000090)

// SRAM
#define SRAM_BASE_ADDR                  (0x01000000)
#define SRAM_BOOT_OFFSET                (0x00000000)
#define SRAM_LOAD_OFFSET                (0x00000400)
#define SRAM_DATA_OFFSET                (0x00004000)
#define SRAM_STAT_FLAG                  (0x00000004)
#define SRAM_RESET_OFFSET               (0x00003E00)
#define SRAM_MODE_FLAG                  (0x00003F00)
#define SRAM_STOP_FLAG                  (0x00003F10)
#define SRAM_CMND_FLAG                  (0x00003F20)
#define SRAM_LOAD_FLAG                  (0x00003F30)
#define SRAM_BOOT_MAX_SIZE              (0x00000400)
#define SRAM_LOAD_MAX_SIZE              (0x00003000)
#define SRAM_DATA_MAX_SIZE              (0x00018000)
#define SRAM_BOOTCHECK_ADDR             (0xFFF69020)

// DRAM
#define DRAM_PARM_OFFSET                (0x00005000)
#define DRAM_PARM_MAX_SIZE              (0x00001000)

#define SECTOR_SIZE                     (128 << 10)
#define WR_DATA_SIZE                    (0x200)
#define WR_FUNC_SIZE                    (0x2CC)
#define RD_DATA_SIZE                    (0x40)
#define RD_FUNC_SIZE                    (0x0C + 0x40)
#define RD_FUNC_DATA                    (0x0C)

#define PERCENT_PROGRESS_START()        do { printf("  0%%  "); fflush(stdout); pp = 0; } while (0)
#define PERCENT_PROGRESS()              do { px = (double)(dataSize - remainSize) * 100 / dataSize; if (px > pp) { printf("\r%3d%%  ", px); fflush(stdout); pp = px; } } while (0)
#define PERCENT_PROGRESS_END()          do { printf("\r100%%\n"); fflush(stdout); } while (0)

typedef enum
{
    HEVC_ERR_NONE,
    HEVC_ERR_OPEN,
    HEVC_ERR_READ,
    HEVC_ERR_WRITE,
    HEVC_ERR_SIZE,
    HEVC_ERR_VERIFY,
    HEVC_ERR_NOT_BOOTED,
    HEVC_ERR_BOOT,
    HEVC_ERR_STOP,
    HEVC_ERR_RECOVERED,
    HEVC_ERR_LOAD,
    HEVC_ERR_OFFSET,
    HEVC_ERR_DEVICEID,
    HEVC_ERR_NOT_LOADED,
    HEVC_ERR_ALREADY_LOADED,
    HEVC_ERR_BUSY,
    HEVC_ERR_ERASE,
    HEVC_ERR_PROGRAM,
    HEVC_ERR_STATE
} HEVCError;

// Flash Parameters
typedef struct
{
    char device_name[64];
    uint32_t device_id[2];
    uint32_t device_size;
} M31FlashParams;

class CNTV2HEVCFirmwareInstallerThread : public AJAThread
{
	//	Instance Methods
	public:
    
        virtual AJAStatus							ThreadInit (void);
    
		/**
			@brief		Constructs me for a given device and bitfile.
			@param[in]	inDeviceInfo	Specifies the device to be flashed.
			@param[in]	inBitfilePath	Specifies the path (relative or absolute) to the firmware bitfile to install.
			@param[in]	inVerbose		If true (the default), emit detailed messages to the standard output stream when
										flashing has started and when it has successfully completed; otherwise, don't.
		**/
		explicit									CNTV2HEVCFirmwareInstallerThread (const NTV2DeviceInfo & inDeviceInfo,
																				  const std::string & inBitfilePath,
																				  const bool inVerbose = true);
		virtual inline								~CNTV2HEVCFirmwareInstallerThread ()				{}

		/**
			@brief		Starts the thread to erase, flash and verify the firmware update.
			@return		AJA_STATUS_SUCCESS if successful; otherwise other values upon failure.
		**/
		virtual AJAStatus							ThreadRun (void);

		//	Accessors
		/**
			@brief		Returns a constant reference to the bitfile path that was specified when I was constructed.
			@return		A constant reference to a string containing the bitfile path that was specified when I was constructed.
		**/
		virtual inline const std::string &			GetBitfilePath (void) const				{return m_bitfilePath;}

		/**
			@brief		Returns true only if the device was successfully flashed.
			@return		True only if the device was successfully flashed.
		**/
		virtual inline bool							IsUpdateSuccessful (void) const			{return m_updateSuccessful;}

		/**
			@brief		Returns a string containing a human-readable status message based on the current installation state.
			@return		A string containing a human-readable status message based on the current installation state.
		**/
		virtual std::string							GetStatusString (void) const;

		/**
			@brief		Returns the current progress value of the current in-progress installation. To compute a percentage,
						divide the value returned from this function by the value returned by GetProgressMax().
			@return		An integer value representing the current progress of the current in-progress installation.
			@note		This function should only be called if the thread is still active.
		**/
		virtual uint32_t							GetProgressValue (void) const;

		/**
			@brief		Returns the maximum progress value of the current in-progress installation. To compute a percentage,
						divide the value returned from GetProgressValue() by the value returned by this function.
			@return		An integer value representing the current progress percentage of the current in-progress installation.
			@note		This function should only be called if the thread is still active.
		**/
		virtual uint32_t							GetProgressMax (void) const;

	protected:
		virtual void								InternalUpdateStatus (void) const;

	private:
													CNTV2HEVCFirmwareInstallerThread ();											//	hidden
													CNTV2HEVCFirmwareInstallerThread (const CNTV2HEVCFirmwareInstallerThread & inObj);	//	hidden
		virtual CNTV2HEVCFirmwareInstallerThread &		operator = (const CNTV2HEVCFirmwareInstallerThread & inObj);					//	hidden
    
    // Support routines, moved over from the Fujitsu maintenance application.
    
    private:
        bool                WriteVerify32(uint32_t offset, uint32_t val);
        uint32_t            Flash16WaitReady(uint32_t barOffset, uint32_t pos);
        uint32_t            Flash16ReadDeviceId(uint32_t barOffset, uint32_t pos);
        uint32_t            Flash16ChipErase(uint32_t barOffset, uint32_t pos);
        uint32_t            Flash16SectorErase(uint32_t barOffset, uint32_t pos, uint32_t off);
        uint32_t            Flash16WriteBuffer512(uint32_t barOffset, uint32_t pos, uint32_t off, uint32_t *buf);
        uint32_t            Flash16Read64(uint32_t barOffset, uint32_t pos, uint32_t off);
        uint32_t            ReadDeviceId(uint32_t barOffset, uint32_t pos);
        void                IdentifyFlash(uint32_t barOffset, M31FlashParams *flash);
    
    public:
        uint32_t            StatusCheck(uint32_t address, uint32_t checkValue, uint32_t timeout, bool isEqual, bool dotUpdate);
        bool                IsBooted();
        HEVCError           Check();
        HEVCError           Boot();
        HEVCError           Stop();
        HEVCError           LoadFlash();
        HEVCError           ChipErase();
        HEVCError           SectorErase(uint32_t offset, uint32_t dataSize);
        HEVCError           Program(char* fileName, uint32_t flashOffset, uint32_t fileOffset, uint32_t length);
        HEVCError           FlashDump(char* fileName, uint32_t offset, uint32_t dataSize);
        HEVCError           LoadDram();
        HEVCError           DramTest(uint32_t offset, uint32_t size, uint32_t blockSize, uint32_t buf0, uint32_t buf1);
        HEVCError           BinVerify(char* fileName, uint32_t fileOffset, uint32_t compareSize, uint32_t bar, uint32_t offset, uint32_t timeout);
        HEVCError           FlashMainModeCommon();
    
        HEVCError           FlashMode(char* fileName, uint32_t offset, uint32_t versionOffset);
        HEVCError           FlashMCPU(char* fileName, uint32_t reset);
        HEVCError           FlashSystem(char* fileName, char* configName, ULWord flashOffset, ULWord fileOffset, ULWord length);
		bool 				WriteRegisterBar4(ULWord address, ULWord value, ULWord mask = 0xffffffff, ULWord shift = 0);

	//	Instance Data
	protected:
		const NTV2DeviceInfo						m_deviceInfo;				///<	Device info passed to me at construction
		std::string									m_bitfilePath;				///<	Absolute path to bitfile on host that's to be flashed into device
		bool										m_updateSuccessful;			///<	Initially False, is set True if firmware successfully installed
		const bool									m_verbose;					///<	Verbose logging to cout/cerr?
		mutable CNTV2Card							m_device;					///<	Talks to the AJA device
		mutable SSC_GET_FIRMWARE_PROGRESS_STRUCT	m_statusStruct;				///<	Firmware update progress
    
        uint64_t                                    m_updateTime;
        uint32_t                                    m_dataBuf[SRAM_DATA_MAX_SIZE / 4];
        uint32_t                                    m_tempBuf[SRAM_DATA_MAX_SIZE / 4];
        M31FlashParams                              m_flash;
    
        HevcDeviceInfo                              m_hevcInfo;
};	//	HEVCFirmwareInstallerThread


#endif	//	__NTV2HEVCFIRMWAREINSTALLERTHREAD_H__
