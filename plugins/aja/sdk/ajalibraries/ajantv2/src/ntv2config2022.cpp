/* SPDX-License-Identifier: MIT */
/**
    @file		ntv2config2022.cpp
    @brief		Implements the CNTV2Config2022 class.
    @copyright	(C) 2014-2021 AJA Video Systems, Inc.
**/

#include "ntv2config2022.h"
#include "ntv2configts2022.h"
#include "ntv2endian.h"
#include "ntv2utils.h"
#include "ntv2card.h"
#include <sstream>

#if defined (AJALinux) || defined (AJAMac)
    #include <stdlib.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

using namespace std;
void tx_2022_channel::init()
{
    sfp1Enable          = true;
    sfp2Enable          = false;

    sfp1RemoteIP.erase();
    sfp1LocalPort       = 0;
    sfp1RemotePort      = 0;

    sfp2RemoteIP.erase();
    sfp2LocalPort       = 0;
    sfp2RemotePort      = 0;

    tos                 = 0x64;
    ttl                 = 0x40;
    ssrc                = 0;
}

bool tx_2022_channel::operator != ( const tx_2022_channel &other )
{
    return !(*this == other);
}

bool tx_2022_channel::operator == ( const tx_2022_channel &other )
{
    if ((sfp1Enable			== other.sfp1Enable)        &&
        (sfp2Enable			== other.sfp2Enable)		&&
		
        (sfp1LocalPort      == other.sfp1LocalPort)     &&
        (sfp1RemoteIP       == other.sfp1RemoteIP)      &&
        (sfp1RemotePort     == other.sfp1RemotePort)    &&

        (sfp2LocalPort      == other.sfp2LocalPort)     &&
        (sfp2RemoteIP       == other.sfp2RemoteIP)      &&
        (sfp2RemotePort     == other.sfp2RemotePort))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void rx_2022_channel::init()
{
    sfp1Enable         = true;
    sfp2Enable         = false;

    sfp1RxMatch  = 0;
    sfp1SourceIP.erase();
    sfp1DestIP.erase();
    sfp1SourcePort = 0;
    sfp1DestPort = 0;
    sfp1Vlan = 0;

    sfp2RxMatch  = 0;
    sfp2SourceIP.erase();
    sfp2DestIP.erase();
    sfp2SourcePort = 0;
    sfp2DestPort = 0;
    sfp2Vlan = 0;

    ssrc = 0;
    playoutDelay = 50;
}

bool rx_2022_channel::operator != ( const rx_2022_channel &other )
{
    return !(*this == other);
}

bool rx_2022_channel::operator == ( const rx_2022_channel &other )
{
    if ((sfp1Enable			== other.sfp1Enable)        &&
        (sfp2Enable			== other.sfp2Enable)		&&
		
        (sfp1RxMatch        == other.sfp1RxMatch)		&&
        (sfp1SourceIP       == other.sfp1SourceIP)		&&
        (sfp1DestIP			== other.sfp1DestIP)        &&
        (sfp1SourcePort		== other.sfp1SourcePort)    &&
        (sfp1DestPort       == other.sfp1DestPort)		&&
        (sfp1Vlan           == other.sfp1Vlan)			&&
		
        (sfp2RxMatch        == other.sfp2RxMatch)		&&
        (sfp2SourceIP       == other.sfp2SourceIP)		&&
        (sfp2DestIP         == other.sfp2DestIP)		&&
        (sfp2SourcePort     == other.sfp2SourcePort)	&&
        (sfp2DestPort       == other.sfp2DestPort)		&&
        (sfp2Vlan           == other.sfp2Vlan)			&&
		
        (ssrc               == other.ssrc)              &&
        (playoutDelay       == other.playoutDelay))
    {
        return true;
    }
    else
    {
        return false;
    }
}


void j2kEncoderConfig::init()
{
    videoFormat     = NTV2_FORMAT_720p_5994;
    ullMode         = false;
    bitDepth        = 10;
    chromaSubsamp   = kJ2KChromaSubSamp_422_Standard;
    mbps            = 200;
    streamType      = kJ2KStreamTypeStandard;
    audioChannels   = 2;
    pmtPid          = 255;
    videoPid        = 256;
    pcrPid          = 257;
    audio1Pid       = 258;
}

bool j2kEncoderConfig::operator != ( const j2kEncoderConfig &other )
{
    return (!(*this == other));
}

bool j2kEncoderConfig::operator == ( const j2kEncoderConfig &other )
{
    if ((videoFormat        == other.videoFormat)       &&
        (ullMode            == other.ullMode)           &&
        (bitDepth           == other.bitDepth)          &&
        (chromaSubsamp      == other.chromaSubsamp)     &&
        (mbps               == other.mbps)              &&
        (streamType         == other.streamType)        &&
        (audioChannels      == other.audioChannels)     &&
        (pmtPid             == other.pmtPid)            &&
        (videoPid           == other.videoPid)          &&
        (pcrPid             == other.pcrPid)            &&
        (audio1Pid          == other.audio1Pid))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void j2kDecoderConfig::init()
{
    selectionMode = j2kDecoderConfig::eProgSel_Default;
    programNumber = 1;
    programPID    = 0;
    audioNumber   = 1;
}

bool j2kDecoderConfig::operator != ( const j2kDecoderConfig &other )
{
    return (!(*this == other));
}

bool j2kDecoderConfig::operator == ( const j2kDecoderConfig &other )
{
    if ((selectionMode      == other.selectionMode)     &&
        (programNumber      == other.programNumber)     &&
        (programPID         == other.programPID)        &&
        (audioNumber        == other.audioNumber))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void j2kDecoderStatus::init()
{
    numAvailablePrograms = 0;
    numAvailableAudios = 0;
    availableProgramNumbers.clear();
    availableProgramPIDs.clear();
    availableAudioPIDs.clear();
}

//////////////////////////////////////////////////////////////////////////////////
//
//  CNTV2Config2022
//
//////////////////////////////////////////////////////////////////////////////////

CNTV2Config2022::CNTV2Config2022(CNTV2Card & device) : CNTV2MBController(device)
{
    uint32_t features    = getFeatures();

    _numTx0Chans = (features & (SAREK_TX0_MASK)) >> 28;
    _numRx0Chans = (features & (SAREK_RX0_MASK)) >> 24;
    _numTx1Chans = (features & (SAREK_TX1_MASK)) >> 20;
    _numRx1Chans = (features & (SAREK_RX1_MASK)) >> 16;

    _numRxChans  = _numRx0Chans + _numRx1Chans;
    _numTxChans  = _numTx0Chans + _numTx1Chans;

    _is2022_6   = ((features & SAREK_2022_6)   != 0);
    _is2022_2   = ((features & SAREK_2022_2)   != 0);
    _is2022_7   = ((features & SAREK_2022_7)   != 0);
    _is_txTop34 = ((features & SAREK_TX_TOP34) != 0);

    _biDirectionalChannels = false;

    _tstreamConfig = NULL;
    if (_is2022_2)
    {
        _tstreamConfig = new CNTV2ConfigTs2022(device);
    }

    _isIoIp = (device.GetDeviceID() == DEVICE_ID_IOIP_2022);
}

CNTV2Config2022::~CNTV2Config2022()
{
    if (_is2022_2)
    {
        delete _tstreamConfig;
    }
}

bool CNTV2Config2022::SetNetworkConfiguration(const eSFP sfp, const IPVNetConfig & netConfig)
{
    string ip, subnet, gateway;
    struct in_addr addr;
    addr.s_addr = (uint32_t)netConfig.ipc_ip;
    ip = inet_ntoa(addr);
    addr.s_addr = (uint32_t)netConfig.ipc_subnet;
    subnet = inet_ntoa(addr);
    addr.s_addr = (uint32_t)netConfig.ipc_gateway;
    gateway = inet_ntoa(addr);

    bool rv = SetNetworkConfiguration(sfp, ip, subnet, gateway);
    return rv;
}

bool CNTV2Config2022::GetNetworkConfiguration(const eSFP sfp, IPVNetConfig & netConfig)
{
    string ip, subnet, gateway;
    GetNetworkConfiguration(sfp, ip, subnet, gateway);

    netConfig.ipc_ip      = NTV2EndianSwap32((uint32_t)inet_addr(ip.c_str()));
    netConfig.ipc_subnet  = NTV2EndianSwap32((uint32_t)inet_addr(subnet.c_str()));
    netConfig.ipc_gateway = NTV2EndianSwap32((uint32_t)inet_addr(gateway.c_str()));

    return true;
}

bool CNTV2Config2022::SetNetworkConfiguration(const eSFP sfp, const std::string localIPAddress, const std::string subnetMask, const std::string gateway)
{
    if (!mDevice.IsMBSystemReady())
    {
        mIpErrorCode = NTV2IpErrNotReady;
        return false;
    }

    if (!mDevice.IsMBSystemValid())
    {
        mIpErrorCode = NTV2IpErrSoftwareMismatch;
        return false;
    }

    uint32_t addr = inet_addr(localIPAddress.c_str());
    addr = NTV2EndianSwap32(addr);

    uint32_t macLo;
    uint32_t macHi;

    // get primaray mac address
    uint32_t macAddressRegister = SAREK_REGS + kRegSarekMAC;
    mDevice.ReadRegister(macAddressRegister, macHi);
    macAddressRegister++;
    mDevice.ReadRegister(macAddressRegister, macLo);

    uint32_t boardHi = (macHi & 0xffff0000) >>16;
    uint32_t boardLo = ((macHi & 0x0000ffff) << 16) + ((macLo & 0xffff0000) >> 16);

    // get sfp2 mac address
    macAddressRegister++;
    mDevice.ReadRegister(macAddressRegister, macHi);
    macAddressRegister++;
    mDevice.ReadRegister(macAddressRegister, macLo);

    uint32_t boardHi2 = (macHi & 0xffff0000) >>16;
    uint32_t boardLo2 = ((macHi & 0x0000ffff) << 16) + ((macLo & 0xffff0000) >> 16);

    uint32_t core6;
    uint32_t core2;

    if (sfp == SFP_1)
    {
        core6 = (_is_txTop34) ? SAREK_2022_6_TX_CORE_1 : SAREK_2022_6_TX_CORE_0;
        core2 = SAREK_2022_2_TX_CORE_0;
    }
    else
    {
        core6 = (_is_txTop34) ? SAREK_2022_6_TX_CORE_0 : SAREK_2022_6_TX_CORE_1;
        core2 = SAREK_2022_2_TX_CORE_1;
    }

    if (_is2022_6)
    {
        // initialise constants
        if (_isIoIp)
            mDevice.WriteRegister(kReg2022_6_tx_sys_mem_conf     + core6, 0x10);    // IoIP
        else
            mDevice.WriteRegister(kReg2022_6_tx_sys_mem_conf     + core6, 0x04);    // KonaIP
        mDevice.WriteRegister(kReg2022_6_tx_hitless_config   + core6, 0x01); // disable

        // source ip address
        mDevice.WriteRegister(kReg2022_6_tx_src_ip_addr      + core6,addr);

        mDevice.WriteRegister(kReg2022_6_tx_pri_mac_low_addr + core6,boardLo);
        mDevice.WriteRegister(kReg2022_6_tx_pri_mac_hi_addr  + core6,boardHi);

        mDevice.WriteRegister(kReg2022_6_tx_sec_mac_low_addr + core6,boardLo2);
        mDevice.WriteRegister(kReg2022_6_tx_sec_mac_hi_addr  + core6,boardHi2);
    }
    else
    {
        mDevice.WriteRegister(kReg2022_6_tx_pri_mac_low_addr + core2,boardLo);
        mDevice.WriteRegister(kReg2022_6_tx_pri_mac_hi_addr  + core2,boardHi);

        mDevice.WriteRegister(kReg2022_6_tx_sec_mac_low_addr + core2,boardLo2);
        mDevice.WriteRegister(kReg2022_6_tx_sec_mac_hi_addr  + core2,boardHi2);
    }

    bool rv = SetMBNetworkConfiguration (sfp, localIPAddress, subnetMask, gateway);
    return rv;
}

bool CNTV2Config2022::GetNetworkConfiguration(const eSFP sfp, std::string & localIPAddress, std::string & subnetMask, std::string & gateway)
{
    struct in_addr addr;

    if (sfp == SFP_1)
    {
        uint32_t val;
        mDevice.ReadRegister(SAREK_REGS + kRegSarekIP0, val);
        addr.s_addr = val;
        localIPAddress = inet_ntoa(addr);

        mDevice.ReadRegister(SAREK_REGS + kRegSarekNET0, val);
        addr.s_addr = val;
        subnetMask = inet_ntoa(addr);

        mDevice.ReadRegister(SAREK_REGS + kRegSarekGATE0, val);
        addr.s_addr = val;
        gateway = inet_ntoa(addr);
    }
    else
    {
        uint32_t val;
        mDevice.ReadRegister(SAREK_REGS + kRegSarekIP1, val);
        addr.s_addr = val;
        localIPAddress = inet_ntoa(addr);

        mDevice.ReadRegister(SAREK_REGS + kRegSarekNET1, val);
        addr.s_addr = val;
        subnetMask = inet_ntoa(addr);

        mDevice.ReadRegister(SAREK_REGS + kRegSarekGATE1, val);
        addr.s_addr = val;
        gateway = inet_ntoa(addr);
    }
    return true;
}

bool  CNTV2Config2022::DisableNetworkInterface(const eSFP sfp)
{
    return CNTV2MBController::DisableNetworkInterface(sfp);
}


bool CNTV2Config2022::SetRxChannelConfiguration(const NTV2Channel channel,const rx_2022_channel &rxConfig)
{
    uint32_t    baseAddr;
    bool        rv;

    bool enabled_7  = false;
    uint32_t unused = 0;
    Get2022_7_Mode(enabled_7,unused);

    bool sfp1 =  rxConfig.sfp1Enable;
    bool sfp2 =  rxConfig.sfp2Enable;
    if (enabled_7)
    {
        sfp1 = true;
        sfp2 = true;
    }

    if (sfp1 && (GetSFPActive(SFP_1) == false))
    {
        mIpErrorCode = NTV2IpErrSFP1NotConfigured;
        return false;
    }

    if (sfp2 && (GetSFPActive(SFP_2) == false))
    {
        mIpErrorCode = NTV2IpErrSFP2NotConfigured;
        return false;
    }

    if (_is2022_7)
    {
        // select sfp2 channel
        rv = SelectRxChannel(channel, SFP_2, baseAddr);
        if (!rv) return false;

        // hold off access while we update channel regs
        ChannelSemaphoreClear(kReg2022_6_rx_control, baseAddr);

        SetRxMatch(channel, SFP_2, 0);       // disable while configuring

        // source ip address
        uint32_t sourceIp = inet_addr(rxConfig.sfp2SourceIP.c_str());
        sourceIp = NTV2EndianSwap32(sourceIp);
        WriteChannelRegister(kReg2022_6_rx_match_src_ip_addr + baseAddr, sourceIp);

        // dest ip address
        uint32_t destIp = inet_addr(rxConfig.sfp2DestIP.c_str());
        destIp = NTV2EndianSwap32(destIp);
        WriteChannelRegister(kReg2022_6_rx_match_dest_ip_addr + baseAddr, destIp);

        // source port
        WriteChannelRegister(kReg2022_6_rx_match_src_port + baseAddr, rxConfig.sfp2SourcePort);

        // dest port
        WriteChannelRegister(kReg2022_6_rx_match_dest_port + baseAddr, rxConfig.sfp2DestPort);

        // vlan
        WriteChannelRegister(kReg2022_6_rx_match_vlan + baseAddr, rxConfig.sfp2Vlan);

        // matching
        SetRxMatch(channel, SFP_2, rxConfig.sfp2RxMatch);

        // enable  register updates
        ChannelSemaphoreSet(kReg2022_6_rx_control, baseAddr);

        // update IGMP subscriptions
        uint8_t ip0 = (destIp & 0xff000000)>> 24;
        if ((ip0 >= 224 && ip0 <= 239) && sfp2)
        {
            // is multicast
            bool enabled = false;
            GetRxChannelEnable(channel,enabled);
            if (rxConfig.sfp2RxMatch & RX_MATCH_2022_SOURCE_IP)
                SetIGMPGroup(SFP_2, VideoChannelToStream(channel), destIp, sourceIp, enabled);
            else
                SetIGMPGroup(SFP_2, VideoChannelToStream(channel), destIp, 0, enabled);
        }
        else
        {
            UnsetIGMPGroup(SFP_2, VideoChannelToStream(channel));
        }

         SetRxLinkState(channel, sfp1, sfp2);
    }
    else
    {
        SetRxLinkState(channel, true, false);
    }

    // select sfp1 channel
    rv = SelectRxChannel(channel, SFP_1, baseAddr);
    if (!rv) return false;

    // hold off access while we update channel regs
    ChannelSemaphoreClear(kReg2022_6_rx_control, baseAddr);

    SetRxMatch(channel, SFP_1, 0);      // disable while configuring

    // source ip address
    uint32_t sourceIp = inet_addr(rxConfig.sfp1SourceIP.c_str());
    sourceIp = NTV2EndianSwap32(sourceIp);
    WriteChannelRegister(kReg2022_6_rx_match_src_ip_addr + baseAddr, sourceIp);

    // dest ip address
    uint32_t destIp = inet_addr(rxConfig.sfp1DestIP.c_str());
    destIp = NTV2EndianSwap32(destIp);
    WriteChannelRegister(kReg2022_6_rx_match_dest_ip_addr + baseAddr, destIp);

    // source port
    WriteChannelRegister(kReg2022_6_rx_match_src_port + baseAddr, rxConfig.sfp1SourcePort);

    // dest port
    WriteChannelRegister(kReg2022_6_rx_match_dest_port + baseAddr, rxConfig.sfp1DestPort);

    // ssrc
    WriteChannelRegister(kReg2022_6_rx_match_ssrc + baseAddr, rxConfig.ssrc);

    // vlan
    WriteChannelRegister(kReg2022_6_rx_match_vlan + baseAddr, rxConfig.sfp1Vlan);

    // matching
    SetRxMatch(channel, SFP_1, rxConfig.sfp1RxMatch);

    // playout delay in 27MHz clocks or 90kHz clocks
    uint32_t delay;
    delay = (_is2022_2) ? (rxConfig.playoutDelay * 90) << 9 : rxConfig.playoutDelay * 27000;
    WriteChannelRegister(kReg2022_6_rx_playout_delay + baseAddr, delay);

    // network path differential
    if (_is2022_2 || (enabled_7 == false))
    {
        WriteChannelRegister(kReg2022_6_rx_network_path_differential + baseAddr, 0);
    }

    // some constants
    WriteChannelRegister(kReg2022_6_rx_chan_timeout        + baseAddr, 0x12ffffff);
    WriteChannelRegister(kReg2022_6_rx_media_pkt_buf_size  + baseAddr, 0x0000ffff);
    if (_isIoIp)
        WriteChannelRegister(kReg2022_6_rx_media_buf_base_addr + baseAddr, 0xC0000000 + (0x10000000 * channel));    // IoIp
    else
        WriteChannelRegister(kReg2022_6_rx_media_buf_base_addr + baseAddr, 0x10000000 * channel);                   // KonaIp

    // enable  register updates
    ChannelSemaphoreSet(kReg2022_6_rx_control, baseAddr);

    if (_is2022_2)
    {
        // setup PLL
       mDevice.WriteRegister(kRegPll_Config  + SAREK_PLL, PLL_CONFIG_PCR,PLL_CONFIG_PCR);
       mDevice.WriteRegister(kRegPll_SrcIp   + SAREK_PLL, sourceIp);
       mDevice.WriteRegister(kRegPll_SrcPort + SAREK_PLL, rxConfig.sfp1SourcePort);
       mDevice.WriteRegister(kRegPll_DstIp   + SAREK_PLL, destIp);
       mDevice.WriteRegister(kRegPll_DstPort + SAREK_PLL, rxConfig.sfp1DestPort);

       uint32_t rxMatch  = rxConfig.sfp1RxMatch;
       uint32_t pllMatch = 0;
       if (rxMatch & RX_MATCH_2022_DEST_IP)     pllMatch |= PLL_MATCH_DEST_IP;
       if (rxMatch & RX_MATCH_2022_SOURCE_IP)   pllMatch |= PLL_MATCH_SOURCE_IP;
       if (rxMatch & RX_MATCH_2022_DEST_PORT)   pllMatch |= PLL_MATCH_DEST_PORT;
       if (rxMatch & RX_MATCH_2022_SOURCE_PORT) pllMatch |= PLL_MATCH_SOURCE_PORT;
       pllMatch |= PLL_MATCH_ES_PID;    // always set for TS PCR
       mDevice.WriteRegister(kRegPll_Match   + SAREK_PLL, pllMatch);

    }

    // update IGMP subscriptions
    uint8_t ip0 = (destIp & 0xff000000)>> 24;
    if ((ip0 >= 224 && ip0 <= 239) && sfp1)
    {
        // is multicast
        bool enabled = false;
        GetRxChannelEnable(channel,enabled);
        if (rxConfig.sfp1RxMatch & RX_MATCH_2022_SOURCE_IP)
            SetIGMPGroup(SFP_1, VideoChannelToStream(channel), destIp, sourceIp, enabled);
        else
            SetIGMPGroup(SFP_1, VideoChannelToStream(channel), destIp, 0, enabled);
    }
    else
    {
        UnsetIGMPGroup(SFP_1, VideoChannelToStream(channel));
    }

    return rv;
}

bool  CNTV2Config2022::GetRxChannelConfiguration(const NTV2Channel channel, rx_2022_channel & rxConfig)
{
    uint32_t    baseAddr;
    uint32_t    val;
    bool        rv;

    //get sfp enables
    GetRxLinkState(channel,rxConfig.sfp1Enable, rxConfig.sfp2Enable);

    if (_is2022_7)
    {
        // Select sfp2 channel
        rv = SelectRxChannel(channel, SFP_2, baseAddr);
        if (!rv) return false;

        // source ip address
        ReadChannelRegister(kReg2022_6_rx_match_src_ip_addr + baseAddr, &val);
        struct in_addr in;
        in.s_addr = NTV2EndianSwap32(val);
        char * ip = inet_ntoa(in);
        rxConfig.sfp2SourceIP = ip;

        // dest ip address
        ReadChannelRegister(kReg2022_6_rx_match_dest_ip_addr + baseAddr, &val);
        in.s_addr = NTV2EndianSwap32(val);
        ip = inet_ntoa(in);
        rxConfig.sfp2DestIP = ip;

        // source port
        ReadChannelRegister(kReg2022_6_rx_match_src_port + baseAddr, &rxConfig.sfp2SourcePort);

        // dest port
        ReadChannelRegister(kReg2022_6_rx_match_dest_port + baseAddr, &rxConfig.sfp2DestPort);

        // vlan
        ReadChannelRegister(kReg2022_6_rx_match_vlan + baseAddr, &val);
        rxConfig.sfp2Vlan = val & 0xffff;

        // matching
        GetRxMatch(channel, SFP_2, rxConfig.sfp2RxMatch);
    }
    else
    {
        rxConfig.sfp1Enable = true;
        rxConfig.sfp2Enable = false;
    }

    // select sfp1 channel
    rv = SelectRxChannel(channel, SFP_1, baseAddr);
    if (!rv) return false;

    // source ip address
    ReadChannelRegister(kReg2022_6_rx_match_src_ip_addr + baseAddr, &val);
    struct in_addr in;
    in.s_addr = NTV2EndianSwap32(val);
    char * ip = inet_ntoa(in);
    rxConfig.sfp1SourceIP = ip;

    // dest ip address
    ReadChannelRegister(kReg2022_6_rx_match_dest_ip_addr + baseAddr, &val);
    in.s_addr = NTV2EndianSwap32(val);
    ip = inet_ntoa(in);
    rxConfig.sfp1DestIP = ip;

    // source port
    ReadChannelRegister(kReg2022_6_rx_match_src_port + baseAddr, &rxConfig.sfp1SourcePort);

    // dest port
    ReadChannelRegister(kReg2022_6_rx_match_dest_port + baseAddr, &rxConfig.sfp1DestPort);

    // ssrc
    ReadChannelRegister(kReg2022_6_rx_match_ssrc + baseAddr, &rxConfig.ssrc);

    // vlan
    ReadChannelRegister(kReg2022_6_rx_match_vlan + baseAddr, &val);
    rxConfig.sfp1Vlan = val & 0xffff;

    // matching
    GetRxMatch(channel, SFP_1, rxConfig.sfp1RxMatch);

    // playout delay in ms
    ReadChannelRegister(kReg2022_6_rx_playout_delay + baseAddr,  &val);
    rxConfig.playoutDelay = (_is2022_2) ? (val >>9)/90 : val/27000;

    return true;
}

bool CNTV2Config2022::SetRxChannelEnable(const NTV2Channel channel, bool enable)
{
    uint32_t    baseAddr;
    bool        rv;
    bool        disableIGMP;

    //get sfp enables
    bool sfp1Enable;
    bool sfp2Enable;
    GetRxLinkState(channel,sfp1Enable, sfp2Enable);

    if (enable && sfp1Enable)
    {
        if (GetSFPActive(SFP_1) == false)
        {
            mIpErrorCode = NTV2IpErrSFP1NotConfigured;
            return false;
        }
    }

    if (enable && sfp2Enable)
    {
        if (GetSFPActive(SFP_2) == false)
        {
            mIpErrorCode = NTV2IpErrSFP2NotConfigured;
            return false;
        }
    }

    if (enable && _biDirectionalChannels)
    {
        bool txEnabled;
        GetTxChannelEnable(channel, txEnabled);
        if (txEnabled)
        {
            // disable tx channel
            SetTxChannelEnable(channel,false);
        }
        mDevice.SetSDITransmitEnable(channel, false);
    }

    if (sfp2Enable)
    {
        // IGMP subscription for sfp2 channel
        GetIGMPDisable(SFP_2, disableIGMP);
        if (!disableIGMP)
        {
            EnableIGMPGroup(SFP_2, VideoChannelToStream(channel), enable);
        }
    }
    else
    {
        EnableIGMPGroup(SFP_2, VideoChannelToStream(channel), false);
    }

    if (sfp1Enable)
    {
        // IGMP subscription for sfp1 channel
        GetIGMPDisable(SFP_1, disableIGMP);
        if (!disableIGMP)
        {
            EnableIGMPGroup(SFP_1, VideoChannelToStream(channel), enable);
        }
    }
    else
    {
        EnableIGMPGroup(SFP_1, VideoChannelToStream(channel), false);
    }

    rv = SelectRxChannel(channel, SFP_1, baseAddr);
    if (!rv) return false;
    if (enable & sfp1Enable)
    {
        uint8_t match;
        GetRxMatch(channel, SFP_1, match);
        WriteChannelRegister(kReg2022_6_rx_match_sel + baseAddr, (uint32_t)match);
    }
    else
    {
        // disables rx
        WriteChannelRegister(kReg2022_6_rx_match_sel + baseAddr, 0);
    }

    rv = SelectRxChannel(channel, SFP_2, baseAddr);
    if (!rv) return false;
    if (enable & sfp2Enable)
    {
        uint8_t match;
        GetRxMatch(channel, SFP_2, match);
        WriteChannelRegister(kReg2022_6_rx_match_sel + baseAddr, (uint32_t)match);
    }
    else
    {
        // disables rx
        WriteChannelRegister(kReg2022_6_rx_match_sel + baseAddr, 0);
    }

    // set channel enable
    rv = SelectRxChannel(channel, SFP_1, baseAddr);
    if (enable)
    {
        // Bit 0 = enable
        // Bit 1 = ignore RTP timestamp
        // Bit 1 is set because otherwise, we sometimes lock up on format
        // change on incoming stream
        WriteChannelRegister(kReg2022_6_rx_chan_enable + baseAddr, 0x03);
    }
    else
    {
        WriteChannelRegister(kReg2022_6_rx_chan_enable + baseAddr, 0x00);
    }

    return rv;
}

bool CNTV2Config2022::GetRxChannelEnable(const NTV2Channel channel, bool & enabled)
{
    uint32_t baseAddr;

    enabled = false;

    // select sfp1 channel
    bool rv = SelectRxChannel(channel, SFP_1, baseAddr);
    if (!rv) return false;

    uint32_t val;
    rv = ReadChannelRegister(kReg2022_6_rx_match_sel + baseAddr,&val);
    if (!rv) return false;
    if (val)
    {
        enabled = true;
        return true;
    }

    // select sfp2 channel
    rv = SelectRxChannel(channel, SFP_2, baseAddr);
    if (!rv) return false;

    rv = ReadChannelRegister(kReg2022_6_rx_match_sel + baseAddr,&val);
    if (!rv) return false;
    if (val)
    {
        enabled = true;
        return true;
    }

    return true;
}

static bool MacAddressToRegisters(const MACAddr &macaddr, uint32_t & hi, uint32_t & lo)
{
    hi  = macaddr.mac[0]  << 8;
    hi += macaddr.mac[1];

    lo  = macaddr.mac[2] << 24;
    lo += macaddr.mac[3] << 16;
    lo += macaddr.mac[4] << 8;
    lo += macaddr.mac[5];
	return true;
}

bool CNTV2Config2022::SetTxChannelConfiguration(const NTV2Channel channel, const tx_2022_channel & txConfig)
{
    uint32_t    baseAddr;
    uint32_t    hi;
    uint32_t    lo;


    uint32_t    destIp;

    bool        rv;

    if (txConfig.sfp1Enable && (GetSFPActive(SFP_1) == false))
    {
        mIpErrorCode = NTV2IpErrSFP1NotConfigured;
        return false;
    }

    if (txConfig.sfp2Enable && (GetSFPActive(SFP_2) == false))
    {
        mIpErrorCode = NTV2IpErrSFP2NotConfigured;
        return false;
    }

    if (_is2022_7)
    {
        // Select sfp2 channel
        rv = SelectTxChannel(channel, SFP_2, baseAddr);
        if (!rv) return false;

        // hold off access while we update channel regs
        ChannelSemaphoreClear(kReg2022_6_tx_control, baseAddr);

        // initialise
        uint32_t val = (txConfig.tos << 8) | txConfig.ttl;
        WriteChannelRegister(kReg2022_6_tx_ip_header + baseAddr, val);
        WriteChannelRegister(kReg2022_6_tx_ssrc + baseAddr, txConfig.ssrc);

        if (_is2022_6)
        {
            WriteChannelRegister(kReg2022_6_tx_video_para_config + baseAddr, 0x01);     // include video timestamp
        }
        else
        {
            WriteChannelRegister(kReg2022_2_tx_ts_config + baseAddr,0x0e);
        }

        // dest ip address
        destIp = inet_addr(txConfig.sfp2RemoteIP.c_str());
        destIp = NTV2EndianSwap32(destIp);
        WriteChannelRegister(kReg2022_6_tx_dest_ip_addr + baseAddr,destIp);

        // source port
        WriteChannelRegister(kReg2022_6_tx_udp_src_port + baseAddr,txConfig.sfp2LocalPort);

        // dest port
        WriteChannelRegister(kReg2022_6_tx_udp_dest_port + baseAddr,txConfig.sfp2RemotePort);

        // Get or generate a Mac address if we have 2022-7 enabled.
        if (txConfig.sfp2Enable)
        {
            rv = GetMACAddress(SFP_2, VideoChannelToStream(channel), txConfig.sfp2RemoteIP,hi,lo);
            if (!rv) return false;
            WriteChannelRegister(kReg2022_6_tx_dest_mac_low_addr + baseAddr,lo);
            WriteChannelRegister(kReg2022_6_tx_dest_mac_hi_addr  + baseAddr,hi);
        }

        // enable  register updates
        ChannelSemaphoreSet(kReg2022_6_tx_control, baseAddr);

        SetTxLinkState(channel, txConfig.sfp1Enable,txConfig.sfp2Enable);
    }
    else
    {
        SetTxLinkState(channel, true, false);
    }

    // select sfp1 channel
    rv = SelectTxChannel(channel, SFP_1, baseAddr);
    if (!rv) return false;

    // hold off access while we update channel regs
    ChannelSemaphoreClear(kReg2022_6_tx_control, baseAddr);

    // initialise
    uint32_t val = (txConfig.tos << 8) | txConfig.ttl;
    WriteChannelRegister(kReg2022_6_tx_ip_header + baseAddr, val);
    WriteChannelRegister(kReg2022_6_tx_ssrc + baseAddr, txConfig.ssrc);
    if (_is2022_6)
    {
        WriteChannelRegister(kReg2022_6_tx_video_para_config + baseAddr, 0x01);     // include video timestamp
    }
    else
    {
        WriteChannelRegister(kReg2022_2_tx_ts_config + baseAddr,0x0e);
    }

    // dest ip address
    destIp = inet_addr(txConfig.sfp1RemoteIP.c_str());
    destIp = NTV2EndianSwap32(destIp);
    WriteChannelRegister(kReg2022_6_tx_dest_ip_addr + baseAddr,destIp);

    // source port
    WriteChannelRegister(kReg2022_6_tx_udp_src_port + baseAddr,txConfig.sfp1LocalPort);

    // dest port
    WriteChannelRegister(kReg2022_6_tx_udp_dest_port + baseAddr,txConfig.sfp1RemotePort);

    if (txConfig.sfp1Enable)
    {
        rv = GetMACAddress(SFP_1, VideoChannelToStream(channel), txConfig.sfp1RemoteIP,hi,lo);
        if (!rv) return false;
        WriteChannelRegister(kReg2022_6_tx_dest_mac_low_addr + baseAddr,lo);
        WriteChannelRegister(kReg2022_6_tx_dest_mac_hi_addr  + baseAddr,hi);
    }

    // enable  register updates
    ChannelSemaphoreSet(kReg2022_6_tx_control, baseAddr);

    return rv;
}

bool CNTV2Config2022::GetTxChannelConfiguration(const NTV2Channel channel, tx_2022_channel & txConfig)
{
    uint32_t    baseAddr;
    uint32_t    val;
    bool        rv;
    if (_is2022_7)
    {
        // select sfp2 channel
        rv = SelectTxChannel(channel, SFP_2, baseAddr);
        if (!rv) return false;

        //get sfp enables
        GetTxLinkState(channel,txConfig.sfp1Enable, txConfig.sfp2Enable);

        ReadChannelRegister(kReg2022_6_tx_ip_header + baseAddr,&val);
        txConfig.ttl = val & 0xff;
        txConfig.tos = (val & 0xff00) >> 8;
        ReadChannelRegister(kReg2022_6_tx_ssrc + baseAddr,&txConfig.ssrc);

        // dest ip address
        ReadChannelRegister(kReg2022_6_tx_dest_ip_addr + baseAddr,&val);
        struct in_addr in;
        in.s_addr = NTV2EndianSwap32(val);
        char * ip = inet_ntoa(in);
        txConfig.sfp2RemoteIP = ip;
        // source port
        ReadChannelRegister(kReg2022_6_tx_udp_src_port + baseAddr,&txConfig.sfp2LocalPort);

        // dest port
        ReadChannelRegister(kReg2022_6_tx_udp_dest_port + baseAddr,&txConfig.sfp2RemotePort);
    }
    else
    {
        txConfig.sfp1Enable = true;
        txConfig.sfp2Enable = false;
    }
    // Select sfp1 channel
    rv = SelectTxChannel(channel, SFP_1, baseAddr);
    if (!rv) return false;

    // dest ip address
    ReadChannelRegister(kReg2022_6_tx_dest_ip_addr + baseAddr,&val);
    struct in_addr in;
    in.s_addr = NTV2EndianSwap32(val);
    char * ip = inet_ntoa(in);
    txConfig.sfp1RemoteIP = ip;

    // source port
    ReadChannelRegister(kReg2022_6_tx_udp_src_port + baseAddr,&txConfig.sfp1LocalPort);

    // dest port
    ReadChannelRegister(kReg2022_6_tx_udp_dest_port + baseAddr,&txConfig.sfp1RemotePort);
    return true;
}

bool CNTV2Config2022::SetTxChannelEnable(const NTV2Channel channel, bool enable)
{
    uint32_t    baseAddr;
    bool        rv;
    uint32_t    localIp;

    //get sfp enables
    bool sfp1Enable;
    bool sfp2Enable;
    GetTxLinkState(channel,sfp1Enable, sfp2Enable);

    if (enable && sfp1Enable)
    {
        if (GetSFPActive(SFP_1) == false)
        {
            mIpErrorCode = NTV2IpErrSFP1NotConfigured;
            return false;
        }
    }

    if (enable && sfp2Enable)
    {
        if (GetSFPActive(SFP_2) == false)
        {
            mIpErrorCode = NTV2IpErrSFP2NotConfigured;
            return false;
        }
    }

    if (_biDirectionalChannels)
    {
        bool rxEnabled;
        GetRxChannelEnable(channel,rxEnabled);
        if (rxEnabled)
        {
            // disable rx channel
            SetRxChannelEnable(channel,false);
        }
        mDevice.SetSDITransmitEnable(channel, true);
    }

    // select sfp1 channel
    rv = SelectTxChannel(channel, SFP_1, baseAddr);
    if (!rv) return false;

    if (!enable)
    {
        rv = SelectTxChannel(channel, SFP_1, baseAddr);
        if (!rv) return false;

        WriteChannelRegister(kReg2022_6_tx_chan_enable    + baseAddr,0x0);   // disables channel
    }

    // hold off access while we update channel regs
    ChannelSemaphoreClear(kReg2022_6_tx_control, baseAddr);

    WriteChannelRegister(kReg2022_6_tx_hitless_config   + baseAddr,0x0);  // 0 enables hitless mode

    if (enable && sfp1Enable)
    {
        if (GetTxLink(channel) == SFP_1)
        {
            mDevice.ReadRegister(SAREK_REGS + kRegSarekIP0, localIp);
        }
        else
        {
            mDevice.ReadRegister(SAREK_REGS + kRegSarekIP1, localIp);
        }
        WriteChannelRegister(kReg2022_6_tx_src_ip_addr + baseAddr,NTV2EndianSwap32(localIp));

        WriteChannelRegister(kReg2022_6_tx_link_enable   + baseAddr,0x01);  // enables tx over mac1/mac2
    }
    else
    {
            WriteChannelRegister(kReg2022_6_tx_link_enable   + baseAddr,0x00);  // enables tx over mac1/mac2
    }

    // enable  register updates
    ChannelSemaphoreSet(kReg2022_6_tx_control, baseAddr);

    if (_is2022_7)
    {
        // Select sfp2 channel
        rv = SelectTxChannel(channel, SFP_2, baseAddr);
        if (!rv) return false;

        // hold off access while we update channel regs
        ChannelSemaphoreClear(kReg2022_6_tx_control, baseAddr);

        WriteChannelRegister(kReg2022_6_tx_hitless_config   + baseAddr,0x0);  // 0 enables hitless mode

        if (enable && sfp2Enable)
        {
            mDevice.ReadRegister(SAREK_REGS + kRegSarekIP1, localIp);
            WriteChannelRegister(kReg2022_6_tx_src_ip_addr + baseAddr,NTV2EndianSwap32(localIp));

            // enable
            WriteChannelRegister(kReg2022_6_tx_link_enable   + baseAddr,0x01);  // enables tx over mac1/mac2
        }
        else
        {
            // disable
            WriteChannelRegister(kReg2022_6_tx_link_enable   + baseAddr,0x0);   // disables tx over mac1/mac2
        }

        // enable  register updates
        ChannelSemaphoreSet(kReg2022_6_tx_control, baseAddr);
    }

    if (enable)
    {
        SelectTxChannel(channel, SFP_1, baseAddr);
        WriteChannelRegister(kReg2022_6_tx_chan_enable + baseAddr,0x01);  // enables channel
    }

    return true;
}

bool CNTV2Config2022::GetTxChannelEnable(const NTV2Channel channel, bool & enabled)
{
    uint32_t baseAddr;
    uint32_t val;
    bool rv;

    enabled = false;

    rv = SelectTxChannel(channel, SFP_1, baseAddr);
    if (!rv) return false;

    ReadChannelRegister(kReg2022_6_tx_chan_enable + baseAddr, &val);
    if (val == 0x01)
    {
        enabled = true;
    }

    return true;
}

bool CNTV2Config2022::Set2022_7_Mode(bool enable, uint32_t rx_networkPathDifferential)
{
    if (!mDevice.IsMBSystemReady())
    {
        mIpErrorCode = NTV2IpErrNotReady;
        return false;
    }

    if (!_is2022_7)
    {
		mIpErrorCode = NTV2IpErrNotSupported;
        return false;
    }

    bool old_enable = false;
    uint32_t unused = 0;
    Get2022_7_Mode(old_enable, unused);

    bool enableChange = (old_enable != enable);

    SetDualLinkMode(enable);

    if (_numRxChans)
    {
        uint32_t baseAddr;
        SelectRxChannel(NTV2_CHANNEL1, SFP_1, baseAddr);
        if (enableChange)
        {
            // reset
            WriteChannelRegister(kReg2022_6_rx_reset + baseAddr, 0x01);
            WriteChannelRegister(kReg2022_6_rx_reset + baseAddr, 0x00);
        }
        if (enable)
        {
            uint32_t delay = rx_networkPathDifferential * 27000;
            // network path differential in 27MHz clocks
            WriteChannelRegister(kReg2022_6_rx_network_path_differential + baseAddr, delay);
        }
        else
        {
            WriteChannelRegister(kReg2022_6_rx_network_path_differential + baseAddr, 0);
        }
    }

    if (_numTxChans && enableChange)
    {
        // save
        uint32_t addr;
        mDevice.ReadRegister(kReg2022_6_tx_src_ip_addr + SAREK_2022_6_TX_CORE_0, addr);

        // reset the tx core
        uint32_t baseAddr;
        SelectTxChannel(NTV2_CHANNEL1, SFP_1, baseAddr);
        WriteChannelRegister(kReg2022_6_tx_reset + baseAddr, 0x01);
        WriteChannelRegister(kReg2022_6_tx_reset + baseAddr, 0x00);

        // restore everything
        uint32_t macLo;
        uint32_t macHi;

        // get primaray mac address
        uint32_t macAddressRegister = SAREK_REGS + kRegSarekMAC;
        mDevice.ReadRegister(macAddressRegister, macHi);
        macAddressRegister++;
        mDevice.ReadRegister(macAddressRegister, macLo);

        uint32_t boardHi = (macHi & 0xffff0000) >>16;
        uint32_t boardLo = ((macHi & 0x0000ffff) << 16) + ((macLo & 0xffff0000) >> 16);

        // get sfp2 mac address
        macAddressRegister++;
        mDevice.ReadRegister(macAddressRegister, macHi);
        macAddressRegister++;
        mDevice.ReadRegister(macAddressRegister, macLo);

        uint32_t boardHi2 = (macHi & 0xffff0000) >>16;
        uint32_t boardLo2 = ((macHi & 0x0000ffff) << 16) + ((macLo & 0xffff0000) >> 16);

        // initialise constants
        mDevice.WriteRegister(kReg2022_6_tx_sys_mem_conf     + SAREK_2022_6_TX_CORE_0, 0x04);
        mDevice.WriteRegister(kReg2022_6_tx_hitless_config   + SAREK_2022_6_TX_CORE_0, 0x01); // disable

        // source ip address
        mDevice.WriteRegister(kReg2022_6_tx_src_ip_addr      + SAREK_2022_6_TX_CORE_0,addr);

        mDevice.WriteRegister(kReg2022_6_tx_pri_mac_low_addr + SAREK_2022_6_TX_CORE_0,boardLo);
        mDevice.WriteRegister(kReg2022_6_tx_pri_mac_hi_addr  + SAREK_2022_6_TX_CORE_0,boardHi);

        mDevice.WriteRegister(kReg2022_6_tx_sec_mac_low_addr + SAREK_2022_6_TX_CORE_0,boardLo2);
        mDevice.WriteRegister(kReg2022_6_tx_sec_mac_hi_addr  + SAREK_2022_6_TX_CORE_0,boardHi2);
    }

    return true;
}

bool  CNTV2Config2022::Get2022_7_Mode(bool & enable, uint32_t & rx_networkPathDifferential)
{
    enable = false;
    rx_networkPathDifferential = 0;

    if (!_is2022_7)
    {
		mIpErrorCode = NTV2IpErrNotSupported;
        return false;
    }

    GetDualLinkMode(enable);

    if (_numRxChans)
    {
        // network path differential in ms
        uint32_t val;
        uint32_t baseAddr;
        SelectRxChannel(NTV2_CHANNEL1, SFP_1, baseAddr);
        ReadChannelRegister(kReg2022_6_rx_network_path_differential + baseAddr, &val);
        rx_networkPathDifferential = val/27000;
    }
    return true;
}

bool CNTV2Config2022::SetIGMPDisable(eSFP sfp, bool disable)
{
    uint32_t val = (disable) ? 1 : 0;
    if (sfp == SFP_1)
    {
        mDevice.WriteRegister(SAREK_REGS + kSarekRegIGMPDisable,val);
    }
    else
    {
        mDevice.WriteRegister(SAREK_REGS + kSarekRegIGMPDisable2,val);
    }
    return true;
}

bool CNTV2Config2022::GetIGMPDisable(eSFP sfp, bool & disabled)
{
    uint32_t val;
    if (sfp == SFP_1)
    {
        mDevice.ReadRegister(SAREK_REGS + kSarekRegIGMPDisable, val);
    }
    else
    {
        mDevice.ReadRegister(SAREK_REGS + kSarekRegIGMPDisable2, val);
    }

    disabled = (val == 1) ? true : false;

    return true;
}

bool CNTV2Config2022::SetIGMPVersion(eIGMPVersion_t version)
{
    uint32_t mbversion;
    switch (version)
    {
    case eIGMPVersion_2:
        mbversion = 2;
        break;
    case eIGMPVersion_3:
        mbversion = 3;
        break;
    default:
        mIpErrorCode = NTV2IpErrInvalidIGMPVersion;
        return false;
    }
    return CNTV2MBController::SetIGMPVersion(mbversion);
}

bool CNTV2Config2022::GetIGMPVersion(eIGMPVersion_t & version)
{
    uint32_t version32;
    bool rv = mDevice.ReadRegister(SAREK_REGS + kRegSarekIGMPVersion, version32);
    version =  (version32 == 2) ? eIGMPVersion_2 : eIGMPVersion_3;
    return rv;
}

bool CNTV2Config2022::SetJ2KEncoderConfiguration(const NTV2Channel channel, const j2kEncoderConfig & j2kConfig)
{
    if (_is2022_2)
    {
        CNTV2ConfigTs2022 tsConfig(mDevice);
        bool rv = tsConfig.SetupJ2KEncoder(channel, j2kConfig);
        mIpErrorCode = tsConfig.getLastErrorCode();
        return rv;
    }
    return false;
}

bool CNTV2Config2022::GetJ2KEncoderConfiguration(const NTV2Channel channel, j2kEncoderConfig &j2kConfig)
{
    if (_is2022_2)
    {
        CNTV2ConfigTs2022 tsConfig(mDevice);
        bool rv = tsConfig.ReadbackJ2KEncoder(channel, j2kConfig);
        mIpErrorCode = tsConfig.getLastErrorCode();
        return rv;
    }
    return false;
}

bool CNTV2Config2022::SetJ2KDecoderConfiguration(const  j2kDecoderConfig & j2kConfig)
{
    if (_is2022_2)
    {
    	mDevice.SetAudioSystemInputSource(NTV2_AUDIOSYSTEM_1,NTV2_AUDIO_AES,NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_1);
        CNTV2ConfigTs2022 tsConfig(mDevice);
        bool rv = tsConfig.SetupJ2KDecoder(j2kConfig);
        mIpErrorCode = tsConfig.getLastErrorCode();
        return rv;
    }
    return false;
}

bool CNTV2Config2022::GetJ2KDecoderConfiguration(j2kDecoderConfig & j2kConfig)
{
    if (_is2022_2)
    {
        CNTV2ConfigTs2022 tsConfig(mDevice);
        bool rv = tsConfig.ReadbackJ2KDecoder(j2kConfig);
        mIpErrorCode = tsConfig.getLastErrorCode();
        return rv;
    }
    return false;
}

bool CNTV2Config2022::GetJ2KDecoderStatus(j2kDecoderStatus & j2kStatus)
{
    if (_is2022_2)
    {
        CNTV2ConfigTs2022 tsConfig(mDevice);
        bool rv = tsConfig.GetJ2KDecoderStatus(j2kStatus);
        return rv;
    }
    return false;
}


/////////////////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////////////////

eSFP  CNTV2Config2022::GetTxLink(NTV2Channel chan)
{
    if (!_is_txTop34)
    {
        if ((uint32_t)chan >= _numTx0Chans)
        {
            return SFP_2;
        }
        else
        {
            return SFP_1;
        }
    }
    else
    {
        if ((uint32_t)chan >= _numTx0Chans)
        {
            return SFP_1;
        }
        else
        {
            return SFP_2;
        }
    }
}


bool CNTV2Config2022::SelectRxChannel(NTV2Channel channel, eSFP sfp, uint32_t & baseAddr)
{
    uint32_t iChannel = (uint32_t) channel;
    uint32_t channelIndex = iChannel;

    if (iChannel > _numRxChans)
        return false;

    if (_is2022_6)
    {
        if (iChannel >= _numRx0Chans)
        {
            channelIndex = iChannel - _numRx0Chans;
            baseAddr  = SAREK_2022_6_RX_CORE_1;
        }
        else
        {
            channelIndex = iChannel;
            baseAddr  = SAREK_2022_6_RX_CORE_0;
        }
    }
    else
    {
        if (iChannel >= _numRx0Chans)
        {
            channelIndex = iChannel - _numRx0Chans;
            baseAddr  = SAREK_2022_2_RX_CORE_1;
        }
        else
        {
            channelIndex = iChannel;
            baseAddr  = SAREK_2022_2_RX_CORE_0;
        }
    }

    if (sfp == SFP_2)
        channelIndex |= 0x80000000;

    // select channel
    SetChannel(kReg2022_6_rx_channel_access + baseAddr, channelIndex);

    return true;
}

bool CNTV2Config2022::SelectTxChannel(NTV2Channel channel, eSFP sfp, uint32_t & baseAddr)
{
    uint32_t iChannel = (uint32_t) channel;
    uint32_t channelIndex = iChannel;

    if (iChannel > _numTxChans)
        return false;

    if (_is2022_6)
    {
        if (iChannel >= _numTx0Chans)
        {
            channelIndex = iChannel - _numTx0Chans;
            baseAddr  = SAREK_2022_6_TX_CORE_1;
        }
        else
        {
            channelIndex = iChannel;
            baseAddr  = SAREK_2022_6_TX_CORE_0;
        }
    }
    else
    {
        if (iChannel >= _numTx0Chans)
        {
            channelIndex = iChannel - _numTx0Chans;
            baseAddr  = SAREK_2022_2_TX_CORE_1;
        }
        else
        {
            channelIndex = iChannel;
            baseAddr  = SAREK_2022_2_TX_CORE_0;
        }
    }

    if (sfp == SFP_2)
        channelIndex |= 0x80000000;

    // select channel
    SetChannel(kReg2022_6_tx_channel_access + baseAddr, channelIndex);

    return true;
}

std::string CNTV2Config2022::getLastError()
{
    return NTV2IpErrorEnumToString(getLastErrorCode());
}

NTV2IpError CNTV2Config2022::getLastErrorCode()
{
    NTV2IpError error = mIpErrorCode;
    mIpErrorCode = NTV2IpErrNone;
    return error;
}

void CNTV2Config2022::ChannelSemaphoreSet(uint32_t controlReg, uint32_t baseAddr)
{
    uint32_t val;
    ReadChannelRegister(controlReg + baseAddr, &val);
    WriteChannelRegister(controlReg + baseAddr, val | VOIP_SEMAPHORE_SET);
}

void CNTV2Config2022::ChannelSemaphoreClear(uint32_t controlReg, uint32_t baseAddr)
{
    uint32_t val;
    ReadChannelRegister(controlReg + baseAddr, &val);
    WriteChannelRegister(controlReg + baseAddr, val & VOIP_SEMAPHORE_CLEAR);
}


bool CNTV2Config2022::GetMACAddress(eSFP sfp, NTV2Stream stream, string remoteIP, uint32_t & hi, uint32_t & lo)
{
    uint32_t destIp = inet_addr(remoteIP.c_str());
    destIp = NTV2EndianSwap32(destIp);

    uint32_t    mac;
    MACAddr     macaddr;

     // is remote address muticast?
    uint8_t ip0 = (destIp & 0xff000000)>> 24;
    if (ip0 >= 224 && ip0 <= 239)
    {
        // multicast - generate MAC
        mac = destIp & 0x7fffff;  // lower 23 bits

        macaddr.mac[0] = 0x01;
        macaddr.mac[1] = 0x00;
        macaddr.mac[2] = 0x5e;
        macaddr.mac[3] =  mac >> 16;
        macaddr.mac[4] = (mac & 0xffff) >> 8;
        macaddr.mac[5] =  mac & 0xff;
    }
    else
    {
        // unicast - get MAC from ARP
        string macAddr;
        bool rv;
        // is destination on the same subnet?
        IPVNetConfig nc;
        GetNetworkConfiguration(sfp, nc);
        if ( (destIp & nc.ipc_subnet) != (nc.ipc_ip & nc.ipc_subnet))
        {
            struct in_addr addr;
            addr.s_addr  = NTV2EndianSwap32(nc.ipc_gateway);
            string gateIp = inet_ntoa(addr);
            rv = GetRemoteMAC(gateIp, sfp, stream, macAddr);
        }
        else
        {
            rv = GetRemoteMAC(remoteIP, sfp, stream, macAddr);
        }
        if (!rv)
        {
            SetTxChannelEnable(VideoStreamToChannel(stream), false); // stop transmit
            mIpErrorCode = NTV2IpErrCannotGetMacAddress;
            return false;
        }

        istringstream ss(macAddr);
        string token;
        int i=0;
        while (i < 6)
        {
            getline (ss, token, ':');
            macaddr.mac[i++] = (uint8_t)strtoul(token.c_str(),NULL,16);
        }
    }

	return MacAddressToRegisters( macaddr, hi, lo );
}

bool CNTV2Config2022::GetSFPMSAData(eSFP sfp, SFPMSAData & data)
{
    return GetSFPInfo(sfp, data);
}

bool CNTV2Config2022::GetLinkStatus(eSFP sfp, SFPStatus & sfpStatus)
{
    uint32_t val;
    mDevice.ReadRegister(SAREK_REGS + kRegSarekLinkStatus, val);
    uint32_t val2;
    mDevice.ReadRegister(SAREK_REGS + kRegSarekSFPStatus, val2);

    if (sfp == SFP_2)
    {
        sfpStatus.SFP_linkUp        = (val  & LINK_B_UP) ? true : false;
        sfpStatus.SFP_present       = (val2 & SFP_2_NOT_PRESENT) ? false : true;
        sfpStatus.SFP_rxLoss        = (val2 & SFP_2_RX_LOS) ? true : false;
        sfpStatus.SFP_txFault       = (val2 & SFP_2_TX_FAULT) ? true : false;
    }
    else
    {
        sfpStatus.SFP_linkUp        = (val  & LINK_A_UP) ? true : false;
        sfpStatus.SFP_present       = (val2 & SFP_1_NOT_PRESENT) ? false : true;
        sfpStatus.SFP_rxLoss        = (val2 & SFP_1_RX_LOS) ? true : false;
        sfpStatus.SFP_txFault       = (val2 & SFP_1_TX_FAULT) ? true : false;
    }

    return true;
}

bool CNTV2Config2022::Get2022ChannelRxStatus(NTV2Channel channel, s2022RxChannelStatus & chanStatus)
{
    uint32_t addr;

    SelectRxChannel(channel, SFP_2, addr);
    ReadChannelRegister(addr + kReg2022_6_rx_sec_recv_pkt_cnt, &chanStatus.sfp2RxPackets);
    ReadChannelRegister(addr + kReg2022_6_rx_link_valid_media_pkt_cnt, &chanStatus.sfp2ValidRxPackets);

    SelectRxChannel(channel, SFP_1, addr);
    ReadChannelRegister(addr + kReg2022_6_rx_pri_recv_pkt_cnt, &chanStatus.sfp1RxPackets);
    ReadChannelRegister(addr + kReg2022_6_rx_link_valid_media_pkt_cnt, &chanStatus.sfp1ValidRxPackets);

    return true;
}

bool CNTV2Config2022::SetIPServicesControl(const bool enable, const bool forceReconfig)
{
    uint32_t val = 0;
    if (enable)
        val |= BIT(0);

    if (forceReconfig)
        val |= BIT(0);

    mDevice.WriteRegister(SAREK_REGS + kRegSarekServices, val);

    return true;
}

bool CNTV2Config2022::GetIPServicesControl(bool & enable, bool & forceReconfig)
{
    uint32_t val;
    mDevice.ReadRegister(SAREK_REGS + kRegSarekServices, val);

    if (val & BIT(0))
        enable = true;
    else
        enable = false;

    if (val & BIT(1))
        forceReconfig = true;
    else
        forceReconfig = false;

    return true;
}

NTV2Stream CNTV2Config2022::VideoChannelToStream(const NTV2Channel channel)
{
    NTV2Stream stream;
    switch (channel)
    {
        case NTV2_CHANNEL1:     stream = NTV2_VIDEO1_STREAM;    break;
        case NTV2_CHANNEL2:     stream = NTV2_VIDEO2_STREAM;    break;
        case NTV2_CHANNEL3:     stream = NTV2_VIDEO3_STREAM;    break;
        case NTV2_CHANNEL4:     stream = NTV2_VIDEO4_STREAM;    break;
        default:                stream = NTV2_STREAM_INVALID;   break;
    }
    return stream;
}

NTV2Channel CNTV2Config2022::VideoStreamToChannel(const NTV2Stream stream)
{
    NTV2Channel channel;
    switch (stream)
    {
        case NTV2_VIDEO1_STREAM:    channel = NTV2_CHANNEL1;            break;
        case NTV2_VIDEO2_STREAM:    channel = NTV2_CHANNEL2;            break;
        case NTV2_VIDEO3_STREAM:    channel = NTV2_CHANNEL3;            break;
        case NTV2_VIDEO4_STREAM:    channel = NTV2_CHANNEL4;            break;
        default:                    channel = NTV2_CHANNEL_INVALID;     break;
    }
    return channel;
}
