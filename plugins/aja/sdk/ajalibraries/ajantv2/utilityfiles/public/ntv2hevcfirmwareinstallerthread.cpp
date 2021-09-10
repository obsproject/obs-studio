/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2hevcfirmwareinstallerthread.cpp
	@brief		Implementation of CNTV2HEVCFirmwareInstallerThread class.
	@copyright	(C) 2015-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2hevcfirmwareinstallerthread.h"
#include "ntv2bitfile.h"
#include "ntv2utils.h"
#include "ajabase/system/file_io.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/common/timer.h"

#include <sys/stat.h>


using namespace std;


#if defined (AJADebug) || defined (_DEBUG) || defined (DEBUG)
	static const bool	gDebugging	(true);
#else
	static const bool	gDebugging	(false);
#endif


#define	REALLY_UPDATE		true		///<	Set this to false to simulate flashing a device


M31FlashParams m_flashTable[] =
{
    { "S29GL128S",   { 0x7E0001, 0x010021 },  16 << 20 },
    { "S29GL256S",   { 0x7E0001, 0x010022 },  32 << 20 },
    { "S29GL512S",   { 0x7E0001, 0x010023 },  64 << 20 },
    { "S29GL01GS",   { 0x7E0001, 0x010028 }, 128 << 20 },
};


CNTV2HEVCFirmwareInstallerThread::CNTV2HEVCFirmwareInstallerThread (const NTV2DeviceInfo & inDeviceInfo,
															const string & inBitfilePath,
															const bool inVerbose)
	:	m_deviceInfo		(inDeviceInfo),
		m_bitfilePath		(inBitfilePath),
		m_updateSuccessful	(false),
		m_verbose			(inVerbose)
{
    printf("CNTV2HEVCFirmwareInstallerThread verbose = %d\n", inVerbose);
	::memset (&m_statusStruct, 0, sizeof (m_statusStruct));
}

AJAStatus CNTV2HEVCFirmwareInstallerThread::ThreadInit()
{
    m_device.Open (m_deviceInfo.deviceIndex);
	if (!m_device.IsOpen ())
	{
		cerr << "## ERROR:  CNTV2FirmwareInstallerThread: Device not open" << endl;
		return AJA_STATUS_OPEN;
	}
    
    m_device.HevcGetDeviceInfo(&m_hevcInfo);
        
    printf("HEVC PCIe vendor %08x device %08x subVendor %08x subDevice %08x\n",
           m_hevcInfo.pciId.vendor, m_hevcInfo.pciId.device, m_hevcInfo.pciId.subVendor, m_hevcInfo.pciId.subDevice);

    
	return AJA_STATUS_SUCCESS;
}

AJAStatus CNTV2HEVCFirmwareInstallerThread::ThreadRun()
{
	AJAStatus status = AJA_STATUS_SUCCESS;
	bool loop = true;

    m_device.Open (m_deviceInfo.deviceIndex);
	if (!m_device.IsOpen ())
	{
		cerr << "## ERROR:  CNTV2FirmwareInstallerThread:  Device not open" << endl;
		return AJA_STATUS_OPEN;
	}
	if (m_bitfilePath.empty ())
	{
		cerr << "## ERROR:  CNTV2FirmwareInstallerThread:  Bitfile path is empty!" << endl;
		return AJA_STATUS_BAD_PARAM;
	}

    
	// initialize the thread
	status = ThreadInit();
    
	if (status)
	{
		// call the loop until done
		while (loop && !Terminate())
		{
			loop = ThreadLoop();
		}
        
		// flush the thread when done
		status = ThreadFlush();
	}
    
	return status;
}

string CNTV2HEVCFirmwareInstallerThread::GetStatusString (void) const
{
	InternalUpdateStatus ();
	switch (m_statusStruct.programState)
	{
		case kProgramStateEraseMainFlashBlock:		return "Erasing...";
		case kProgramStateEraseSecondFlashBlock:	return gDebugging ? "Erasing second flash block..." : "Erasing...";
		case kProgramStateEraseFailSafeFlashBlock:	return gDebugging ? "Erasing fail-safe..." : "Erasing...";
		case kProgramStateProgramFlash:				return "Programming...";
		case kProgramStateVerifyFlash:				return "Verifying...";
		case kProgramStateFinished:					return "Done";
		default:									return "";
	}		
}

uint32_t CNTV2HEVCFirmwareInstallerThread::GetProgressValue (void) const
{
	InternalUpdateStatus ();
	return m_statusStruct.programProgress;
}


uint32_t CNTV2HEVCFirmwareInstallerThread::GetProgressMax (void) const
{
	InternalUpdateStatus ();
	return m_statusStruct.programTotalSize;
}


void CNTV2HEVCFirmwareInstallerThread::InternalUpdateStatus (void) const
{
	if (REALLY_UPDATE && m_device.IsOpen ())
		m_device.GetProgramStatus (&m_statusStruct);
}


CNTV2HEVCFirmwareInstallerThread::CNTV2HEVCFirmwareInstallerThread ()
	:	m_deviceInfo		(),
		m_bitfilePath		(),
		m_updateSuccessful	(false),
		m_verbose			(false)
{
	assert (false);
}

CNTV2HEVCFirmwareInstallerThread::CNTV2HEVCFirmwareInstallerThread (const CNTV2HEVCFirmwareInstallerThread & inObj)
	:	m_deviceInfo		(),
		m_bitfilePath		(),
		m_updateSuccessful	(false),
		m_verbose			(false)
{
	(void) inObj;
	assert (false);
}

CNTV2HEVCFirmwareInstallerThread & CNTV2HEVCFirmwareInstallerThread::operator = (const CNTV2HEVCFirmwareInstallerThread & inObj)
{
	(void) inObj;
	assert (false);
	return *this;
}


// Support routines, moved over from the Fujitsu maintenance application.

bool CNTV2HEVCFirmwareInstallerThread::WriteVerify32(uint32_t offset, uint32_t val)
{
    uint32_t    chk, n = 0;
    
    m_device.HevcWriteRegister(offset, val);
    do
    {
        n++;
        m_device.HevcReadRegister(offset, &chk);
    } while (chk != val && n < 1000);
    
    if (chk != val)
        return false;
    else
        return true;
}

uint32_t CNTV2HEVCFirmwareInstallerThread::Flash16WaitReady(uint32_t barOffset, uint32_t pos)
{
    WriteVerify32(barOffset + (pos + 0x00), 0x0000000A); // 16bit 1-check
    WriteVerify32(barOffset + (pos + 0x04), 0x00000000); // offset
    WriteVerify32(barOffset + (pos + 0x08), 0x0000000C); // byte count
    WriteVerify32(barOffset + (pos + 0x0C), 0x00000000); // data
    WriteVerify32(barOffset + (pos + 0x10), 0x00000040); // mask bit
    WriteVerify32(barOffset + (pos + 0x14), pos + 0x30); // jump offset DQ6 == 0
    
    WriteVerify32(barOffset + (pos + 0x18), 0x00000009); // 16bit 0-check
    WriteVerify32(barOffset + (pos + 0x1C), 0x00000000); // offset
    WriteVerify32(barOffset + (pos + 0x20), 0x0000000C); // byte count
    WriteVerify32(barOffset + (pos + 0x24), 0x00000000); // data
    WriteVerify32(barOffset + (pos + 0x28), 0x00000040); // mask bit
    WriteVerify32(barOffset + (pos + 0x2C), pos + 0x60); // jump offset DQ6 == 1, no toggle
    
    WriteVerify32(barOffset + (pos + 0x30), 0x0000000A); // 16bit 1-check
    WriteVerify32(barOffset + (pos + 0x34), 0x00000000); // offset
    WriteVerify32(barOffset + (pos + 0x38), 0x0000000C); // byte count
    WriteVerify32(barOffset + (pos + 0x3C), 0x00000000); // data
    WriteVerify32(barOffset + (pos + 0x40), 0x00000040); // mask bit
    WriteVerify32(barOffset + (pos + 0x44), pos + 0x60); // jump offset DQ6 == 0, no toggle
    
    WriteVerify32(barOffset + (pos + 0x48), 0x0000000A); // 16bit 1-check
    WriteVerify32(barOffset + (pos + 0x4C), 0x00000000); // offset
    WriteVerify32(barOffset + (pos + 0x50), 0x0000000C); // byte count
    WriteVerify32(barOffset + (pos + 0x54), 0x00000000); // data
    WriteVerify32(barOffset + (pos + 0x58), 0x00000020); // mask bit
    WriteVerify32(barOffset + (pos + 0x5C), pos + 0x00); // jump offset DQ5 == 0
    
    WriteVerify32(barOffset + (pos + 0x60), 0x00000003); // 16bit write
    WriteVerify32(barOffset + (pos + 0x64), 0x00000000); // offset
    WriteVerify32(barOffset + (pos + 0x68), 0x00000002); // byte count
    WriteVerify32(barOffset + (pos + 0x6C), 0x000000F0); // data
    
    return pos + 0x70;
}

uint32_t CNTV2HEVCFirmwareInstallerThread::Flash16ReadDeviceId(uint32_t barOffset, uint32_t pos)
{
    uint32_t len = 32;
    
    WriteVerify32(barOffset + (pos + 0x00), 0x00000003); // 16bit write
    WriteVerify32(barOffset + (pos + 0x04), 0x00000AAA); // offset
    WriteVerify32(barOffset + (pos + 0x08), 0x00000002); // byte count
    WriteVerify32(barOffset + (pos + 0x0C), 0x000000AA); // data
    
    WriteVerify32(barOffset + (pos + 0x10), 0x00000003); // 16bit write
    WriteVerify32(barOffset + (pos + 0x14), 0x00000554); // offset
    WriteVerify32(barOffset + (pos + 0x18), 0x00000002); // byte count
    WriteVerify32(barOffset + (pos + 0x1C), 0x00000055); // data
    
    WriteVerify32(barOffset + (pos + 0x20), 0x00000003); // 16bit write
    WriteVerify32(barOffset + (pos + 0x24), 0x00000AAA); // offset
    WriteVerify32(barOffset + (pos + 0x28), 0x00000002); // byte count
    WriteVerify32(barOffset + (pos + 0x2C), 0x00000090); // data
    
    WriteVerify32(barOffset + (pos + 0x30), 0x00000004); // 16bit read
    WriteVerify32(barOffset + (pos + 0x34), 0x00000000); // offset
    WriteVerify32(barOffset + (pos + 0x38), len       ); // byte count
    
    pos += 0x3C + ((len + 3) & ~3);
    
    WriteVerify32(barOffset + (pos + 0x00), 0x00000003); // 16bit write
    WriteVerify32(barOffset + (pos + 0x04), 0x00000000); // offset
    WriteVerify32(barOffset + (pos + 0x08), 0x00000002); // byte count
    WriteVerify32(barOffset + (pos + 0x0C), 0x000000F0); // data
    
    return pos + 0x10;
}

uint32_t CNTV2HEVCFirmwareInstallerThread::Flash16ChipErase(uint32_t barOffset, uint32_t pos)
{
    WriteVerify32(barOffset + (pos + 0x00), 0x00000003); // 16bit write
    WriteVerify32(barOffset + (pos + 0x04), 0x00000AAA); // offset
    WriteVerify32(barOffset + (pos + 0x08), 0x00000002); // byte count
    WriteVerify32(barOffset + (pos + 0x0C), 0x000000AA); // data
    
    WriteVerify32(barOffset + (pos + 0x10), 0x00000003); // 16bit write
    WriteVerify32(barOffset + (pos + 0x14), 0x00000554); // offset
    WriteVerify32(barOffset + (pos + 0x18), 0x00000002); // byte count
    WriteVerify32(barOffset + (pos + 0x1C), 0x00000055); // data
    
    WriteVerify32(barOffset + (pos + 0x20), 0x00000003); // 16bit write
    WriteVerify32(barOffset + (pos + 0x24), 0x00000AAA); // offset
    WriteVerify32(barOffset + (pos + 0x28), 0x00000002); // byte count
    WriteVerify32(barOffset + (pos + 0x2C), 0x00000080); // data
    
    WriteVerify32(barOffset + (pos + 0x30), 0x00000003); // 16bit write
    WriteVerify32(barOffset + (pos + 0x34), 0x00000AAA); // offset
    WriteVerify32(barOffset + (pos + 0x38), 0x00000002); // byte count
    WriteVerify32(barOffset + (pos + 0x3C), 0x000000AA); // data
    
    WriteVerify32(barOffset + (pos + 0x40), 0x00000003); // 16bit write
    WriteVerify32(barOffset + (pos + 0x44), 0x00000554); // offset
    WriteVerify32(barOffset + (pos + 0x48), 0x00000002); // byte count
    WriteVerify32(barOffset + (pos + 0x4C), 0x00000055); // data
    
    WriteVerify32(barOffset + (pos + 0x50), 0x00000003); // 16bit write
    WriteVerify32(barOffset + (pos + 0x54), 0x00000AAA); // offset
    WriteVerify32(barOffset + (pos + 0x58), 0x00000002); // byte count
    WriteVerify32(barOffset + (pos + 0x5C), 0x00000010); // data
    
    return Flash16WaitReady(barOffset, pos + 0x60);
}

uint32_t CNTV2HEVCFirmwareInstallerThread::Flash16SectorErase(uint32_t barOffset, uint32_t pos, uint32_t off)
{
    WriteVerify32(barOffset + (pos + 0x00), 0x00000003); // 16bit write
    WriteVerify32(barOffset + (pos + 0x04), 0x00000AAA); // offset
    WriteVerify32(barOffset + (pos + 0x08), 0x00000002); // byte count
    WriteVerify32(barOffset + (pos + 0x0C), 0x000000AA); // data
    
    WriteVerify32(barOffset + (pos + 0x10), 0x00000003); // 16bit write
    WriteVerify32(barOffset + (pos + 0x14), 0x00000554); // offset
    WriteVerify32(barOffset + (pos + 0x18), 0x00000002); // byte count
    WriteVerify32(barOffset + (pos + 0x1C), 0x00000055); // data
    
    WriteVerify32(barOffset + (pos + 0x20), 0x00000003); // 16bit write
    WriteVerify32(barOffset + (pos + 0x24), 0x00000AAA); // offset
    WriteVerify32(barOffset + (pos + 0x28), 0x00000002); // byte count
    WriteVerify32(barOffset + (pos + 0x2C), 0x00000080); // data
    
    WriteVerify32(barOffset + (pos + 0x30), 0x00000003); // 16bit write
    WriteVerify32(barOffset + (pos + 0x34), 0x00000AAA); // offset
    WriteVerify32(barOffset + (pos + 0x38), 0x00000002); // byte count
    WriteVerify32(barOffset + (pos + 0x3C), 0x000000AA); // data
    
    WriteVerify32(barOffset + (pos + 0x40), 0x00000003); // 16bit write
    WriteVerify32(barOffset + (pos + 0x44), 0x00000554); // offset
    WriteVerify32(barOffset + (pos + 0x48), 0x00000002); // byte count
    WriteVerify32(barOffset + (pos + 0x4C), 0x00000055); // data
    
    WriteVerify32(barOffset + (pos + 0x50), 0x00000003); // 16bit write
    WriteVerify32(barOffset + (pos + 0x54), off       ); // offset
    WriteVerify32(barOffset + (pos + 0x58), 0x00000002); // byte count
    WriteVerify32(barOffset + (pos + 0x5C), 0x00000030); // data
    
    return Flash16WaitReady(barOffset, pos + 0x60);
}

uint32_t CNTV2HEVCFirmwareInstallerThread::Flash16WriteBuffer512(uint32_t barOffset, uint32_t pos, uint32_t off, uint32_t *buf)
{
    int i;
    
    WriteVerify32(barOffset + (pos + 0x00), 0x00000003); // 16bit write
    WriteVerify32(barOffset + (pos + 0x04), 0x00000AAA); // offset
    WriteVerify32(barOffset + (pos + 0x08), 0x00000002); // byte count
    WriteVerify32(barOffset + (pos + 0x0C), 0x000000AA); // data
    
    WriteVerify32(barOffset + (pos + 0x10), 0x00000003); // 16bit write
    WriteVerify32(barOffset + (pos + 0x14), 0x00000554); // offset
    WriteVerify32(barOffset + (pos + 0x18), 0x00000002); // byte count
    WriteVerify32(barOffset + (pos + 0x1C), 0x00000055); // data
    
    WriteVerify32(barOffset + (pos + 0x20), 0x00000003); // 16bit write
    WriteVerify32(barOffset + (pos + 0x24), off       ); // offset
    WriteVerify32(barOffset + (pos + 0x28), 0x00000002); // byte count
    WriteVerify32(barOffset + (pos + 0x2C), 0x00000025); // data
    
    WriteVerify32(barOffset + (pos + 0x30), 0x00000003); // 16bit write
    WriteVerify32(barOffset + (pos + 0x34), off       ); // offset
    WriteVerify32(barOffset + (pos + 0x38), 0x00000002); // byte count
    WriteVerify32(barOffset + (pos + 0x3C), 0x000000FF); // data
    
    WriteVerify32(barOffset + (pos + 0x40), 0x00000003); // 16bit write
    WriteVerify32(barOffset + (pos + 0x44), off       ); // offset
    WriteVerify32(barOffset + (pos + 0x48), 0x00000200); // byte count
    
    for (i = 0; i < 128; i++)
    {
        WriteVerify32(barOffset + (pos + 0x4C + (4 * i)), buf[i]); // data
    }
    
    pos += 0x24C;
    off += 0x1FE;
    
    WriteVerify32(barOffset + (pos + 0x00), 0x00000003); // 16bit write
    WriteVerify32(barOffset + (pos + 0x04), off       ); // offset
    WriteVerify32(barOffset + (pos + 0x08), 0x00000002); // byte count
    WriteVerify32(barOffset + (pos + 0x0C), 0x00000029); // data
    
    return Flash16WaitReady(barOffset, pos + 0x10); // 0x25C + 0x70 = 0x2CC
}

uint32_t CNTV2HEVCFirmwareInstallerThread::Flash16Read64(uint32_t barOffset, uint32_t pos, uint32_t off)
{
    WriteVerify32(barOffset + (pos + 0x00), 0x00000004); // 16bit read
    WriteVerify32(barOffset + (pos + 0x04), off       ); // offset
    WriteVerify32(barOffset + (pos + 0x08), 0x00000040); // byte count
    
    return pos + 0x0C + 0x40;
}

uint32_t CNTV2HEVCFirmwareInstallerThread::ReadDeviceId(uint32_t barOffset, uint32_t pos)
{
    pos = Flash16WaitReady(barOffset, pos);
    pos = Flash16ReadDeviceId(barOffset, pos);
    
    return pos;
}

uint32_t CNTV2HEVCFirmwareInstallerThread::StatusCheck(uint32_t address, uint32_t checkValue, uint32_t timeout, bool isEqual, bool dotUpdate)
{
    uint32_t    value;
    uint64_t    startTime, currentTime;
    
    startTime = AJATime::GetSystemMilliseconds();
    
    if (isEqual)
    {
        do
        {
            AJATime::Sleep(1);
            m_device.HevcReadRegister(address, &value);
            currentTime = AJATime::GetSystemMilliseconds();
            if (dotUpdate && (currentTime > m_updateTime + 1000))
            {
                printf(".");
				fflush(stdout);
                m_updateTime = currentTime;
            }
        } while (value == checkValue && (currentTime-startTime <  timeout));
    }
    else
    {
        do
        {
            AJATime::Sleep(1);
            m_device.HevcReadRegister(address, &value);
            currentTime = AJATime::GetSystemMilliseconds();
            if (dotUpdate && (currentTime > m_updateTime + 1000))
            {
                printf(".");
				fflush(stdout);
                m_updateTime = currentTime;
            }
        } while (value != checkValue && (currentTime-startTime <  timeout));
    }
    
    return value;
}

void CNTV2HEVCFirmwareInstallerThread::IdentifyFlash(uint32_t barOffset, M31FlashParams *flash)
{
    uint32_t i, id[2];
    
    barOffset += 0x70 + 0x3C;
    
    m_device.HevcReadRegister((barOffset + 0x00), &id[0]);
    m_device.HevcReadRegister((barOffset + 0x1C), &id[1]);
    
    id[0] &= 0x00FF00FF;
    id[1] &= 0x00FF00FF;
    
	//printf("flash (%08x %08x)\n", id[0], id[1]);
    
    for (i = 0; m_flashTable[i].device_id[0] != 0; i++)
    {
        if ((m_flashTable[i].device_id[0] == id[0]) && (m_flashTable[i].device_id[1] == id[1]))
            break;
    }
    
    memcpy(flash, &m_flashTable[i], sizeof(M31FlashParams));
}

bool CNTV2HEVCFirmwareInstallerThread::IsBooted()
{
    bool        result = true;
    uint32_t    val = 0;
    
    if (m_hevcInfo.pciId.subVendor == 0x00000000)
    {
        m_device.HevcReadRegister((SRAM_BOOTCHECK_ADDR), &val);
        if ((val & 0x01) == 0x00)
        {
            result = false;
        }
    }
    return result;
}


// Functions

HEVCError CNTV2HEVCFirmwareInstallerThread::Check()
{
    HEVCError   result = HEVC_ERR_NONE;
    uint32_t    val = 0;
    
    printf("Check\n");

    // check boot status
    if (m_hevcInfo.pciId.subVendor == 0x00000000)
    {
        printf("Maintenance mode subVendor=%08x\n", m_hevcInfo.pciId.subVendor);
    }
    else
    {
        printf("Normal mode subVendor=%08x\n", m_hevcInfo.pciId.subVendor);
        // Make sure we are booted
		m_device.HevcReadRegister((BAR5_OFFSET + PCIE_BOOT_FLAG), &val);
        if (val == 0x00000000)
        {
            result = HEVC_ERR_NOT_BOOTED;
        }
        else
        {
            // Make sure we are in the boot state
            m_device.HevcReadRegister((BAR2_OFFSET + 0x200), &val);
            if (val == 0xFFFFFFFF)
            {
                result = HEVC_ERR_STATE;
            }
        }
    }
    return result;
}

HEVCError CNTV2HEVCFirmwareInstallerThread::Boot()
{
    HEVCError       result = HEVC_ERR_NONE;
    FILE*           fp = NULL;
    struct stat     st;
    uint32_t        mode;
    uint32_t        val;
    uint32_t        i;
    uint32_t        dataSize;
    
    printf("Boot\n");

    if (m_hevcInfo.pciId.subVendor == 0x00000000)
    {
        // temporary use for boot check
        m_device.HevcReadRegister((SRAM_BOOTCHECK_ADDR), &val);
        if ((val & 0x01) == 0x00)
        {
            // open boot program
            fp = fopen("m3x_mainte_boot.bin", "rb");
            if (fp == NULL || fstat(fileno(fp), &st) != 0)
            {
                result = HEVC_ERR_OPEN;
                goto bootexit;
            }
            
            // get program size
            dataSize = st.st_size;
            if (dataSize == 0 || dataSize > SRAM_BOOT_MAX_SIZE)
            {
                result = HEVC_ERR_SIZE;
                goto bootexit;
            }
            
            // buffer clear
            memset(m_dataBuf, 0x00, sizeof(m_dataBuf));
            
            // read boot program
            if (fread(m_dataBuf, 1, dataSize, fp) != dataSize)
            {
                result = HEVC_ERR_READ;
                goto bootexit;
            }
            
            // write boot program to sram
            for (i = 0; i < ((dataSize + 3) & ~3) / 4; i++)
            {
                WriteVerify32(SRAM_BASE_ADDR + SRAM_BOOT_OFFSET + (i * 4), m_dataBuf[i]);
            }
            
            // write mode flag
            m_device.HevcWriteRegister((SRAM_BASE_ADDR + SRAM_MODE_FLAG), 0x00000048);
            
            // write boot flag
            m_device.HevcWriteRegister((SRAM_BOOTCHECK_ADDR), val | 0x01);
            
            // boot program start
            WriteRegisterBar4((HSIO_SYSOC_BASE_ADDR + PCIE_INT_REG_SET_OFFSET), 0x00000001);
            
            // status check
            mode = StatusCheck(SRAM_BASE_ADDR + SRAM_MODE_FLAG, 0x000000FF, 5*1000, false, false);
            if (mode != 0x000000FF)
            {
                result = HEVC_ERR_BOOT;
                goto bootexit;
            }
            printf("Boot success\n");
        }
        else
        {
            // get mode flag
            m_device.HevcReadRegister((SRAM_BASE_ADDR + SRAM_MODE_FLAG), &mode);
            if (mode == 0x00000048)
            {
                result = HEVC_ERR_BOOT;
                goto bootexit;
            }
            printf("Already booted\n");
        }
    }
    
bootexit:
    if (fp != NULL)
    {
        fclose(fp);
    }
    
    return result;
}

HEVCError CNTV2HEVCFirmwareInstallerThread::Stop()
{
    uint32_t        mode;
    uint32_t        val;
    uint32_t        pos;
    
    printf("Stop\n");

    m_device.HevcReadRegister((SRAM_BASE_ADDR + SRAM_MODE_FLAG), &mode);
    if (mode == 0x00000048)
    {
        return HEVC_ERR_BOOT;
    }
    
    // If we are already stopped do nothing
    if (mode == 0x000000FF)
    {
        printf("Already stopped\n");
        return HEVC_ERR_NONE;
    }
    
    val = 0x000000FF;
    
    if (mode != 0x00000000)
    {
        if (mode & 0x00010000)
            val = 0;
        else
        {
            // get command flag
            m_device.HevcReadRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), &val);
            
            if (val == 0x000000FF)
            {
                // stop start
                m_device.HevcWriteRegister((SRAM_BASE_ADDR + SRAM_STOP_FLAG), 0x000000FF);
                AJATime::Sleep(10);
                
                // status check
                mode = StatusCheck(SRAM_BASE_ADDR + SRAM_MODE_FLAG, 0x000000FF, 5*1000, false, false);
                if (mode != 0x000000FF)
                {
                    return HEVC_ERR_STOP;
                }
                
                // load start
                WriteVerify32(SRAM_BASE_ADDR + SRAM_LOAD_FLAG, 0x00000000);
                m_device.HevcWriteRegister((SRAM_BASE_ADDR + SRAM_MODE_FLAG), 0x00000000);
                AJATime::Sleep(10);
                
                // status check
                mode = StatusCheck(SRAM_BASE_ADDR + SRAM_MODE_FLAG, 0x00000000, 5*1000, true, false);
            }
            
            if (mode != 0x00000000)
            {
                pos = SRAM_DATA_OFFSET;
                
                // command set
                pos = Flash16WaitReady(SRAM_BASE_ADDR, pos);
                
                WriteVerify32(SRAM_BASE_ADDR + (pos + 0x00), 0x0000002A); // recovery
                WriteVerify32(SRAM_BASE_ADDR + (pos + 0x04), 0x00000000); // offset
                WriteVerify32(SRAM_BASE_ADDR + (pos + 0x08), 0x00000000); // byte count
                
                // command end
                WriteVerify32(SRAM_BASE_ADDR + (pos + 0x0C), 0x00000000);
                
                // remap recovery start
                m_device.HevcWriteRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), 0x000000FF);
                AJATime::Sleep(10);
                
                // status check
                val = StatusCheck(SRAM_BASE_ADDR + SRAM_CMND_FLAG, 0x000000FF, 100*1000, true, false);
            }
        }
    }
    
    // stop start
    m_device.HevcWriteRegister((SRAM_BASE_ADDR + SRAM_STOP_FLAG), 0x000000FF);
    AJATime::Sleep(10);
    
    // status check
    mode = StatusCheck(SRAM_BASE_ADDR + SRAM_MODE_FLAG, 0x000000FF, 5*1000, false, false);
    if (mode != 0x000000FF)
    {
        return HEVC_ERR_STOP;
    }
    
    if (val != 0)
    {
        return HEVC_ERR_RECOVERED;
    }
    
    printf("Stop success\n");
    return HEVC_ERR_NONE;
}

HEVCError CNTV2HEVCFirmwareInstallerThread::LoadFlash()
{
    HEVCError       result = HEVC_ERR_NONE;
    FILE*           fp = NULL;
    struct stat     st;
    uint32_t        mode;
    uint32_t        val;
    uint32_t        pos, i;
    uint32_t        dataSize;
    
    printf("LoadFlash\n");
    
    m_device.HevcReadRegister((SRAM_BASE_ADDR + SRAM_MODE_FLAG), &mode);
    if (mode == 0x00000048)
    {
        result = HEVC_ERR_BOOT;
        goto loadflashexit;
    }
    if (mode == 0x00000000)
    {
        result = HEVC_ERR_LOAD;
        goto loadflashexit;
    }
    if (mode != 0x000000FF)
    {
        result = HEVC_ERR_ALREADY_LOADED;
        goto loadflashexit;
    }
    
    if (m_hevcInfo.pciId.subVendor == 0x00000000)
    {
        // open and read flash program into memory
        fp = fopen("m3x_mainte_flash.bin", "rb");
        if (fp == NULL || fstat(fileno(fp), &st) != 0)
        {
            result = HEVC_ERR_OPEN;
            goto loadflashexit;
        }
        
        dataSize = st.st_size;
        if (dataSize == 0 || dataSize > SRAM_LOAD_MAX_SIZE)
        {
            result = HEVC_ERR_SIZE;
            goto loadflashexit;
        }
        
        memset(m_dataBuf, 0x00, sizeof(m_dataBuf));
        
        if (fread(m_dataBuf, 1, dataSize, fp) != dataSize)
        {
            result = HEVC_ERR_READ;
            goto loadflashexit;
        }
        
        // write flash program to sram
        for (i = 0; i < ((dataSize + 3) & ~3) / 4; i++)
        {
            WriteVerify32((SRAM_BASE_ADDR + SRAM_LOAD_OFFSET) + (i * 4), m_dataBuf[i]);
        }
    }
    
    // load start
    WriteVerify32(SRAM_BASE_ADDR + SRAM_LOAD_FLAG, 0x00000000);
    m_device.HevcWriteRegister((SRAM_BASE_ADDR + SRAM_MODE_FLAG), 0x00000000);
    AJATime::Sleep(10);
    
    // status check
    mode = StatusCheck(SRAM_BASE_ADDR + SRAM_MODE_FLAG, 0x00000000, 5*1000, true, false);
    if (mode == 0x00000000)
    {
        result = HEVC_ERR_LOAD;
        goto loadflashexit;
    }
    
    pos = SRAM_DATA_OFFSET;
    
    // command set
    pos = ReadDeviceId(SRAM_BASE_ADDR, pos);
    
    // command end
    WriteVerify32(SRAM_BASE_ADDR + pos, 0x00000000);
    
    // read id start
    m_device.HevcWriteRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), 0x000000FF);
    AJATime::Sleep(10);
    
    // status check
    val = StatusCheck(SRAM_BASE_ADDR + SRAM_CMND_FLAG, 0x000000FF, 5*1000, true, false);
    if (val != 0)
    {
        result = HEVC_ERR_DEVICEID;
        goto loadflashexit;
    }
    
    // Identify the Spansion flash
    IdentifyFlash(SRAM_BASE_ADDR + SRAM_DATA_OFFSET, &m_flash);
    
    printf("Load success (memcs ID[%06X:%06X] %s)\n", m_flash.device_id[0], m_flash.device_id[1], m_flash.device_name);
    
loadflashexit:
    if (fp != NULL)
    {
        fclose(fp);
    }
    
    return result;
}

HEVCError CNTV2HEVCFirmwareInstallerThread::ChipErase()
{
    uint32_t        mode;
    uint32_t        val;
    uint32_t        pos;
    AJATimer        timer;
    
    printf("ChipErase\n");

    m_device.HevcReadRegister((SRAM_BASE_ADDR + SRAM_MODE_FLAG), &mode);
    if (mode == 0x00000048)
    {
        return HEVC_ERR_BOOT;
    }
    if (mode == 0x000000FF)
    {
        return HEVC_ERR_NOT_LOADED;
    }
    if (mode == 0x00000000)
    {
        return HEVC_ERR_LOAD;
    }
    if (mode & 0x00010000)
    {
        return HEVC_ERR_ALREADY_LOADED;
    }
    
    // command flag check
    m_device.HevcReadRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), &val);
    if (val == 0x000000FF)
    {
        return HEVC_ERR_BUSY;
    }
    
    timer.Start();
    
    pos = SRAM_DATA_OFFSET;
    
    // command set
    pos = ReadDeviceId(SRAM_BASE_ADDR, pos);
    
    // command end
    WriteVerify32(SRAM_BASE_ADDR + pos, 0x00000000);
    
    // read id start
    m_device.HevcWriteRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), 0x000000FF);
    AJATime::Sleep(10);
    
    // status check
    val = StatusCheck(SRAM_BASE_ADDR + SRAM_CMND_FLAG, 0x000000FF, 5*1000, true, false);
    if (val != 0)
    {
        return HEVC_ERR_DEVICEID;
    }
    
    // Identify the Spansion flash
    IdentifyFlash(SRAM_BASE_ADDR + SRAM_DATA_OFFSET, &m_flash);
    
    pos = SRAM_DATA_OFFSET;
    
    // command set
    pos = Flash16ChipErase(SRAM_BASE_ADDR, pos);
    
    // command end
    WriteVerify32(SRAM_BASE_ADDR + pos, 0x00000000);
    
    // chip erase start
    m_device.HevcWriteRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), 0x000000FF);
    AJATime::Sleep(10);
    
    
    // status check with update
    m_updateTime = AJATime::GetSystemMilliseconds();
    val = StatusCheck(SRAM_BASE_ADDR + SRAM_CMND_FLAG, 0x000000FF, 500*1000, true, true);
    printf("\n");
    if (val != 0)
    {
        return HEVC_ERR_ERASE;
    }
    
    timer.Stop();
    printf("ChipErase success\n");
    printf("Elapsed time for chip erase: %4.3f seconds\n", (double)timer.ElapsedTime()/1000);
    return HEVC_ERR_NONE;
}

HEVCError CNTV2HEVCFirmwareInstallerThread::SectorErase(uint32_t offset, uint32_t dataSize)
{
    uint32_t        mode;
    uint32_t        val;
    uint32_t        pos, offsetEnd;
    AJATimer        timer;
    
    printf("SectorErase\n");
    
    m_device.HevcReadRegister((SRAM_BASE_ADDR + SRAM_MODE_FLAG), &mode);
    if (mode == 0x00000048)
    {
        return HEVC_ERR_BOOT;
    }
    if (mode == 0x000000FF)
    {
        return HEVC_ERR_NOT_LOADED;
    }
    if (mode == 0x00000000)
    {
        return HEVC_ERR_LOAD;
    }
    if (mode & 0x00010000)
    {
        return HEVC_ERR_ALREADY_LOADED;
    }
    
    // command flag check
    m_device.HevcReadRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), &val);
    if (val == 0x000000FF)
    {
        return HEVC_ERR_BUSY;
    }
    
    // check size
    if (dataSize == 0)
    {
        return HEVC_ERR_SIZE;
    }
    
    timer.Start();
    
    pos = SRAM_DATA_OFFSET;
    
    // command set
    pos = ReadDeviceId(SRAM_BASE_ADDR, pos);
    
    // command end
    WriteVerify32(SRAM_BASE_ADDR + pos, 0x00000000);
    
    // read id start
    m_device.HevcWriteRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), 0x000000FF);
    AJATime::Sleep(10);
    
    // status check
    val = StatusCheck(SRAM_BASE_ADDR + SRAM_CMND_FLAG, 0x000000FF, 5*1000, true, false);
    if (val != 0)
    {
        return HEVC_ERR_DEVICEID;
    }
    
    // Identify the Spansion flash
    IdentifyFlash(SRAM_BASE_ADDR + SRAM_DATA_OFFSET, &m_flash);
    
    // check offset
    if (offset >= m_flash.device_size)
    {
        return HEVC_ERR_OFFSET;
    }
    
    if (dataSize > m_flash.device_size - offset)
    {
        dataSize = m_flash.device_size - offset;
    }
    
    offsetEnd = offset + dataSize;
    
    m_updateTime = AJATime::GetSystemMilliseconds();

    while (offset < offsetEnd)
    {
        pos = SRAM_DATA_OFFSET;
        
        // command set
        pos = Flash16SectorErase(SRAM_BASE_ADDR, pos, offset);
        
        // command end
        WriteVerify32(SRAM_BASE_ADDR + pos, 0x00000000);
        
        // sector erase start
        m_device.HevcWriteRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), 0x000000FF);
        AJATime::Sleep(10);
        
        // status check
        val = StatusCheck(SRAM_BASE_ADDR + SRAM_CMND_FLAG, 0x000000FF, 10*1000, true, true);
        if (val != 0)
        {
            return HEVC_ERR_ERASE;
        }
        
        offset += SECTOR_SIZE;
    }
    
    printf("\n");
    
    timer.Stop();
    printf("SectorErase success\n");
    printf("Elapsed time for sector erase: %4.3f seconds\n", (double)timer.ElapsedTime()/1000);
    return HEVC_ERR_NONE;
}

HEVCError CNTV2HEVCFirmwareInstallerThread::Program(char* fileName, uint32_t flashOffset, uint32_t fileOffset, uint32_t length)
{
    HEVCError       result = HEVC_ERR_NONE;
    FILE*           fp = NULL;
    struct stat     st;
    uint32_t        mode;
    uint32_t        val, i, j;
    uint32_t        pos, offsetEnd;
    uint32_t        pp, px;
    uint32_t        size, dataSize, blockSize, remainSize;
    uint32_t        loopCount;
    AJATimer        timer;
    
    printf("Program (%s - flashOffset=0x%08X, fileOffset=0x%08X length=0x%08X)\n", fileName, flashOffset, fileOffset, length);
    
    m_device.HevcReadRegister((SRAM_BASE_ADDR + SRAM_MODE_FLAG), &mode);
    if (mode == 0x00000048)
    {
        result = HEVC_ERR_BOOT;
        goto programexit;
    }
    if (mode == 0x000000FF)
    {
        result = HEVC_ERR_NOT_LOADED;
        goto programexit;
    }
    if (mode == 0x00000000)
    {
        result = HEVC_ERR_LOAD;
        goto programexit;
    }
    if (mode & 0x00010000)
    {
        result = HEVC_ERR_ALREADY_LOADED;
        goto programexit;
    }
    
    // command flag check
    m_device.HevcReadRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), &val);
    if (val == 0x000000FF)
    {
        result = HEVC_ERR_BUSY;
        goto programexit;
    }
    
    // open data file
    fp = fopen(fileName, "rb");
    if (fp == NULL || fstat(fileno(fp), &st) != 0)
    {
        result = HEVC_ERR_OPEN;
        goto programexit;
    }
    
    // get data size
    dataSize = st.st_size;
    if (dataSize == 0)
    {
        result = HEVC_ERR_SIZE;
        goto programexit;
    }

	if (fileOffset >= dataSize || fseek(fp, fileOffset, SEEK_SET) != 0 || ftell(fp) != fileOffset)
	{
		result = HEVC_ERR_OFFSET;
		goto programexit;
	}
	dataSize -= fileOffset;

	if (length < dataSize)
		dataSize = length;
    
    timer.Start();
    
    // check capability
    WriteVerify32(SRAM_BASE_ADDR + SRAM_DATA_OFFSET, 0x0000002E);
    
    // check capability start
    m_device.HevcWriteRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), 0x000000FF);
    AJATime::Sleep(10);
    
    // status check
    val = StatusCheck(SRAM_BASE_ADDR + SRAM_CMND_FLAG, 0x000000FF, 5*1000, true, false);
    if (val == 0x000000FF)
    {
        result = HEVC_ERR_PROGRAM;
        goto programexit;
    }
    
    pos = SRAM_DATA_OFFSET;
    
    // command set
    pos = ReadDeviceId(SRAM_BASE_ADDR, pos);
    
    // command end
    WriteVerify32(SRAM_BASE_ADDR + pos, 0x00000000);
    
    // read id start
    m_device.HevcWriteRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), 0x000000FF);
    AJATime::Sleep(10);
    
    // status check
    val = StatusCheck(SRAM_BASE_ADDR + SRAM_CMND_FLAG, 0x000000FF, 5*1000, true, false);
    if (val != 0)
    {
        result = HEVC_ERR_DEVICEID;
        goto programexit;
    }
    
    // Identify the Spansion flash
    IdentifyFlash(SRAM_BASE_ADDR + SRAM_DATA_OFFSET, &m_flash);
    
    // check flashOffset
    if ((flashOffset & (WR_DATA_SIZE - 1)) || flashOffset >= m_flash.device_size)
    {
        result = HEVC_ERR_OFFSET;
        goto programexit;
    }
    
    // check size
    if (dataSize > m_flash.device_size - flashOffset)
    {
        result = HEVC_ERR_SIZE;
        goto programexit;
    }
    
    // calc block size
    blockSize = (sizeof(m_dataBuf) / WR_FUNC_SIZE - 1) * WR_DATA_SIZE;
    size = (sizeof(m_dataBuf) / RD_FUNC_SIZE - 1) * RD_DATA_SIZE;
    if (blockSize > size)
    {
        blockSize = size;
    }
    
    PERCENT_PROGRESS_START();
    
    remainSize = dataSize;
    offsetEnd = flashOffset + dataSize;
    
    while (flashOffset < offsetEnd)
    {
        if (remainSize < blockSize)
            size = remainSize;
        else
            size = blockSize;
        
        loopCount = ((size + (WR_DATA_SIZE - 1)) & ~(WR_DATA_SIZE - 1)) / WR_DATA_SIZE;
        
        // buffer fill
        memset(m_dataBuf, 0xFF, sizeof(m_dataBuf));
        
        // read flash data
        if (fread(m_dataBuf, 1, size, fp) != size)
        {
            result = HEVC_ERR_READ;
            goto programexit;
        }
        
        pos = SRAM_DATA_OFFSET;
        
        // command set
        for (i = 0; i < loopCount; i++)
        {
            pos = Flash16WriteBuffer512(SRAM_BASE_ADDR, pos, flashOffset + (WR_DATA_SIZE * i), m_dataBuf + (WR_DATA_SIZE / 4 * i));
        }
        
        // command end
        WriteVerify32(SRAM_BASE_ADDR + pos, 0x00000000);
        
        // write start
        m_device.HevcWriteRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), 0x000000FF);
        AJATime::Sleep(10);
        
        // status check
        val = StatusCheck(SRAM_BASE_ADDR + SRAM_CMND_FLAG, 0x000000FF, 5*1000, true, false);
        if (val != 0)
        {
            result = HEVC_ERR_WRITE;
            goto programexit;
        }
        
        loopCount = ((size + (RD_DATA_SIZE - 1)) & ~(RD_DATA_SIZE - 1)) / RD_DATA_SIZE;
        
        pos = SRAM_DATA_OFFSET;
        
        // command set
        for (i = 0; i < loopCount; i++)
        {
            pos = Flash16Read64(SRAM_BASE_ADDR, pos, flashOffset + (RD_DATA_SIZE * i));
        }
        
        // command end
        WriteVerify32(SRAM_BASE_ADDR + pos, 0x00000000);
        
        // read start
        m_device.HevcWriteRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), 0x000000FF);
        AJATime::Sleep(10);
        
        // status check
        val = StatusCheck(SRAM_BASE_ADDR + SRAM_CMND_FLAG, 0x000000FF, 5*1000, true, false);
        if (val != 0)
        {
            result = HEVC_ERR_READ;
            goto programexit;
        }
        
        pos = SRAM_DATA_OFFSET;
        
        // read flash data
        for (i = 0; i < loopCount; i++)
        {
            for (j = 0; j < RD_DATA_SIZE / 4; j++)
            {
                m_device.HevcReadRegister((SRAM_BASE_ADDR + (pos + RD_FUNC_DATA)) + (j * 4), &val);
                m_tempBuf[RD_DATA_SIZE / 4 * i + j] = val;
            }
            pos += RD_FUNC_SIZE;
        }
        
        // verify flash data
        if (memcmp(m_tempBuf, m_dataBuf, size) != 0)
        {
            fprintf(stderr, "flash verify error\n");
            for (i = 0; i < ((size + 3) & ~3) / 4; i++)
            {
                if (m_tempBuf[i] != m_dataBuf[i])
                {
                    fprintf(stderr, "error: 0x%08X W:0x%08X R:0x%08X\n", flashOffset + i * 4, m_dataBuf[i], m_tempBuf[i]);
                } else {
                    fprintf(stderr, "     : 0x%08X W:0x%08X R:0x%08X\n", flashOffset + i * 4, m_dataBuf[i], m_tempBuf[i]);
                }
            }
            result = HEVC_ERR_VERIFY;
            goto programexit;
        }
        
        flashOffset += size;
        remainSize -= size;
        
        PERCENT_PROGRESS();
    }
    
    PERCENT_PROGRESS_END();
    
    timer.Stop();
    printf("Program success\n");
    printf("Elapsed time for program: %4.3f seconds\n", (double)timer.ElapsedTime()/1000);
    
programexit:
    if (fp != NULL)
    {
        fclose(fp);
    }
    
    return result;
}

HEVCError CNTV2HEVCFirmwareInstallerThread::FlashDump(char* fileName, uint32_t offset, uint32_t dataSize)
{
    HEVCError       result = HEVC_ERR_NONE;
    FILE*           fp = NULL;
    uint32_t        mode;
    uint32_t        val, i, j;
    uint32_t        pos, offsetEnd;
    uint32_t        pp, px;
    uint32_t        size, blockSize, remainSize;
    uint32_t        loopCount;
    AJATimer        timer;
    
    printf("FlashDump\n");
    
    m_device.HevcReadRegister((SRAM_BASE_ADDR + SRAM_MODE_FLAG), &mode);
	printf("mode = %0x\n", mode);
    if (mode == 0x00000048)
    {
        result = HEVC_ERR_BOOT;
        goto flashdumpexit;
    }
    if (mode == 0x000000FF)
    {
        result = HEVC_ERR_NOT_LOADED;
        goto flashdumpexit;
    }
    if (mode == 0x00000000)
    {
        result = HEVC_ERR_LOAD;
        goto flashdumpexit;
    }
    if (mode & 0x00010000)
    {
        result = HEVC_ERR_ALREADY_LOADED;
        goto flashdumpexit;
    }
    
    // command flag check
    m_device.HevcReadRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), &val);
    if (val == 0x000000FF)
    {
        result = HEVC_ERR_BUSY;
        goto flashdumpexit;
    }
    
    // open data file
    fp = fopen(fileName, "wb");
    if (fp == NULL)
    {
        result = HEVC_ERR_OPEN;
        goto flashdumpexit;
    }
    
    // check size
    if (dataSize == 0)
    {
        result = HEVC_ERR_SIZE;
        goto flashdumpexit;
    }
    
    timer.Start();
    
    pos = SRAM_DATA_OFFSET;
    
    // command set
    pos = ReadDeviceId(SRAM_BASE_ADDR, pos);
    
    // command end
    WriteVerify32(SRAM_BASE_ADDR + pos, 0x00000000);
    
    // read id start
    m_device.HevcWriteRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), 0x000000FF);
    AJATime::Sleep(10);
    
    // status check
    val = StatusCheck(SRAM_BASE_ADDR + SRAM_CMND_FLAG, 0x000000FF, 5*1000, true, false);
    if (val != 0)
    {
        result = HEVC_ERR_DEVICEID;
        goto flashdumpexit;
    }
    
    // Identify the Spansion flash
    IdentifyFlash(SRAM_BASE_ADDR + SRAM_DATA_OFFSET, &m_flash);
    
    // check offset
    if ((offset & (RD_DATA_SIZE - 1)) || offset >= m_flash.device_size)
    {
        result = HEVC_ERR_OFFSET;
        goto flashdumpexit;
    }
    
    // check size
    if (dataSize > m_flash.device_size - offset)
    {
        result = HEVC_ERR_SIZE;
        goto flashdumpexit;
    }
    
    // calc block size
    blockSize = (sizeof(m_dataBuf) / RD_FUNC_SIZE - 1) * RD_DATA_SIZE;
    
    PERCENT_PROGRESS_START();
    
    remainSize = dataSize;
    offsetEnd = offset + dataSize;
    
    while (offset < offsetEnd)
    {
        if (remainSize < blockSize)
            size = remainSize;
        else
            size = blockSize;
        
        loopCount = ((size + (RD_DATA_SIZE - 1)) & ~(RD_DATA_SIZE - 1)) / RD_DATA_SIZE;
        
        pos = SRAM_DATA_OFFSET;
        
        // command set
        for (i = 0; i < loopCount; i++)
        {
            pos = Flash16Read64(SRAM_BASE_ADDR, pos, offset + (RD_DATA_SIZE * i));
        }
        
        // command end
        WriteVerify32(SRAM_BASE_ADDR + pos, 0x00000000);
        
        // read start
        m_device.HevcWriteRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), 0x000000FF);
        AJATime::Sleep(10);
        
        // status check
        val = StatusCheck(SRAM_BASE_ADDR + SRAM_CMND_FLAG, 0x000000FF, 5*1000, true, false);
        if (val != 0)
        {
            result = HEVC_ERR_READ;
            goto flashdumpexit;
        }
        
        pos = SRAM_DATA_OFFSET;
        
        // read flash data
        for (i = 0; i < loopCount; i++)
        {
            for (j = 0; j < RD_DATA_SIZE / 4; j++)
            {
                m_device.HevcReadRegister((SRAM_BASE_ADDR + (pos + RD_FUNC_DATA)) + (j *4), &val);
                m_tempBuf[RD_DATA_SIZE / 4 * i + j] = val;
            }
            pos += RD_FUNC_SIZE;
        }
        
        // write flash data
        if (fwrite(m_tempBuf, 1, size, fp) != size)
        {
            result = HEVC_ERR_WRITE;
            goto flashdumpexit;
        }
        
        offset += size;
        remainSize -= size;
        
        PERCENT_PROGRESS();
    }
    
    PERCENT_PROGRESS_END();
    
    timer.Stop();
    printf("FlashDump success\n");
    printf("Elapsed time for flash dump: %4.3f seconds\n", (double)timer.ElapsedTime()/1000);
    
flashdumpexit:
    if (fp != NULL)
    {
        fclose(fp);
    }
    
    return result;
}

HEVCError CNTV2HEVCFirmwareInstallerThread::LoadDram()
{
    HEVCError       result = HEVC_ERR_NONE;
    FILE*           fp = NULL;
    struct stat     st;
    uint32_t        mode;
    uint32_t        val, i;
    uint32_t        dataSize;
    
    printf("LoadDram\n");
    
    m_device.HevcReadRegister((SRAM_BASE_ADDR + SRAM_MODE_FLAG), &mode);
    if (mode == 0x00000048)
    {
        result = HEVC_ERR_BOOT;
        goto loaddramexit;
    }
    if (mode == 0x00000000)
    {
        result = HEVC_ERR_LOAD;
        goto loaddramexit;
    }
    if (mode != 0x000000FF)
    {
        result = HEVC_ERR_ALREADY_LOADED;
        goto loaddramexit;
    }
    
    if (m_hevcInfo.pciId.subVendor == 0x00000000)
    {
        // open dram program
        fp = fopen("m3x_mainte_dram.bin", "rb");
        if (fp == NULL || fstat(fileno(fp), &st) != 0)
        {
            result = HEVC_ERR_OPEN;
            goto loaddramexit;
        }
        
        // get program size
        dataSize = st.st_size;
        if (dataSize == 0 || dataSize > SRAM_LOAD_MAX_SIZE)
        {
            result = HEVC_ERR_SIZE;
            goto loaddramexit;
        }
        
        // buffer clear
        memset(m_dataBuf, 0x00, sizeof(m_dataBuf));
        
        // read dram program
        if (fread(m_dataBuf, 1, dataSize, fp) != dataSize)
        {
            result = HEVC_ERR_READ;
            goto loaddramexit;
        }
        
        // write dram program to sram
        for (i = 0; i < ((dataSize + 3) & ~3) / 4; i++)
        {
            WriteVerify32((SRAM_BASE_ADDR + SRAM_LOAD_OFFSET) + (i * 4), m_dataBuf[i]);
        }
        
        fclose(fp);
        
        // open dram param
        fp = fopen("dram_init_param.bin", "rb");
        if (fp == NULL || fstat(fileno(fp), &st) != 0)
        {
            result = HEVC_ERR_OPEN;
            goto loaddramexit;
        }
        
        // get param size
        dataSize = st.st_size;
        if (dataSize == 0 || dataSize > DRAM_PARM_MAX_SIZE)
        {
            result = HEVC_ERR_SIZE;
            goto loaddramexit;
        }
        
        // buffer clear
        memset(m_dataBuf, 0x00, sizeof(m_dataBuf));
        
        // read dram param
        if (fread(m_dataBuf, 1, dataSize, fp) != dataSize)
        {
            result = HEVC_ERR_READ;
            goto loaddramexit;
        }
        
        // write dram param to sram
        for (i = 0; i < ((dataSize + 3) & ~3) / 4; i++)
        {
            WriteVerify32((SRAM_BASE_ADDR + DRAM_PARM_OFFSET) + (i * 4), m_dataBuf[i]);
        }
    }
    else
    {
        // get boot status
        m_device.HevcReadRegister((SRAM_BASE_ADDR + SRAM_STAT_FLAG), &val);
        if (val != 0x00000004 && val != 0x00000006 && val != 0x00000010)
        {
            result = HEVC_ERR_LOAD;
            goto loaddramexit;
        }
    }
    
    // load start
    WriteVerify32(SRAM_BASE_ADDR + SRAM_LOAD_FLAG, 0x00000001);
    m_device.HevcWriteRegister((SRAM_BASE_ADDR + SRAM_MODE_FLAG), 0x00000000);
    AJATime::Sleep(10);
    
    // status check
    mode = StatusCheck(SRAM_BASE_ADDR + SRAM_MODE_FLAG, 0x00000000, 5*1000, true, false);
    if (mode == 0x00000000)
    {
        result = HEVC_ERR_LOAD;
        goto loaddramexit;
    }
    
    printf("----- area ---------------- [write lv][gate tr][rd fifo][rd data][wr data]\n");
    
    for (i = 0; i < 4; i++)
    {
        m_device.HevcReadRegister((SRAM_BASE_ADDR + 0x0001EF00) + (i * 4), &val);
        if (i == 0)
            printf("ch.A: 0x20000000-0x3FFFFFFF   ");
        else if (i == 1)
            printf("ch.B: 0x40000000-0x7FFFFFFF   ");
        else if (i == 2)
            printf("ch.C: 0x80000000-0xBFFFFFFF   ");
        else
            printf("ch.D: 0xC0000000-0xDFFFFFFF   ");
        
        printf("%s      ", val & 0x01 ? "pass" : "fail");
        printf("%s     ",  val & 0x02 ? "pass" : "fail");
        printf("%s     ",  val & 0x04 ? "pass" : "fail");
        printf("%s     ",  val & 0x08 ? "pass" : "fail");
        printf("%s\n",     val & 0x10 ? "pass" : "fail");
    }
    
    printf("LoadDram success\n");
    
loaddramexit:
    if (fp != NULL)
    {
        fclose(fp);
    }
    
    return result;
}

HEVCError CNTV2HEVCFirmwareInstallerThread::DramTest(uint32_t offset, uint32_t size, uint32_t blockSize, uint32_t buf0, uint32_t buf1)
{
    uint32_t        mode;
    uint32_t        val, val2, val3;
    uint32_t        offsetEnd;
    uint32_t        pp, px;
    uint32_t        dataSize, remainSize;
    AJATimer        timer;
    
    printf("DramTest\n");
    
    m_device.HevcReadRegister((SRAM_BASE_ADDR + SRAM_MODE_FLAG), &mode);
    if (mode == 0x00000048)
    {
        return HEVC_ERR_BOOT;
    }
    if (mode == 0x000000FF)
    {
        return HEVC_ERR_NOT_LOADED;
    }
    if (mode == 0x00000000)
    {
        return HEVC_ERR_LOAD;
    }
    if ((mode & 0x00010000) == 0)
    {
        return HEVC_ERR_ALREADY_LOADED;
    }
    
    // don't understand this code it will always fail
    if (m_hevcInfo.pciId.subVendor != 0x00000000)
    {
        // get boot status
        m_device.HevcReadRegister((SRAM_BASE_ADDR + SRAM_STAT_FLAG), &val);
        if (val != 0x00000004 && val != 0x00000006 && val != 0x00000010)
        {
            return HEVC_ERR_LOAD;
        }
    }
    
    // command flag check
    m_device.HevcReadRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), &val);
    if (val == 0x000000FF)
    {
        return HEVC_ERR_BUSY;
    }
    
    m_dataBuf[0] = buf0;
    m_dataBuf[1] = buf1;
    
    // check offset
    if (offset < 0x20000000 || offset >= 0xE0000000)
    {
        return HEVC_ERR_OFFSET;
    }
    
    // check size
    if (size == 0 || size > 0xC0000000)
    {
        return HEVC_ERR_SIZE;
    }
    
    // check unit
    if (blockSize < 0x00000004 || blockSize > 0x00010000 || (blockSize & (blockSize - 1)))
    {
        return HEVC_ERR_SIZE;
    }
    
    offset &= ~(blockSize - 1);
    size = (size + blockSize - 1) & ~(blockSize - 1);
    
    if (size > 0xE0000000 - offset)
        size = 0xE0000000 - offset;
    
    offsetEnd = offset + size;
    
    printf("dram test start 0x%08X-0x%08X\n", offset, offset + size - 1);
    
    timer.Start();
    
    // parameter set
    WriteVerify32(SRAM_BASE_ADDR + 0x00004000, m_dataBuf[0]);
    WriteVerify32(SRAM_BASE_ADDR + 0x00004004, m_dataBuf[1]);
    WriteVerify32(SRAM_BASE_ADDR + 0x00004010, offset);
    WriteVerify32(SRAM_BASE_ADDR + 0x00004014, offsetEnd);
    WriteVerify32(SRAM_BASE_ADDR + 0x00004018, blockSize);
    WriteVerify32(SRAM_BASE_ADDR + 0x00004020, offset);
    
    dataSize = size / blockSize;
    
    PERCENT_PROGRESS_START();
    
    // dram test start
    m_device.HevcWriteRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), 0x000000FF);
    AJATime::Sleep(10);
    
    do
    {
        AJATime::Sleep(1);
        m_device.HevcReadRegister((SRAM_BASE_ADDR + 0x00004020), &val);
        remainSize = (offsetEnd - val) / blockSize;
        PERCENT_PROGRESS();
        m_device.HevcReadRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), &val);
    } while (val == 0x000000FF);
    
    if (val != 0)
    {
        m_device.HevcReadRegister((SRAM_BASE_ADDR + 0x00004024), &val);
        m_device.HevcReadRegister((SRAM_BASE_ADDR + 0x00004028), &val2);
        m_device.HevcReadRegister((SRAM_BASE_ADDR + 0x0000402C), &val3);
        fprintf(stderr, "dram test fail: 0x%08X W:0x%08X R:0x%08X\n", val, val2, val3);
        
        m_device.HevcReadRegister((SRAM_BASE_ADDR + 0x00004024), &offsetEnd);
        offset = offsetEnd & ~(blockSize - 1);
        
        // parameter set
        WriteVerify32(SRAM_BASE_ADDR + 0x00004000, m_dataBuf[0]);
        WriteVerify32(SRAM_BASE_ADDR + 0x00004004, m_dataBuf[1]);
        WriteVerify32(SRAM_BASE_ADDR + 0x00004010, offset);
        WriteVerify32(SRAM_BASE_ADDR + 0x00004014, offset + blockSize);
        WriteVerify32(SRAM_BASE_ADDR + 0x00004018, blockSize);
        WriteVerify32(SRAM_BASE_ADDR + 0x00004020, offset);
        
        // dram test start
        m_device.HevcWriteRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), 0x000000FF);
        AJATime::Sleep(10);
        
        // status check
        do
        {
            m_device.HevcReadRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), &val);
        } while (val == 0x000000FF);
        
        if (val != 0)
        {
            m_device.HevcReadRegister((SRAM_BASE_ADDR + 0x00004024), &val);
            m_device.HevcReadRegister((SRAM_BASE_ADDR + 0x00004028), &val2);
            m_device.HevcReadRegister((SRAM_BASE_ADDR + 0x0000402C), &val3);
            fprintf(stderr, "retry dram test fail: 0x%08X W:0x%08X R:0x%08X\n", val, val2, val3);
        }
        else
            fprintf(stderr, "retry dram test pass: 0x%08X-0x%08X\n", offset, offset + blockSize - 1);
        
        offset = offsetEnd - 16;
        if (offset < 0x20000000)
            offset = 0x20000000;
        
        offsetEnd += 20;
        if (offsetEnd > 0xE0000000)
            offsetEnd = 0xE0000000;
        
        while (offset < offsetEnd)
        {
            // parameter set
            WriteVerify32(SRAM_BASE_ADDR + 0x00004000, m_dataBuf[0]);
            WriteVerify32(SRAM_BASE_ADDR + 0x00004004, m_dataBuf[1]);
            WriteVerify32(SRAM_BASE_ADDR + 0x00004010, offset);
            WriteVerify32(SRAM_BASE_ADDR + 0x00004014, offset + 4);
            WriteVerify32(SRAM_BASE_ADDR + 0x00004018, 4);
            WriteVerify32(SRAM_BASE_ADDR + 0x00004020, offset);
            
            // dram test start
            m_device.HevcWriteRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), 0x000000FF);
            AJATime::Sleep(10);
            
            // status check
            do
            {
                m_device.HevcReadRegister((SRAM_BASE_ADDR + SRAM_CMND_FLAG), &val);
            } while (val == 0x000000FF);
            
            if (val != 0)
            {
                m_device.HevcReadRegister((SRAM_BASE_ADDR + 0x00004024), &val);
                m_device.HevcReadRegister((SRAM_BASE_ADDR + 0x00004028), &val2);
                m_device.HevcReadRegister((SRAM_BASE_ADDR + 0x0000402C), &val3);
                fprintf(stderr, "retry dram test fail: 0x%08X W:0x%08X R:0x%08X\n", val, val2, val3);
            }
            else
                fprintf(stderr, "retry dram test pass: 0x%08X\n", offset);
            
            offset += 4;
        }
        
        fprintf(stderr, "not completed\n");
        return HEVC_ERR_VERIFY;
    }
    
    PERCENT_PROGRESS_END();
    
    timer.Stop();
    printf("DramTest success\n");
    printf("Elapsed time for dram test: %4.3f seconds\n", (double)timer.ElapsedTime()/1000);
    return HEVC_ERR_NONE;
}

HEVCError CNTV2HEVCFirmwareInstallerThread::BinVerify(char* fileName, uint32_t fileOffset, uint32_t compareSize, uint32_t bar, uint32_t offset, uint32_t timeout)
{
    HEVCError   result = HEVC_ERR_NONE;
    FILE*       fp = NULL;
    uint32_t    val, tmp;
    uint32_t    size, dataSize, regoffset;
    uint64_t    startTime, currentTime;
    
    printf("Bin Verify\n");
    
    // open data file
    fp = fopen(fileName, "rb");
    if (fp == NULL)
    {
        result = HEVC_ERR_OPEN;
        goto bvexit;
    }
    
	// seek into the file offset
	fseek(fp, fileOffset, SEEK_SET);
    
    // get data size
    dataSize = compareSize;
    if (dataSize == 0)
    {
        result = HEVC_ERR_SIZE;
        goto bvexit;
    }
    
    if (bar == 0)
        regoffset = BAR0_OFFSET + offset;
    else if (bar == 2)
        regoffset = BAR2_OFFSET + offset;
    else if (bar == 4)
        regoffset = offset;
    else if (bar == 5)
        regoffset = BAR5_OFFSET + offset;
    else
    {
        result = HEVC_ERR_OFFSET;
        goto bvexit;
    }
    
    while (1)
    {
        if (dataSize < 4)
            size = dataSize;
        else
            size = 4;
        
        tmp = 0;
        if (fread(&tmp, 1, size, fp) != size)
        {
            result = HEVC_ERR_READ;
            goto bvexit;
        }
        
        startTime = AJATime::GetSystemMilliseconds();
        do
        {
            m_device.HevcReadRegister(regoffset, &val);
            val = val & (0xFFFFFFFF >> ((4 - size) * 8));
            currentTime = AJATime::GetSystemMilliseconds();
        } while (val != tmp && (currentTime-startTime <  timeout));
        
        if (val != tmp)
        {
            fprintf(stderr, "binary verify error\n");
            fprintf(stderr, "error: 0x%08X F:0x%08X R:0x%08X\n", offset, tmp, val);
            break;
        }
        
        if (dataSize <= 4)
        {
            dataSize = 0;
            break;
        }
        
        regoffset += 4;
        dataSize -= 4;
    }
    
    if (dataSize != 0)
    {
        result = HEVC_ERR_VERIFY;
        goto bvexit;
    }
    
    printf("Bin verify success\n");
    
bvexit:
    if (fp != NULL)
    {
        fclose(fp);
    }
    
    return result;
}

HEVCError CNTV2HEVCFirmwareInstallerThread::FlashMainModeCommon()
{
    HEVCError       result;
    
    result = Check();
    if (result != HEVC_ERR_NONE)
    {
        return result;
    }
    
    result = Boot();
    if (result != HEVC_ERR_NONE)
    {
        return result;
    }
    
    if (!IsBooted())
    {
        return HEVC_ERR_NOT_BOOTED;
    }
    
	// Load the flash
    result = LoadFlash();
    if (result != HEVC_ERR_NONE)
    {
        Stop();
    }
    return result;
}

HEVCError CNTV2HEVCFirmwareInstallerThread::FlashMode(char* fileName, uint32_t offset, uint32_t versionOffset)
{
    HEVCError       result;
    
    result = FlashMainModeCommon();
    if (result != HEVC_ERR_NONE)
    {
        return result;
    }
    
	// Erase sectors
    result = SectorErase(offset, 0x200000);
    if (result != HEVC_ERR_NONE)
    {
        Stop();
        return result;
    }
    
	// Program the bitfile
    result = Program(fileName, offset, 0, 0xFFFFFFFF);
    if (result != HEVC_ERR_NONE)
    {
        Stop();
        return result;
    }
    
    Stop();

	// If we are not in maintenance mode then verify version
    if (m_hevcInfo.pciId.subVendor != 0x00000000)
	{
   		m_device.HevcWriteRegister((BAR0_OFFSET + 0x90), 0x80000000);
		result = BinVerify(fileName, 4096, 60, 2, versionOffset, 5000);
	}

	return result;
}

HEVCError CNTV2HEVCFirmwareInstallerThread::FlashMCPU(char* fileName, uint32_t reset)
{
    HEVCError       result;
    uint32_t        val;
    
    result = FlashMainModeCommon();
    if (result != HEVC_ERR_NONE)
    {
        return result;
    }
    
	// Erase sectors
    result = SectorErase(0x00940000, 0x6C0000);
    if (result != HEVC_ERR_NONE)
    {
        Stop();
        return result;
    }
    
	// Program the bitfile
    result = Program(fileName, 0x00940000, 0, 0xFFFFFFFF);
    if (result != HEVC_ERR_NONE)
    {
        Stop();
        return result;
    }
    
    Stop();

    if (reset && (m_hevcInfo.pciId.subVendor != 0x00000000))
    {
        m_device.HevcWriteRegister((SRAM_BASE_ADDR + SRAM_RESET_OFFSET), 0x00000000);
        AJATime::Sleep(10);
        
       	val = StatusCheck(BAR2_OFFSET + 0x200, 0x00000001, 5*1000, false, false);
        if (val != 0x00000001)
        {
            result = HEVC_ERR_BOOT;
        }
        else
        {
            printf("Reboot success\n");
        }
    }
    return result;
}

HEVCError CNTV2HEVCFirmwareInstallerThread::FlashSystem(char* fileName, char* configName, ULWord flashOffset, ULWord fileOffset, ULWord length)
{
	(void) flashOffset;
	(void) fileOffset;
	(void) length;
    HEVCError       result;
    
    result = FlashMainModeCommon();
    if (result != HEVC_ERR_NONE)
    {
        return result;
    }
    
    // In maintenance mode erase the flash otherwise just erase the program and config sectors
    if (m_hevcInfo.pciId.subVendor == 0x00000000)
    {
        result = ChipErase();
        if (result != HEVC_ERR_NONE)
        {
            Stop();
            return result;
        }
    }
    else
    {
        // Erase sectors
        result = SectorErase(0x00000000, 0x100000);
        if (result != HEVC_ERR_NONE)
        {
            Stop();
            return result;
        }

        result = SectorErase(0x00700000, 0x240000);
        if (result != HEVC_ERR_NONE)
        {
            Stop();
            return result;
        }
    }
    
    // Program the bitfile
    result = Program(fileName, 0x00000000, 0, 0xFFFFFFFF);
    if (result != HEVC_ERR_NONE)
    {
        Stop();
        return result;
    }

    // Program the config file
    result = Program(configName, 0x00800000, 0, 0xFFFFFFFF);
    if (result != HEVC_ERR_NONE)
    {
        Stop();
        return result;
    }

    // Program the version of the system file
    result = Program(fileName, 0x00880000, 0x00010C00, 20);

  	Stop();
    return result;
}

bool CNTV2HEVCFirmwareInstallerThread::WriteRegisterBar4(ULWord address, ULWord value, ULWord mask, ULWord shift)
{
	HevcMessageRegister message;
	bool result;

	memset(&message, 0, sizeof(HevcMessageRegister));
	message.header.type = Hevc_MessageId_Register;
	message.header.size = sizeof(HevcMessageRegister);
	message.data.address = address;
	message.data.writeValue = value;
	message.data.readValue = 0;
	message.data.mask = mask;
	message.data.shift = shift;
	message.data.write = true;
	message.data.read = false;
	message.data.forceBar4 = true;

	result = m_device.HevcSendMessage((HevcMessageHeader*)&message);

	return result;
}


