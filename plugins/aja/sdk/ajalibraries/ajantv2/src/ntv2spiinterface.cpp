/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2spiinterface.cpp
	@brief		Implementation of CNTV2AxiSpiFlash class.
	@copyright	(C) 2017-2021 AJA Video Systems, Inc.
**/
#include "ntv2spiinterface.h"

#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "ntv2registersmb.h"
#include "ntv2mcsfile.h"

using namespace std;

static bool verify_vectors(const std::vector<uint8_t> &dataWritten, const std::vector<uint8_t> &dataRead, bool verbose = false)
{
    bool result = false;

    if (equal(dataWritten.begin(), dataWritten.end(), dataRead.begin()))
    {
        result = true;
    }
    else
    {
        result = false;
        if (verbose)
        {
            pair<vector<uint8_t>::const_iterator, vector<uint8_t>::const_iterator> p;
            p = mismatch(dataWritten.begin(), dataWritten.end(), dataRead.begin());
            int64_t byteMismatchOffset = distance(dataWritten.begin(), p.first);
            ostringstream ossWrite;
            ossWrite << "0x" << std::setw(2) << std::setfill('0') << hex << (int)*p.first;
            ostringstream ossRead;
            ossRead << "0x" << std::setw(2) << std::setfill('0') << hex << (int)*p.second;

            ++p.first; ++p.second;
            p = mismatch(p.first, dataWritten.end(), p.second);

            int errorCount = 0;
            while(p.first != dataWritten.end() && p.second != dataRead.end())
            {
                errorCount++;
                ++p.first; ++p.second;
                p = mismatch(p.first, dataWritten.end(), p.second);
            }

            cout << "Verifying write of: " << dataWritten.size() << " bytes, failed at byte index: " << byteMismatchOffset <<
                    ", byte written to device should be: " << ossWrite.str() << ", byte read back from device is: " << ossRead.str() << ".\n" <<
                    "There are " << errorCount << " other mismatches after this." << endl;
        }
    }

    return result;
}

inline void print_flash_status(const string& label, uint32_t curValue, uint32_t maxValue, uint32_t& lastPercentage)
{
    if (maxValue == 0)
        return;

    uint32_t percentage = (uint32_t)(((double)curValue/(double)maxValue)*100.0);
    if (percentage != lastPercentage)
    {
        lastPercentage = percentage;
        cout << label << " status: " << dec << lastPercentage << "%   \r" << flush;
    }
}

inline void print_flash_status_final(const string& label)
{
    cout << label << " status: 100%   " << endl;
}

// Cypress/Spansion commands

//const uint32_t CYPRESS_FLASH_WRITE_STATUS_COMMAND  = 0x01;
const uint32_t CYPRESS_FLASH_WRITEDISABLE_COMMAND  = 0x04;
const uint32_t CYPRESS_FLASH_READ_STATUS_COMMAND   = 0x05;
const uint32_t CYPRESS_FLASH_WRITEENABLE_COMMAND   = 0x06;
const uint32_t CYPRESS_FLASH_READFAST_COMMAND      = 0x0C; //4 byte address
const uint32_t CYPRESS_FLASH_PAGE_PROGRAM_COMMAND  = 0x12; //4 byte address
//const uint32_t CYPRESS_FLASH_READ_COMMAND          = 0x13; //4 byte address
const uint32_t CYPRESS_FLASH_READBANK_COMMAND      = 0x16;
const uint32_t CYPRESS_FLASH_WRITEBANK_COMMAND     = 0x17;
const uint32_t CYPRESS_FLASH_SECTOR4K_ERASE_COMMAND= 0x21; //4 byte address
const uint32_t CYPRESS_FLASH_READ_CONFIG_COMMAND   = 0x35;
const uint32_t CYPRESS_FLASH_READ_JEDEC_ID_COMMAND = 0x9F;
const uint32_t CYPRESS_FLASH_SECTOR_ERASE_COMMAND  = 0xDC; //4 byte address

inline bool has_4k_start_sectors(const uint32_t reportedSectorSize)
{
    if (reportedSectorSize <= 0x20000)
        return true;
    else
        return false;
}

static uint32_t size_for_sector_number(const uint32_t reportedSectorSize, const uint32_t sector)
{
    if (has_4k_start_sectors(reportedSectorSize) && sector < 32)
        return 4 * 1024;
    else
        return reportedSectorSize;
}

static uint32_t erase_cmd_for_sector(const uint32_t reportedSectorSize, const uint32_t sector)
{
    if (has_4k_start_sectors(reportedSectorSize) && sector < 32)
        return CYPRESS_FLASH_SECTOR4K_ERASE_COMMAND; // 4P4E command, 4byte
    else
        return CYPRESS_FLASH_SECTOR_ERASE_COMMAND; // 4SE command,  4byte
}

static uint32_t sector_for_address(uint32_t sectorSizeBytes, uint32_t address)
{
    if (sectorSizeBytes == 0)
        return 0;

    uint32_t sector = 0;
    if (has_4k_start_sectors(sectorSizeBytes))
    {
         if (address < 0x20000)
         {
             uint32_t sector4kSizeBytes = size_for_sector_number(sectorSizeBytes, 0);
             sector = address / sector4kSizeBytes;
         }
         else
         {
             sector += 32;
             sector += (address-0x20000) / sectorSizeBytes;
         }
    }
    else
    {
        sector = address / sectorSizeBytes;
    }

    return sector;
}

static uint32_t address_for_sector(uint32_t sectorSizeBytes, uint32_t sector)
{
    uint32_t address=0;
    if (has_4k_start_sectors(sectorSizeBytes))
    {
        if (sector < 32)
            address = sector * size_for_sector_number(sectorSizeBytes, 0);
        else
        {
            address = 32 * size_for_sector_number(sectorSizeBytes, 0);
            address += (sector - 32) * sectorSizeBytes;
        }
    }
    else
    {
        address = sector * sectorSizeBytes;
    }

    return address;
}

inline ProgramState programstate_for_address(uint32_t address, int mode)
{
    ProgramState ps;
    switch(mode)
    {
        case 0: ps = (address < 0x100000) ? kProgramStateEraseBank3   : kProgramStateEraseBank4;   break;
        case 1: ps = (address < 0x100000) ? kProgramStateProgramBank3 : kProgramStateProgramBank4; break;
        case 2: ps = (address < 0x100000) ? kProgramStateVerifyBank3  : kProgramStateVerifyBank4;  break;
        default: ps = kProgramStateFinished; break;
    }

    return ps;
}

inline uint32_t make_spi_ready(CNTV2Card& device)
{
    uint32_t deviceId=0;
    device.ReadRegister(kRegBoardID, deviceId);
    return deviceId;
}

#define wait_for_flash_status_ready() { uint8_t fs=0x00; do { FlashReadStatus(fs); } while(fs & 0x1); }

CNTV2AxiSpiFlash::CNTV2AxiSpiFlash(int index, bool verbose)
    : CNTV2SpiFlash(verbose), mBaseByteAddress(0x300000), mSize(0), mSectorSize(0)
{
    mSpiResetReg     = (mBaseByteAddress + 0x40) / 4;
    mSpiControlReg   = (mBaseByteAddress + 0x60) / 4;
    mSpiStatusReg    = (mBaseByteAddress + 0x64) / 4;
    mSpiWriteReg     = (mBaseByteAddress + 0x68) / 4;
    mSpiReadReg      = (mBaseByteAddress + 0x6c) / 4;
    mSpiSlaveReg     = (mBaseByteAddress + 0x70) / 4;
    mSpiGlobalIntReg = (mBaseByteAddress + 0x1c) / 4;
    AsNTV2DriverInterfaceRef(mDevice).Open(UWord(index));

    SpiReset();

    uint8_t manufactureID;
    uint8_t memInerfaceType;
    uint8_t memDensity;
    uint8_t sectorArchitecture;
    uint8_t familyID;
    bool good = FlashDeviceInfo(manufactureID, memInerfaceType, memDensity, sectorArchitecture, familyID);
    if (good)
    {
        switch(memDensity)
        {
            case 0x18: mSize = 16 * 1024 * 1024; break;
            case 0x19: mSize = 32 * 1024 * 1024; break;
            default:   mSize = 0;                break;
        }

        switch(sectorArchitecture)
        {
            case 0x00: mSectorSize = 256 * 1024; break;
            case 0x01: mSectorSize =  64 * 1024; break;
            default:   mSectorSize = 0;          break;
        }
    }

    uint8_t configValue;
    good = FlashReadConfig(configValue);
    uint8_t statusValue;
    good = FlashReadStatus(statusValue);
}

CNTV2AxiSpiFlash::~CNTV2AxiSpiFlash()
{
}

bool CNTV2AxiSpiFlash::DeviceSupported(NTV2DeviceID deviceId)
{
	if ((deviceId == DEVICE_ID_IOIP_2022) ||
		(deviceId == DEVICE_ID_IOIP_2110) ||
		(deviceId == DEVICE_ID_IOIP_2110_RGB12))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool CNTV2AxiSpiFlash::Read(const uint32_t address, std::vector<uint8_t> &data, uint32_t maxBytes)
{
    const uint32_t pageSize = 256;
    ProgramState ps = programstate_for_address(address, 2);

    uint32_t pageAddress = address;
    uint32_t numPages = (uint32_t)ceil((double)maxBytes/(double)pageSize);

    uint32_t bytesLeftToTransfer = maxBytes;
    uint32_t bytesTransfered = 0;
    mDevice.WriteRegister(kVRegFlashState, ps);
    mDevice.WriteRegister(kVRegFlashSize, bytesLeftToTransfer);
    mDevice.WriteRegister(kVRegFlashStatus, 0);

    uint32_t lastPercent = 0;
    for(uint32_t p=0;p<numPages;p++)
    {
        vector<uint8_t> commandSequence;
        commandSequence.push_back(CYPRESS_FLASH_READFAST_COMMAND);
        FlashFixAddress(pageAddress, commandSequence);

        uint32_t bytesToTransfer = pageSize;
        if (bytesLeftToTransfer < pageSize)
            bytesToTransfer = bytesLeftToTransfer;

        vector<uint8_t> dummyInput;
        SpiTransfer(commandSequence, dummyInput, data, bytesToTransfer);
        wait_for_flash_status_ready();

        bytesLeftToTransfer -= bytesToTransfer;
        pageAddress += pageSize;

        bytesTransfered += bytesToTransfer;

        if (mVerbose && maxBytes > 0)
            print_flash_status("Verify", bytesTransfered, maxBytes, lastPercent);

        mDevice.WriteRegister(kVRegFlashState, ps);
        mDevice.WriteRegister(kVRegFlashStatus, bytesTransfered);
    }

    if (mVerbose)
        print_flash_status_final("Verify");

    return true;
}

bool CNTV2AxiSpiFlash::Write(const uint32_t address, const std::vector<uint8_t> data, uint32_t maxBytes)
{
    const uint32_t pageSize = 256;
    ProgramState ps = programstate_for_address(address, 1);

    uint32_t maxWrite = maxBytes;
    if (maxWrite > data.size())
        maxWrite = (uint32_t)data.size();

    std::vector<uint8_t> dummyOutput;

    uint32_t pageAddress = address;
    uint32_t numPages = (uint32_t)ceil((double)maxWrite/(double)pageSize);

    uint32_t bytesTransfered = 0;
    mDevice.WriteRegister(kVRegFlashState, ps);
    mDevice.WriteRegister(kVRegFlashSize, maxWrite);
    mDevice.WriteRegister(kVRegFlashStatus, 0);

    uint32_t lastPercent = 0;
    for(uint32_t p=0;p<numPages;p++)
    {
        vector<uint8_t> commandSequence;
        commandSequence.push_back(CYPRESS_FLASH_PAGE_PROGRAM_COMMAND);
        FlashFixAddress(pageAddress, commandSequence);

        vector<uint8_t> pageData;
        for(unsigned i=0;i<pageSize;i++)
        {
            uint32_t offset = (p*pageSize)+i;
            if (offset >= data.size())
                break;

            pageData.push_back(data.at(offset));
        }

        // enable write
        SpiEnableWrite(true);

        SpiTransfer(commandSequence, pageData, dummyOutput, (uint32_t)pageData.size());
        wait_for_flash_status_ready();

        // disable write
        SpiEnableWrite(false);

        pageAddress += pageSize;
		bytesTransfered += static_cast<uint32_t>(pageData.size());

        if (mVerbose && maxWrite > 0)
            print_flash_status("Program", bytesTransfered, maxWrite, lastPercent);

        mDevice.WriteRegister(kVRegFlashState, ps);
        mDevice.WriteRegister(kVRegFlashStatus, bytesTransfered);
    }

    if (mVerbose)
        print_flash_status_final("Program");

    return true;
}

bool CNTV2AxiSpiFlash::Erase(const uint32_t address, uint32_t bytes)
{
    //testing
#if 0
    // 64k
    uint32_t test1Addr = 0x20000;
    uint32_t test1ExpectedSector = 32;

    // 4k
    uint32_t test2Addr = 0x1F000;
    uint32_t test2ExpectedSector = 31;

    // 64k
    uint32_t test3Addr = 0x1FF0000;
    uint32_t test3ExpectedSector = 541;

    uint32_t test1Res = sector_for_address(mSectorSize, test1Addr);
    uint32_t test2Res = sector_for_address(mSectorSize, test2Addr);
    uint32_t test3Res = sector_for_address(mSectorSize, test3Addr);

    uint32_t test1AddrRes = address_for_sector(mSectorSize, test1ExpectedSector);
    uint32_t test2AddrRes = address_for_sector(mSectorSize, test2ExpectedSector);
    uint32_t test3AddrRes = address_for_sector(mSectorSize, test3ExpectedSector);

    uint32_t test1Cmd = erase_cmd_for_sector(mSectorSize, test1ExpectedSector);
    uint32_t test2Cmd = erase_cmd_for_sector(mSectorSize, test2ExpectedSector);
    uint32_t test3Cmd = erase_cmd_for_sector(mSectorSize, test3ExpectedSector);
    return true;
#endif

    ProgramState ps = programstate_for_address(address, 0);

    uint32_t startSector = sector_for_address(mSectorSize, address);
    uint32_t endSector = sector_for_address(mSectorSize, address + bytes);

    uint32_t cmd = erase_cmd_for_sector(mSectorSize, startSector);
    uint32_t sectorAddress = address_for_sector(mSectorSize, startSector);

    vector<uint8_t> commandSequence;
    commandSequence.push_back(cmd);
    FlashFixAddress(address, commandSequence);

    uint32_t lastPercent = 0;

    if (mVerbose && endSector > startSector)
        print_flash_status("Erase", startSector, endSector-startSector, lastPercent);

    // enable write
    SpiEnableWrite(true);

    vector<uint8_t> dummyInput;
    vector<uint8_t> dummyOutput;
    SpiTransfer(commandSequence, dummyInput, dummyOutput, bytes);
    wait_for_flash_status_ready();

    // disable write
    SpiEnableWrite(false);

    // Handle the case of erase spanning sectors
    if (endSector > startSector)
    {
        uint32_t numSectors = endSector-startSector;
        mDevice.WriteRegister(kVRegFlashState, ps);
        mDevice.WriteRegister(kVRegFlashSize, numSectors);
        mDevice.WriteRegister(kVRegFlashStatus, 0);

        uint32_t start = startSector;
        while (start < endSector)
        {
            ++start;
            cmd = erase_cmd_for_sector(mSectorSize, start);
            sectorAddress = address_for_sector(mSectorSize, start);

            vector<uint8_t> commandSequence2;
            commandSequence2.push_back(cmd);
            FlashFixAddress(sectorAddress, commandSequence2);

            // enable write
            SpiEnableWrite(true);

            //vector<uint8_t> dummyInput;
            SpiTransfer(commandSequence2, dummyInput, dummyOutput, bytes);
            wait_for_flash_status_ready();

            // disable write
            SpiEnableWrite(false);

            uint32_t curProgress = start-startSector;
            if (mVerbose)
                print_flash_status("Erase", curProgress, endSector-startSector, lastPercent);

            mDevice.WriteRegister(kVRegFlashState, ps);
            mDevice.WriteRegister(kVRegFlashStatus, curProgress);
        }

        if (mVerbose)
            print_flash_status_final("Erase");

    }

    return true;
}

bool CNTV2AxiSpiFlash::Verify(const uint32_t address, const std::vector<uint8_t>& dataWritten)
{
    vector<uint8_t> verifyData;
    bool readGood = Read(address, verifyData, uint32_t(dataWritten.size()));

    if (readGood == false)
        return false;

    return verify_vectors(dataWritten, verifyData, mVerbose);
}

uint32_t CNTV2AxiSpiFlash::Size(SpiFlashSection sectionID)
{
    uint32_t retVal = 0;

    switch(sectionID)
    {
        case SPI_FLASH_SECTION_UBOOT:      retVal = 0x00080000; break;
        case SPI_FLASH_SECTION_KERNEL:     retVal = 0x00C00000; break;
        case SPI_FLASH_SECTION_LICENSE:    retVal = 0x00040000; break;
        case SPI_FLASH_SECTION_MCSINFO:    retVal = 0x00040000; break;
        case SPI_FLASH_SECTION_MAC:        retVal = 0x00040000; break;
        case SPI_FLASH_SECTION_SERIAL:     retVal = 0x00040000; break;
        case SPI_FLASH_SECTION_TOTAL:      retVal = mSize;      break;

        default:
            break;
    }

    return retVal;
}

uint32_t CNTV2AxiSpiFlash::Offset(SpiFlashSection sectionID)
{
    uint32_t retVal = 0xffffffff;

    switch(sectionID)
    {
        case SPI_FLASH_SECTION_UBOOT:      retVal = 0x00000000; break;
        case SPI_FLASH_SECTION_KERNEL:     retVal = 0x00100000; break;
        case SPI_FLASH_SECTION_LICENSE:    retVal = 0x01F00000; break;
        case SPI_FLASH_SECTION_MCSINFO:    retVal = 0x01F40000; break;
        case SPI_FLASH_SECTION_MAC:        retVal = 0x01F80000; break;
        case SPI_FLASH_SECTION_SERIAL:     retVal = 0x01FC0000; break;
        case SPI_FLASH_SECTION_TOTAL:      retVal = 0;          break;

        default:
            break;
    }

    return retVal;
}

bool CNTV2AxiSpiFlash::NTV2DeviceOk()
{
    if (mDevice.IsOpen() == false)
        return false;
    if (DeviceSupported(mDevice.GetDeviceID()) == false)
        return false;

    return true;
}

void CNTV2AxiSpiFlash::SpiReset()
{
    if (!NTV2DeviceOk())
        return;

    mDevice.WriteRegister(mSpiSlaveReg, 0x0);
    SpiResetFifos();

    // make sure in 32bit mode
    uint8_t bankAddressVal=0;
    FlashReadBankAddress(bankAddressVal);
    FlashWriteBankAddress(bankAddressVal | 0x80);
}

bool CNTV2AxiSpiFlash::SpiResetFifos()
{
    if (!NTV2DeviceOk())
        return false;

    uint32_t spi_ctrl_val=0xe6;
    return mDevice.WriteRegister(mSpiControlReg, spi_ctrl_val);
}

void CNTV2AxiSpiFlash::SpiEnableWrite(bool enable)
{
    // see AXI Quad SPI v3.2 guide page 99

    // 1 disable the master transaction by setting 0x100 high in spi control reg
    make_spi_ready(mDevice);
    mDevice.WriteRegister(mSpiControlReg, 0x186);

    // 2 issue the write enable command
    if (enable)
    {
        make_spi_ready(mDevice);
        mDevice.WriteRegister(mSpiWriteReg, CYPRESS_FLASH_WRITEENABLE_COMMAND);
    }
    else
    {
        make_spi_ready(mDevice);
        mDevice.WriteRegister(mSpiWriteReg, CYPRESS_FLASH_WRITEDISABLE_COMMAND);
    }

    // 3 issue chip select
    make_spi_ready(mDevice);
    mDevice.WriteRegister(mSpiSlaveReg, 0x00);

    // 4 enable the master transaction by setting 0x100 low in spi control reg
    uint32_t spi_ctrl_val = 0;
    make_spi_ready(mDevice);
    mDevice.ReadRegister(mSpiControlReg, spi_ctrl_val);
    spi_ctrl_val &= ~0x100;
    make_spi_ready(mDevice);
    mDevice.WriteRegister(mSpiControlReg, spi_ctrl_val);

    // 5 deassert chip select
    make_spi_ready(mDevice);
    mDevice.WriteRegister(mSpiSlaveReg, 0x01);

    // 6 disable the master transaction by setting 0x100 high in spi control reg
    make_spi_ready(mDevice);
    mDevice.ReadRegister(mSpiControlReg, spi_ctrl_val);
    spi_ctrl_val |= 0x100;
    make_spi_ready(mDevice);
    mDevice.WriteRegister(mSpiControlReg, spi_ctrl_val);
}

bool CNTV2AxiSpiFlash::FlashDeviceInfo(uint8_t& manufactureID, uint8_t& memInerfaceType,
                                  uint8_t& memDensity, uint8_t& sectorArchitecture,
                                  uint8_t& familyID)
{
    vector<uint8_t> commandSequence;
    commandSequence.push_back(CYPRESS_FLASH_READ_JEDEC_ID_COMMAND);

    vector<uint8_t> dummyInput;
    vector<uint8_t> resultData;
    bool result = SpiTransfer(commandSequence, dummyInput, resultData, 6);
    if (result && resultData.size() == 6)
    {
        manufactureID      = resultData.at(0);
        memInerfaceType    = resultData.at(1);
        memDensity         = resultData.at(2);
        sectorArchitecture = resultData.at(4);
        familyID           = resultData.at(5);
    }

    return result;
}

bool CNTV2AxiSpiFlash::FlashReadConfig(uint8_t& configValue)
{
    vector<uint8_t> commandSequence;
    commandSequence.push_back(CYPRESS_FLASH_READ_CONFIG_COMMAND);

    vector<uint8_t> dummyInput;
    vector<uint8_t> resultData;
    bool result = SpiTransfer(commandSequence, dummyInput, resultData, 1);
    if (result && resultData.size() > 0)
    {
        configValue = resultData.at(0);
    }
    return result;
}

bool CNTV2AxiSpiFlash::FlashReadStatus(uint8_t& statusValue)
{
    vector<uint8_t> commandSequence;
    commandSequence.push_back(CYPRESS_FLASH_READ_STATUS_COMMAND);

    vector<uint8_t> dummyInput;
    vector<uint8_t> resultData;
    bool result = SpiTransfer(commandSequence, dummyInput, resultData, 1);
    if (result && resultData.size() > 0)
    {
        statusValue = resultData.at(0);
    }
    return result;
}

bool CNTV2AxiSpiFlash::FlashReadBankAddress(uint8_t& bankAddressVal)
{
    vector<uint8_t> commandSequence;
    commandSequence.push_back(CYPRESS_FLASH_READBANK_COMMAND);

    vector<uint8_t> dummyInput;
    vector<uint8_t> resultData;
    bool result = SpiTransfer(commandSequence, dummyInput, resultData, 1);
    if (result && resultData.size() > 0)
    {
        bankAddressVal = resultData.at(0);
    }
    return result;
}

bool CNTV2AxiSpiFlash::FlashWriteBankAddress(const uint8_t bankAddressVal)
{
    vector<uint8_t> commandSequence;
    commandSequence.push_back(CYPRESS_FLASH_WRITEBANK_COMMAND);

    vector<uint8_t> input;
    input.push_back(bankAddressVal);
    std::vector<uint8_t> dummyOutput;
    return SpiTransfer(commandSequence, input, dummyOutput, 1);
}

void CNTV2AxiSpiFlash::FlashFixAddress(const uint32_t address, std::vector<uint8_t>& commandSequence)
{
    // turn the address into a bytes stream and change ordering to match what
    // the flash chip wants to see (MSB)
    commandSequence.push_back((address & (0xff000000)) >> 24);
    commandSequence.push_back((address & (0x00ff0000)) >> 16);
    commandSequence.push_back((address & (0x0000ff00)) >>  8);
    commandSequence.push_back((address & (0x000000ff)) >>  0);
}

bool CNTV2AxiSpiFlash::SpiTransfer(std::vector<uint8_t> commandSequence,
                                   const std::vector<uint8_t> inputData,
                                   std::vector<uint8_t>& outputData, uint32_t maxByteCutoff)
{
    bool retVal = true;

    if (commandSequence.empty())
        return false;

    make_spi_ready(mDevice);
    SpiResetFifos();

    // issue chip select
    make_spi_ready(mDevice);
    mDevice.WriteRegister(mSpiSlaveReg, 0x00);

    // issue the command & arguments
    uint32_t dummyVal = 0;
    for(unsigned i=0;i<commandSequence.size();++i)
    {
        make_spi_ready(mDevice);
        mDevice.WriteRegister(mSpiWriteReg, (ULWord)commandSequence.at(i));
        if (commandSequence.size() > 1)
        {
            make_spi_ready(mDevice);
            mDevice.ReadRegister(mSpiReadReg, dummyVal);
        }
    }

    if (commandSequence.at(0) == CYPRESS_FLASH_SECTOR4K_ERASE_COMMAND ||
        commandSequence.at(0) == CYPRESS_FLASH_SECTOR_ERASE_COMMAND)
    {
        // an erase command, don't need to do anything else
    }
    else if (inputData.empty() == false)
    {
        // a write command
        uint32_t maxWrite = maxByteCutoff;
        if (maxWrite > inputData.size())
            maxWrite = (uint32_t)inputData.size();

        for(uint32_t i=0;i<maxWrite;++i)
        {
            make_spi_ready(mDevice);
            mDevice.WriteRegister(mSpiWriteReg, inputData.at(i));
        }
    }
    else
    {
        // a read command
        uint32_t val = 0;
        for(uint32_t i=0;i<maxByteCutoff+1;++i)
        {
            make_spi_ready(mDevice);
            mDevice.WriteRegister(mSpiWriteReg, 0x0); //dummy

            make_spi_ready(mDevice);
            mDevice.ReadRegister(mSpiReadReg, val);

            // the first byte back is a dummy when reading flash
            if (i > 0)
                outputData.push_back(val);
        }
    }

    // deassert chip select
    make_spi_ready(mDevice);
    mDevice.WriteRegister(mSpiSlaveReg, 0x01);

    return retVal;
}
