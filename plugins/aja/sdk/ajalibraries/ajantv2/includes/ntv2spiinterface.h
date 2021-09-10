/* SPDX-License-Identifier: MIT */
/**
    @file		ntv2spiinterface.h
    @brief		Declares the CNTV2SpiFlash and CNTV2AxiSpiFlash classes.
    @copyright	(C) 2017-2021 AJA Video Systems, Inc.    All rights reserved.
**/
#ifndef NTV2SPIINTERFACE_H
#define NTV2SPIINTERFACE_H

#include "ntv2card.h"

typedef enum
{
    SPI_FLASH_SECTION_UBOOT,
    SPI_FLASH_SECTION_KERNEL,
    SPI_FLASH_SECTION_LICENSE,
    SPI_FLASH_SECTION_MCSINFO,
    SPI_FLASH_SECTION_MAC,
    SPI_FLASH_SECTION_SERIAL,

    SPI_FLASH_SECTION_TOTAL // should be at end, represents the whole flash chip
}SpiFlashSection;

class CNTV2SpiFlash
{
public:
    CNTV2SpiFlash(bool verbose = false) : mVerbose(verbose) {}
    virtual ~CNTV2SpiFlash() {}

    virtual bool Read(const uint32_t address, std::vector<uint8_t> &data, uint32_t maxBytes = 1) = 0;
    virtual bool Write(const uint32_t address, const std::vector<uint8_t> data, uint32_t maxBytes = 1) = 0;
    virtual bool Erase(const uint32_t address, uint32_t bytes) = 0;
    virtual bool Verify(const uint32_t address, const std::vector<uint8_t>& dataWritten) = 0;
    virtual uint32_t Size(SpiFlashSection sectionID = SPI_FLASH_SECTION_TOTAL) = 0;
    virtual uint32_t Offset(SpiFlashSection sectionID = SPI_FLASH_SECTION_TOTAL) = 0;
    virtual void SetVerbosity(bool verbose) {mVerbose = verbose;}
    virtual bool GetVerbosity() {return mVerbose;}

    static bool DeviceSupported(NTV2DeviceID deviceId) {(void)deviceId; return false;}

protected:
    bool mVerbose;
};

class CNTV2AxiSpiFlash : public CNTV2SpiFlash
{
public:
    CNTV2AxiSpiFlash(int index = 0, bool verbose = false);
    virtual ~CNTV2AxiSpiFlash();

    // common flash interface
    virtual bool Read(const uint32_t address, std::vector<uint8_t> &data, uint32_t maxBytes = 1);
    virtual bool Write(const uint32_t address, const std::vector<uint8_t> data, uint32_t maxBytes = 1);
    virtual bool Erase(const uint32_t address, uint32_t bytes);
    virtual bool Verify(const uint32_t address, const std::vector<uint8_t>& dataWritten);
    virtual uint32_t Size(SpiFlashSection sectionID = SPI_FLASH_SECTION_TOTAL);
    virtual uint32_t Offset(SpiFlashSection sectionID = SPI_FLASH_SECTION_TOTAL);
    static bool DeviceSupported(NTV2DeviceID deviceId);

    // Axi specific
private:
    bool NTV2DeviceOk();

    void SpiReset();
    bool SpiResetFifos();
    void SpiEnableWrite(bool enable);
    bool SpiTransfer(std::vector<uint8_t> commandSequence,
                     const std::vector<uint8_t> inputData,
                     std::vector<uint8_t>& outputData, uint32_t maxByteCutoff = 1);

    bool FlashDeviceInfo(uint8_t& manufactureID, uint8_t& memInerfaceType,
                         uint8_t& memDensity, uint8_t& sectorArchitecture,
                         uint8_t& familyID);
    bool FlashReadConfig(uint8_t& configValue);
    bool FlashReadStatus(uint8_t& statusValue);
    bool FlashReadBankAddress(uint8_t& bankAddressVal);
    bool FlashWriteBankAddress(const uint8_t bankAddressVal);

    void FlashFixAddress(const uint32_t address, std::vector<uint8_t>& commandSequence);

    uint32_t    mBaseByteAddress;
    uint32_t    mSize;
    uint32_t    mSectorSize;

    CNTV2Card   mDevice;

    uint32_t    mSpiResetReg;
    uint32_t    mSpiControlReg;
    uint32_t    mSpiStatusReg;
    uint32_t    mSpiWriteReg;
    uint32_t    mSpiReadReg;
    uint32_t    mSpiSlaveReg;
    uint32_t    mSpiGlobalIntReg;
};

#endif
