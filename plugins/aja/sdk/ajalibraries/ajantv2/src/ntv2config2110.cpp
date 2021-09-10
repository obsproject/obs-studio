/* SPDX-License-Identifier: MIT */
/**
    @file       ntv2config2110.cpp
    @brief      Implements the CNTV2Config2110 class.
    @copyright  (C) 2014-2021 AJA Video Systems, Inc.   
**/

#include "ntv2config2110.h"
#include "ntv2endian.h"
#include "ntv2card.h"
#include "ntv2utils.h"
#include <sstream>
#include <algorithm>

#if defined (AJALinux) || defined (AJAMac)
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#endif

#define RESET_MILLISECONDS 5

uint32_t CNTV2Config2110::videoPacketizers[4]		= {SAREK_4175_TX_PACKETIZER_1,
													   SAREK_4175_TX_PACKETIZER_2,
													   SAREK_4175_TX_PACKETIZER_3,
													   SAREK_4175_TX_PACKETIZER_4};

uint32_t CNTV2Config2110::videoRGB12Packetizers[4]	= {SAREK_4175_TX_PACKETIZER_RGB12_1,
													   SAREK_4175_TX_PACKETIZER_RGB12_2,
													   SAREK_4175_TX_PACKETIZER_RGB12_3,
													   SAREK_4175_TX_PACKETIZER_RGB12_4};

uint32_t CNTV2Config2110::audioPacketizers[4]		= {SAREK_3190_TX_PACKETIZER_0,
													   SAREK_3190_TX_PACKETIZER_1,
													   SAREK_3190_TX_PACKETIZER_2,
													   SAREK_3190_TX_PACKETIZER_3};

uint32_t CNTV2Config2110::videoDepacketizers[4]		= {SAREK_4175_RX_DEPACKETIZER_1,
													   SAREK_4175_RX_DEPACKETIZER_2,
													   SAREK_4175_RX_DEPACKETIZER_3,
													   SAREK_4175_RX_DEPACKETIZER_4};

uint32_t CNTV2Config2110::audioDepacketizers[4]		= {SAREK_3190_RX_DEPACKETIZER_1,
													   SAREK_3190_RX_DEPACKETIZER_2,
													   SAREK_3190_RX_DEPACKETIZER_3,
													   SAREK_3190_RX_DEPACKETIZER_4};

using namespace std;

void tx_2110Config::init()
{
    for (int i=0; i < 2; i++)
    {
        localPort[i]   = 0;
        remotePort[i]  = 0;
        remoteIP[i].erase();
    }
    videoFormat         = NTV2_FORMAT_UNKNOWN;
    videoSamples        = VPIDSampling_YUV_422;
    ttl                 = 0x40;
    tos                 = 0x64;
    ssrc                = 1000;
    numAudioChannels    = 0;
    firstAudioChannel   = 0;
    audioPktInterval    = PACKET_INTERVAL_1mS;
    channel             = (NTV2Channel)0;
}

bool tx_2110Config::operator != ( const tx_2110Config &other )
{
    return !(*this == other);
}

bool tx_2110Config::operator == ( const tx_2110Config &other )
{
    if ((localPort              == other.localPort)             &&
        (remotePort             == other.remotePort)            &&
        (remoteIP               == other.remoteIP)              &&
        (videoFormat            == other.videoFormat)           &&
        (videoSamples           == other.videoSamples)          &&
        (numAudioChannels       == other.numAudioChannels)      &&
        (firstAudioChannel      == other.firstAudioChannel)     &&
        (audioPktInterval       == other.audioPktInterval))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void rx_2110Config::init()
{
    rxMatch             = 0;
    sourceIP.erase();
    destIP.erase();
    sourcePort          = 0;
    destPort            = 0;
    ssrc                = 1000;
    vlan                = 1;
    payloadType         = 0;
    videoFormat         = NTV2_FORMAT_UNKNOWN;
    videoSamples        = VPIDSampling_YUV_422;
    numAudioChannels    = 2;
    audioPktInterval    = PACKET_INTERVAL_1mS;
}

bool rx_2110Config::operator != ( const rx_2110Config &other )
{
    return (!(*this == other));
}

bool rx_2110Config::operator == ( const rx_2110Config &other )
{
    if (    (rxMatch           == other.rxMatch)            &&
            (sourceIP          == other.sourceIP)           &&
            (destIP            == other.destIP)             &&
            (sourcePort        == other.sourcePort)         &&
            (destPort          == other.destPort)           &&
            (ssrc              == other.ssrc)               &&
            (vlan              == other.vlan)               &&
            (videoFormat       == other.videoFormat)        &&
            (videoSamples      == other.videoSamples)       &&
            (numAudioChannels  == other.numAudioChannels)    &&
            (audioPktInterval  == other.audioPktInterval))
    {
        return true;
    }
    else
    {
        return false;
    }
}

//////////////////////////////////////////////////////////////////////////////////
//
//  CNTV2Config2110
//
//////////////////////////////////////////////////////////////////////////////////

CNTV2Config2110::CNTV2Config2110(CNTV2Card & device) : CNTV2MBController(device)
{
    uint32_t features    = getFeatures();

    _numTx0Chans = (features & (SAREK_TX0_MASK)) >> 28;
    _numRx0Chans = (features & (SAREK_RX0_MASK)) >> 24;
    _numTx1Chans = (features & (SAREK_TX1_MASK)) >> 20;
    _numRx1Chans = (features & (SAREK_RX1_MASK)) >> 16;

    _numRxChans  = _numRx0Chans + _numRx1Chans;
    _numTxChans  = _numTx0Chans + _numTx1Chans;

    _biDirectionalChannels = false;
}

CNTV2Config2110::~CNTV2Config2110()
{
}

bool CNTV2Config2110::SetNetworkConfiguration(const eSFP sfp, const IPVNetConfig & netConfig)
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

bool CNTV2Config2110::GetNetworkConfiguration(const eSFP sfp, IPVNetConfig & netConfig)
{
    string ip, subnet, gateway;
    GetNetworkConfiguration(sfp, ip, subnet, gateway);

    netConfig.ipc_ip      = NTV2EndianSwap32((uint32_t)inet_addr(ip.c_str()));
    netConfig.ipc_subnet  = NTV2EndianSwap32((uint32_t)inet_addr(subnet.c_str()));
    netConfig.ipc_gateway = NTV2EndianSwap32((uint32_t)inet_addr(gateway.c_str()));

    return true;
}

bool CNTV2Config2110::SetNetworkConfiguration(const eSFP sfp, const std::string localIPAddress, const std::string subnetMask, const std::string gateway)
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

    // get sfp1 mac address
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


    uint32_t core;
    core = SAREK_2110_VIDEO_FRAMER_0;
    mDevice.WriteRegister(kRegFramer_src_mac_lo + core,boardLo);
    mDevice.WriteRegister(kRegFramer_src_mac_hi + core,boardHi);
	core = SAREK_2110_AUDIO_ANC_FRAMER_0;
    mDevice.WriteRegister(kRegFramer_src_mac_lo + core,boardLo);
    mDevice.WriteRegister(kRegFramer_src_mac_hi + core,boardHi);

    core = SAREK_2110_VIDEO_FRAMER_1;
    mDevice.WriteRegister(kRegFramer_src_mac_lo + core,boardLo2);
    mDevice.WriteRegister(kRegFramer_src_mac_hi + core,boardHi2);
	core = SAREK_2110_AUDIO_ANC_FRAMER_1;
    mDevice.WriteRegister(kRegFramer_src_mac_lo + core,boardLo2);
    mDevice.WriteRegister(kRegFramer_src_mac_hi + core,boardHi2);

    bool rv = SetMBNetworkConfiguration (sfp, localIPAddress, subnetMask, gateway);
    if (!rv) return false;

    if (sfp == SFP_1)
    {
        ConfigurePTP(sfp,localIPAddress);
    }

    return true;
}

bool CNTV2Config2110::GetNetworkConfiguration(const eSFP sfp, std::string & localIPAddress, std::string & subnetMask, std::string & gateway)
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

bool  CNTV2Config2110::DisableNetworkInterface(const eSFP sfp)
{
    return CNTV2MBController::DisableNetworkInterface(sfp);
}

bool CNTV2Config2110::SetRxStreamConfiguration(const eSFP sfp, const NTV2Stream stream, const rx_2110Config & rxConfig)
{
	if ((mDevice.GetDeviceID() == DEVICE_ID_KONAIP_2110_RGB12) ||
		(mDevice.GetDeviceID() == DEVICE_ID_IOIP_2110_RGB12))
	{
		mIpErrorCode = NTV2IpErrNotSupported;
		return false;
	}

	if ((StreamType(stream) == VIDEO_STREAM) ||
		(StreamType(stream) == AUDIO_STREAM) ||
		(StreamType(stream) == ANC_STREAM))
	{
    	if (GetSFPActive(sfp) == false)
    	{
        	mIpErrorCode = NTV2IpErrSFP1NotConfigured;
        	return false;
    	}

    	SetRxStreamEnable(sfp, stream, false);

    	ResetDepacketizerStream(stream);
    	SetupDepacketizerStream(stream,rxConfig);
    	SetupDecapsulatorStream(sfp, stream, rxConfig);
		return true;
	}
	else
	{
		mIpErrorCode = NTV2IpErrInvalidChannel;
		return false;
	}
}

void  CNTV2Config2110::SetupDecapsulatorStream(const eSFP sfp, const NTV2Stream stream, const rx_2110Config & rxConfig)
{
    uint32_t  decapBaseAddr = GetDecapsulatorAddress(sfp, stream);

    // source ip address
    uint32_t sourceIp = inet_addr(rxConfig.sourceIP.c_str());
    sourceIp = NTV2EndianSwap32(sourceIp);
    mDevice.WriteRegister(kRegDecap_match_src_ip + decapBaseAddr, sourceIp);

    // dest ip address
    uint32_t destIp = inet_addr(rxConfig.destIP.c_str());
    destIp = NTV2EndianSwap32(destIp);
    mDevice.WriteRegister(kRegDecap_match_dst_ip + decapBaseAddr, destIp);

    // source port
    mDevice.WriteRegister(kRegDecap_match_udp_src_port + decapBaseAddr, rxConfig.sourcePort);

    // dest port
    mDevice.WriteRegister(kRegDecap_match_udp_dst_port + decapBaseAddr, rxConfig.destPort);

    // ssrc
    mDevice.WriteRegister(kRegDecap_match_ssrc + decapBaseAddr, rxConfig.ssrc);

    // vlan
    //WriteRegister(kRegDecap_match_vlan + decapBaseAddr, rxConfig.VLAN);

    // payloadType
    mDevice. WriteRegister(kRegDecap_match_payload + decapBaseAddr, rxConfig.payloadType);

    // matching
    mDevice.WriteRegister(kRegDecap_match_sel + decapBaseAddr, rxConfig.rxMatch);
}

void  CNTV2Config2110::DisableDepacketizerStream(NTV2Stream stream)
{
    uint32_t  depacketizerBaseAddr = GetDepacketizerAddress(stream);

    if (StreamType(stream) == VIDEO_STREAM)
    {
        mDevice.WriteRegister(kReg4175_depkt_control + depacketizerBaseAddr, 0x00);
    }
    else if (StreamType(stream) == AUDIO_STREAM)
    {
        mDevice.WriteRegister(kReg3190_depkt_enable + depacketizerBaseAddr, 0x00);
    }
}

void  CNTV2Config2110::EnableDepacketizerStream(const NTV2Stream stream)
{
    uint32_t  depacketizerBaseAddr = GetDepacketizerAddress(stream);

    if (StreamType(stream) == VIDEO_STREAM)
    {
        mDevice.WriteRegister(kReg4175_depkt_control + depacketizerBaseAddr, 0x01);
    }
    else if (StreamType(stream) == AUDIO_STREAM)
    {
        mDevice.WriteRegister(kReg3190_depkt_enable + depacketizerBaseAddr, 0x01);
    }
}

void  CNTV2Config2110::DisableDecapsulatorStream(const eSFP sfp, const NTV2Stream stream)
{
    // disable decasulator
    uint32_t  decapBaseAddr = GetDecapsulatorAddress(sfp, stream);
    mDevice.WriteRegister(kRegDecap_chan_enable + decapBaseAddr, 0x00);
}

void  CNTV2Config2110::EnableDecapsulatorStream(const eSFP sfp, const NTV2Stream stream)
{
    // enable decasulator
    uint32_t  decapBaseAddr = GetDecapsulatorAddress(sfp, stream);
    mDevice.WriteRegister(kRegDecap_chan_enable + decapBaseAddr, 0x01);
}

void  CNTV2Config2110::ResetPacketizerStream(const NTV2Stream stream)
{
    if (StreamType(stream) == VIDEO_STREAM)
    {
        uint32_t    val;
        uint32_t    bit;

        switch(stream)
        {
            default:
            case NTV2_VIDEO1_STREAM:
                bit = BIT(16);
                break;
            case NTV2_VIDEO2_STREAM:
                bit = BIT(17);
                break;
            case NTV2_VIDEO3_STREAM:
                bit = BIT(18);
                break;
            case NTV2_VIDEO4_STREAM:
                bit = BIT(19);
                break;
        }

        // read/modify/write reset bit for a given channel
        mDevice.ReadRegister(kRegSarekRxReset + SAREK_REGS, val);

        // Set reset bit
        val |= bit;
        mDevice.WriteRegister(kRegSarekRxReset + SAREK_REGS, val);

        // Unset reset bit
        val &= ~bit;
        mDevice.WriteRegister(kRegSarekRxReset + SAREK_REGS, val);

		// Wait just a bit
		#if defined(AJAWindows) || defined(MSWindows)
			::Sleep (RESET_MILLISECONDS);
		#else
			usleep (RESET_MILLISECONDS * 1000);
		#endif
    }
}

void  CNTV2Config2110::ResetDepacketizerStream(const NTV2Stream stream)
{
    if (StreamType(stream) == VIDEO_STREAM)
    {
        uint32_t    val;
        uint32_t    bit;

        switch(stream)
        {
        default:
        case NTV2_VIDEO1_STREAM:
            bit = BIT(0);
            break;
        case NTV2_VIDEO2_STREAM:
            bit = BIT(1);
            break;
        case NTV2_VIDEO3_STREAM:
            bit = BIT(2);
            break;
        case NTV2_VIDEO4_STREAM:
            bit = BIT(3);
            break;
        }

        // read/modify/write reset bit for a given channel
        mDevice.ReadRegister(kRegSarekRxReset + SAREK_REGS, val);

        // Set reset bit
        val |= bit;
        mDevice.WriteRegister(kRegSarekRxReset + SAREK_REGS, val);

        // Unset reset bit
        val &= ~bit;
        mDevice.WriteRegister(kRegSarekRxReset + SAREK_REGS, val);

		// Wait just a bit
		#if defined(AJAWindows) || defined(MSWindows)
			::Sleep (RESET_MILLISECONDS);
		#else
			usleep (RESET_MILLISECONDS * 1000);
		#endif
    }
}

void  CNTV2Config2110::SetupDepacketizerStream(const NTV2Stream stream, const rx_2110Config & rxConfig)
{
    if (StreamType(stream) == VIDEO_STREAM)
    {
        SetVideoFormatForRxTx(stream, (NTV2VideoFormat) rxConfig.videoFormat, true);
    }
	else if (StreamType(stream) == AUDIO_STREAM)
    {
        // setup 3190 depacketizer
        uint32_t  depacketizerBaseAddr = GetDepacketizerAddress(stream);

        mDevice.WriteRegister(kReg3190_depkt_enable + depacketizerBaseAddr, 0x00);
        uint32_t num_samples = (rxConfig.audioPktInterval == PACKET_INTERVAL_125uS) ? 6 : 48;
		uint32_t num_channels = rxConfig.numAudioChannels;
        uint32_t val = (num_samples << 8) + num_channels;
        mDevice.WriteRegister(kReg3190_depkt_config + depacketizerBaseAddr,val);

        // audio channels
        mDevice.WriteRegister(kReg3190_depkt_enable + depacketizerBaseAddr,0x01);
    }
}

void CNTV2Config2110::SetVideoFormatForRxTx(const NTV2Stream stream, const NTV2VideoFormat format, const bool rx)
{
	if (StreamType(stream) == VIDEO_STREAM)
	{
		NTV2FormatDescriptor fd(format, NTV2_FBF_10BIT_YCBCR);

		NTV2FrameRate       fr  = GetNTV2FrameRateFromVideoFormat(format);
		NTV2FrameGeometry   fg  = GetNTV2FrameGeometryFromVideoFormat(format);
		NTV2Standard        std = fd.GetVideoStandard();
		bool               is2K = fd.Is2KFormat();

		uint32_t val = ( (((uint32_t) fr) << 8) |
						 (((uint32_t) fg) << 4) |
						  ((uint32_t) std ) );

		if (is2K)
			val += BIT(13);

		if (NTV2_IS_PSF_VIDEO_FORMAT (format))
			val += BIT(15);

		uint32_t reg, reg2;
		if (rx)
		{
			reg = stream - NTV2_CHANNEL1 + kRegRxVideoDecode1;
			reg2 = stream - NTV2_CHANNEL1 + kRegRxNtv2VideoDecode1;
		}
		else
		{
			reg = stream - NTV2_CHANNEL1 + kRegTxVideoDecode1;
			reg2 = stream - NTV2_CHANNEL1 + kRegTxNtv2VideoDecode1;
		}

		// Write the format for the firmware and also tuck it away in NTV2 flavor so we can retrieve it easily
		mDevice.WriteRegister(reg + SAREK_2110_TX_ARBITRATOR, val);
		mDevice.WriteRegister(reg2 + SAREK_REGS2, format);
	}
}

void CNTV2Config2110::GetVideoFormatForRxTx(const NTV2Stream stream, NTV2VideoFormat & format, uint32_t & hwFormat, const bool rx)
{
	if (StreamType(stream) == VIDEO_STREAM)
	{
		uint32_t reg, reg2, value;
		if (rx)
		{
			reg = stream - NTV2_CHANNEL1 + kRegRxVideoDecode1;
			reg2 = stream - NTV2_CHANNEL1 + kRegRxNtv2VideoDecode1;
		}
		else
		{
			reg = stream - NTV2_CHANNEL1 + kRegTxVideoDecode1;
			reg2 = stream - NTV2_CHANNEL1 + kRegTxNtv2VideoDecode1;
		}

		// Write the format for the firmware and also tuck it away in NTV2 flavor so we can retrieve it easily
		mDevice.ReadRegister(reg + SAREK_2110_TX_ARBITRATOR, value);
		hwFormat = value;
		mDevice.ReadRegister(reg2 + SAREK_REGS2, value);
		format = (NTV2VideoFormat)value;
	}
}

bool  CNTV2Config2110::GetRxStreamConfiguration(const eSFP sfp, const NTV2Stream stream, rx_2110Config & rxConfig)
{
	if ((mDevice.GetDeviceID() == DEVICE_ID_KONAIP_2110_RGB12) ||
		(mDevice.GetDeviceID() == DEVICE_ID_IOIP_2110_RGB12))
	{
		mIpErrorCode = NTV2IpErrNotSupported;
		return false;
	}

	if ((StreamType(stream) == VIDEO_STREAM) ||
		(StreamType(stream) == AUDIO_STREAM) ||
		(StreamType(stream) == ANC_STREAM))
	{
		uint32_t    val;
		uint32_t	depacketizerBaseAddr;

		// get address,strean
		uint32_t  decapBaseAddr = GetDecapsulatorAddress(sfp, stream);

		// source ip address
		mDevice.ReadRegister(kRegDecap_match_src_ip + decapBaseAddr, val);
		struct in_addr in;
		in.s_addr = NTV2EndianSwap32(val);
		char * ip = inet_ntoa(in);
		rxConfig.sourceIP = ip;

		// dest ip address
		mDevice.ReadRegister(kRegDecap_match_dst_ip + decapBaseAddr, val);
		in.s_addr = NTV2EndianSwap32(val);
		ip = inet_ntoa(in);
		rxConfig.destIP = ip;

		// source port
		mDevice.ReadRegister(kRegDecap_match_udp_src_port + decapBaseAddr, rxConfig.sourcePort);

		// dest port
		mDevice.ReadRegister(kRegDecap_match_udp_dst_port + decapBaseAddr, rxConfig.destPort);

		// ssrc
		mDevice.ReadRegister(kRegDecap_match_ssrc + decapBaseAddr, rxConfig.ssrc);

		// vlan
		//mDevice.ReadRegister(kRegDecap_match_vlan + decapBaseAddr, val);
		//rxConfig.VLAN = val & 0xffff;

		// payloadType
		mDevice.ReadRegister(kRegDecap_match_payload + decapBaseAddr, val);
		rxConfig.payloadType = val & 0x7f;

		// matching
		mDevice.ReadRegister(kRegDecap_match_sel + decapBaseAddr, rxConfig.rxMatch);

		if (StreamType(stream) == VIDEO_STREAM)
		{
			NTV2VideoFormat     format;
			uint32_t            hwFormat;

			depacketizerBaseAddr = GetDepacketizerAddress(stream);
			GetVideoFormatForRxTx(stream, format, hwFormat, true);
			rxConfig.videoFormat = format;
		}
		else if (StreamType(stream) == AUDIO_STREAM)
		{
			uint32_t samples;

			depacketizerBaseAddr = GetDepacketizerAddress(stream);
			mDevice.ReadRegister(kReg3190_depkt_config + depacketizerBaseAddr, samples);
			rxConfig.audioPktInterval = (((samples >> 8) & 0xff) == 6) ? PACKET_INTERVAL_125uS : PACKET_INTERVAL_1mS;
			rxConfig.numAudioChannels = samples & 0xff;
		}
		return true;
	}
	else
	{
		mIpErrorCode = NTV2IpErrInvalidChannel;
		return false;
	}
}

bool CNTV2Config2110::SetRxStreamEnable(const eSFP sfp, const NTV2Stream stream, bool enable)
{
	if ((mDevice.GetDeviceID() == DEVICE_ID_KONAIP_2110_RGB12) ||
		(mDevice.GetDeviceID() == DEVICE_ID_IOIP_2110_RGB12))
	{
		mIpErrorCode = NTV2IpErrNotSupported;
		return false;
	}

	if ((StreamType(stream) == VIDEO_STREAM) ||
		(StreamType(stream) == AUDIO_STREAM) ||
		(StreamType(stream) == ANC_STREAM))
	{
		if (GetSFPActive(sfp) == false)
		{
			mIpErrorCode = NTV2IpErrSFP1NotConfigured;
			return false;
		}

		if (enable)
		{
			uint32_t  decapBaseAddr = GetDecapsulatorAddress(sfp, stream);

			// get source and dest ip address
			uint32_t srcIp, destIp;
			mDevice.ReadRegister(kRegDecap_match_src_ip + decapBaseAddr, srcIp);
			mDevice.ReadRegister(kRegDecap_match_dst_ip + decapBaseAddr, destIp);

			// setup IGMP subscription
			uint8_t ip0 = (destIp & 0xff000000)>> 24;
			if (ip0 >= 224 && ip0 <= 239)
			{
				// is multicast
				SetIGMPGroup(sfp, stream, destIp, srcIp, true);
			}
			else
			{
				UnsetIGMPGroup(sfp, stream);
			}

			EnableDepacketizerStream(stream);
			EnableDecapsulatorStream(sfp, stream);
		}
		else
		{
			// disable IGMP subscription
			bool disableIGMP;
			GetIGMPDisable(sfp, disableIGMP);
			if (!disableIGMP)
			{
				EnableIGMPGroup(sfp, stream, false);
			}

			DisableDecapsulatorStream(sfp, stream);
			DisableDepacketizerStream(stream);
		}
		return true;
	}
	else
	{
		mIpErrorCode = NTV2IpErrInvalidChannel;
		return false;
	}
}

bool CNTV2Config2110::GetRxStreamEnable(const eSFP sfp, const NTV2Stream stream, bool & enabled)
{
	if ((mDevice.GetDeviceID() == DEVICE_ID_KONAIP_2110_RGB12) ||
		(mDevice.GetDeviceID() == DEVICE_ID_IOIP_2110_RGB12))
	{
		mIpErrorCode = NTV2IpErrNotSupported;
		return false;
	}

	enabled = false;

	if ((StreamType(stream) == VIDEO_STREAM) ||
		(StreamType(stream) == AUDIO_STREAM) ||
		(StreamType(stream) == ANC_STREAM))
	{
		// get address
		uint32_t  decapBaseAddr = GetDecapsulatorAddress(sfp, stream);

		uint32_t val;
		mDevice.ReadRegister(kRegDecap_chan_enable + decapBaseAddr, val);
		enabled = (val & 0x01);
		return true;
	}
	else
	{
		mIpErrorCode = NTV2IpErrInvalidChannel;
		return false;
	}
}

bool CNTV2Config2110::GetRxPacketCount(const NTV2Stream stream, uint32_t & packets)
{
	uint32_t	depacketizerBaseAddr;

    if (StreamType(stream) == VIDEO_STREAM)
    {
		depacketizerBaseAddr = GetDepacketizerAddress(stream);
        mDevice.ReadRegister(kReg4175_depkt_rx_pkt_cnt+ depacketizerBaseAddr, packets);
    }
    else if (StreamType(stream) == AUDIO_STREAM)
    {
		depacketizerBaseAddr = GetDepacketizerAddress(stream);
		mDevice.ReadRegister(kReg3190_depkt_rx_pkt_count + depacketizerBaseAddr, packets);
    }
    else
    {
        packets = 0;
    }

    return true;
}

bool CNTV2Config2110::GetRxByteCount(const NTV2Stream stream, uint32_t & bytes)
{
	uint32_t	depacketizerBaseAddr;

    if (StreamType(stream) == VIDEO_STREAM)
    {
		depacketizerBaseAddr = GetDepacketizerAddress(stream);
        mDevice.ReadRegister(kReg4175_depkt_rx_byte_cnt+ depacketizerBaseAddr, bytes);
    }
	// Currently only the video stream reports bytes
	else
    {
        bytes = 0;
    }

    return true;
}

bool CNTV2Config2110::GetRxByteCount(const eSFP sfp, uint64_t & bytes)
{
    uint32_t val_lo, val_hi;

    if (sfp == SFP_1)
    {
        mDevice.ReadRegister(SAREK_10G_EMAC_0 + kReg10gemac_rx_bytes_lo, val_lo);
        mDevice.ReadRegister(SAREK_10G_EMAC_0 + kReg10gemac_rx_bytes_hi, val_hi);
    }
    else
    {
        mDevice.ReadRegister(SAREK_10G_EMAC_1 + kReg10gemac_rx_bytes_lo, val_lo);
        mDevice.ReadRegister(SAREK_10G_EMAC_1 + kReg10gemac_rx_bytes_hi, val_hi);
    }

    bytes = ((uint64_t)val_hi << 32) + val_lo;
    return true;
}

bool CNTV2Config2110::GetTxPacketCount(const NTV2Stream stream, uint32_t & packets)
{
    if (StreamType(stream) == VIDEO_STREAM)
    {
		uint32_t count;

		uint32_t baseAddrPacketizer = GetPacketizerAddress(NTV2_VIDEO1_STREAM, GetSampling(SFP_1, stream));
        mDevice.ReadRegister(kReg4175_pkt_tx_pkt_cnt + baseAddrPacketizer, count);
        packets = count;
    }
    else
    {
        // TODO:
        packets = 0;
    }

    return true;
}

bool CNTV2Config2110::GetTxByteCount(const eSFP sfp, uint64_t & bytes)
{
    uint32_t val_lo, val_hi;

    if (sfp == SFP_1)
    {
        mDevice.ReadRegister(SAREK_10G_EMAC_0 + kReg10gemac_tx_bytes_lo, val_lo);
        mDevice.ReadRegister(SAREK_10G_EMAC_0 + kReg10gemac_tx_bytes_hi, val_hi);
    }
    else
    {
        mDevice.ReadRegister(SAREK_10G_EMAC_1 + kReg10gemac_tx_bytes_lo, val_lo);
        mDevice.ReadRegister(SAREK_10G_EMAC_1 + kReg10gemac_tx_bytes_hi, val_hi);
    }

    bytes = ((uint64_t)val_hi << 32) + val_lo;
    return true;
}

int CNTV2Config2110::LeastCommonMultiple(int a,int b)
{
    int m = a;
    int n = b;
    while (m != n)
    {
        if (m < n)
            m += a;
        else
            n +=b;
    }
    return m;
}

bool CNTV2Config2110::SetTxStreamConfiguration(const NTV2Stream stream, const tx_2110Config & txConfig)
{
    if (GetSFPActive(SFP_1) == false)
    {
        mIpErrorCode = NTV2IpErrSFP1NotConfigured;
        return false;
    }

    ResetPacketizerStream(stream);

    SetFramerStream(SFP_1, stream, txConfig);
    SetFramerStream(SFP_2, stream, txConfig);
	SetSampling(SFP_1, stream, txConfig.videoSamples);
	SetSampling(SFP_2, stream, txConfig.videoSamples);

    if (StreamType(stream) == VIDEO_STREAM)
    {
		// video setup 3190 packetizer
		uint32_t baseAddrPacketizer = GetPacketizerAddress(stream, GetSampling(SFP_1, stream));

		NTV2VideoFormat vfmt = txConfig.videoFormat;

        // Write the video format into the arbitrator
		SetVideoFormatForRxTx(stream, vfmt, false);

        // setup 4175 packetizer
		bool interlaced = false;
		if (!NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(vfmt))
		{
			interlaced = true;
		}
		else if (NTV2_IS_PSF_VIDEO_FORMAT(vfmt))
		{
			interlaced = true;
		}

		NTV2FormatDescriptor fd(vfmt,NTV2_FBF_10BIT_YCBCR);

        // width
        uint32_t width = fd.GetRasterWidth();
        mDevice.WriteRegister(kReg4175_pkt_width + baseAddrPacketizer,width);

        // height
        uint32_t height = fd.GetRasterHeight();
        if (interlaced)
        {
            height /= 2;
        }
        mDevice.WriteRegister(kReg4175_pkt_height + baseAddrPacketizer,height);

        // video format = sampling
        int vf;
        int componentsPerPixel;
        int componentsPerUnit;

		VPIDSampling sampling = GetSampling(SFP_1, stream);
		switch(sampling)
        {
        case VPIDSampling_GBR_444:
            vf = 0;
            componentsPerPixel = 3;
            componentsPerUnit  = 3;
            break;
        default:
        case VPIDSampling_YUV_422:
            componentsPerPixel = 2;
            componentsPerUnit  = 4;
            vf = 2;
            break;
        }
        mDevice.WriteRegister(kReg4175_pkt_vid_fmt + baseAddrPacketizer,vf);

        const int bitsPerComponent = 10;
        const int pixelsPerClock = 1;
        int activeLineLength    = (width * componentsPerPixel * bitsPerComponent)/8;

        int pixelGroup_root     = LeastCommonMultiple((bitsPerComponent * componentsPerUnit), 8);
        int pixelGroupSize      = pixelGroup_root/8;

        int bytesPerCycle       = (pixelsPerClock * bitsPerComponent * componentsPerPixel)/8;

        int lcm                = LeastCommonMultiple(pixelGroupSize,bytesPerCycle);
        int payloadLength_root =  min(activeLineLength,1376)/lcm;
        int payloadLength      = payloadLength_root * lcm;

        float pktsPerLine      = ((float)activeLineLength)/((float)payloadLength);
        int ipktsPerLine       = (int)ceil(pktsPerLine);

        int payloadLengthLast  = activeLineLength - (payloadLength * (ipktsPerLine -1));

		int ppp = (payloadLength/pixelGroupSize) * 2;   // as per JeffL

		if ((sampling == VPIDSampling_GBR_444) &&
			((mDevice.GetDeviceID() == DEVICE_ID_KONAIP_2110_RGB12) ||
			 (mDevice.GetDeviceID() == DEVICE_ID_IOIP_2110_RGB12)))
		{
			if (NTV2_IS_2K_1080_VIDEO_FORMAT(txConfig.videoFormat))
			{
				ipktsPerLine		= 0x07;
				payloadLength		= 0x0558;
				payloadLengthLast	= 0x03f0;
				ppp					= 0x0130;
			}
			else if (NTV2_IS_720P_VIDEO_FORMAT(txConfig.videoFormat))
			{
				ipktsPerLine		= 0x05;
				payloadLength		= 0x0558;
				payloadLengthLast	= 0x0120;
				ppp					= 0x0130;
			}
			// assume 1920x1080 format
			else
			{
				ipktsPerLine		= 0x07;
				payloadLength		= 0x0558;
				payloadLengthLast	= 0x01b0;
				ppp					= 0x0130;
			}
		}

        // pkts per line
        mDevice.WriteRegister(kReg4175_pkt_pkts_per_line + baseAddrPacketizer,ipktsPerLine);

        // payload length
        mDevice.WriteRegister(kReg4175_pkt_payload_len + baseAddrPacketizer,payloadLength);

        // payload length last
        mDevice.WriteRegister(kReg4175_pkt_payload_len_last + baseAddrPacketizer,payloadLengthLast);

        // payloadType
        mDevice.WriteRegister(kReg4175_pkt_payload_type + baseAddrPacketizer,txConfig.payloadType);

        // SSRC
        mDevice.WriteRegister(kReg4175_pkt_ssrc + baseAddrPacketizer,txConfig.ssrc);

        // pix per pkt
        mDevice.WriteRegister(kReg4175_pkt_pix_per_pkt + baseAddrPacketizer,ppp);

        // interlace
        int ilace = (interlaced) ? 0x01 : 0x00;
        mDevice.WriteRegister(kReg4175_pkt_interlace_ctrl + baseAddrPacketizer,ilace);

        // end setup 4175 packetizer
        SetTxFormat(VideoStreamToChannel(stream), txConfig.videoFormat);
    }
    else if (StreamType(stream) == AUDIO_STREAM)
    {
		// audio setup 3190 packetizer
		uint32_t	baseAddrPacketizer = GetPacketizerAddress(stream, GetSampling(SFP_1, stream));

        uint32_t audioChans = txConfig.numAudioChannels;
        uint32_t samples    = (txConfig.audioPktInterval == PACKET_INTERVAL_125uS) ? 6 : 48;
        uint32_t plength    = audioChans * samples * 3;

        // audio select
        uint32_t aselect = ((uint32_t)txConfig.firstAudioChannel << 16 ) + (audioChans-1);
        aselect = (txConfig.channel << 24) + aselect;
        uint32_t offset = (stream - NTV2_AUDIO1_STREAM) * 4;
        mDevice.WriteRegister(SAREK_2110_AUDIO_STREAMSELECT + offset, aselect);

        // num samples
        mDevice.WriteRegister(kReg3190_pkt_num_samples + baseAddrPacketizer, samples);

        // audio channels - zero-based (i.e. 0 = 1 channel)
        mDevice.WriteRegister(kReg3190_pkt_num_audio_channels + baseAddrPacketizer, audioChans);

        // payload length
        mDevice.WriteRegister(kReg3190_pkt_payload_len + baseAddrPacketizer, plength);

        // payloadType
        mDevice.WriteRegister(kReg3190_pkt_payload_type + baseAddrPacketizer, txConfig.payloadType);

        // ssrc
		mDevice.WriteRegister(kReg3190_pkt_ssrc + baseAddrPacketizer, txConfig.ssrc);
    }
	else if (StreamType(stream) == ANC_STREAM)
	{
		uint32_t channel = (uint32_t)(stream-NTV2_ANC1_STREAM);

		// Setup the anc inserter params, these are static values that don't change from frame to frame
		mDevice.AncInsertSetIPParams(channel, channel+4, txConfig.payloadType, txConfig.ssrc);

		// for anc streams we tuck away these values
		// payloadType
		mDevice.WriteRegister(kRegTxAncPayload1+channel + SAREK_2110_TX_ARBITRATOR, txConfig.payloadType);

		// ssrc
		mDevice.WriteRegister(kRegTxAncSSRC1+channel + SAREK_2110_TX_ARBITRATOR, txConfig.ssrc);
	}

	return true;
}


bool CNTV2Config2110::SetFramerStream(const eSFP sfp, const NTV2Stream stream, const tx_2110Config & txConfig)
{
    // get frame address
    uint32_t baseAddrFramer = GetFramerAddress(sfp, stream);

    // select channel
    SelectTxFramerChannel(stream, baseAddrFramer);

    // setup framer
    // hold off access while we update channel regs
    AcquireFramerControlAccess(baseAddrFramer);

    uint32_t val = (txConfig.tos << 8) | txConfig.ttl;
    WriteChannelRegister(kRegFramer_ip_hdr_media + baseAddrFramer, val);

    int index = (int)sfp;
    // dest ip address
    uint32_t destIp = inet_addr(txConfig.remoteIP[index].c_str());
    destIp = NTV2EndianSwap32(destIp);
    WriteChannelRegister(kRegFramer_dst_ip + baseAddrFramer,destIp);

    // source port
    WriteChannelRegister(kRegFramer_udp_src_port + baseAddrFramer,txConfig.localPort[index]);

    // dest port
    WriteChannelRegister(kRegFramer_udp_dst_port + baseAddrFramer,txConfig.remotePort[index]);

    // MAC address
    uint32_t    hi;
    uint32_t    lo;
    bool rv = GetMACAddress(sfp, stream, txConfig.remoteIP[index], hi, lo);
    if (!rv) return false;
    WriteChannelRegister(kRegFramer_dest_mac_lo  + baseAddrFramer,lo);
    WriteChannelRegister(kRegFramer_dest_mac_hi  + baseAddrFramer,hi);

    // enable  register updates
    ReleaseFramerControlAccess(baseAddrFramer);

    // end framer setup
    return true;
}


bool CNTV2Config2110::GetTxStreamConfiguration(const NTV2Stream stream, tx_2110Config & txConfig)
{
	uint32_t	baseAddrPacketizer;

    GetFramerStream(SFP_1, stream, txConfig);
    GetFramerStream(SFP_2, stream, txConfig);
	txConfig.videoSamples = GetSampling(SFP_1, stream);

    uint32_t val;
    if (StreamType(stream) == VIDEO_STREAM)
    {
		// select video packetizer
		baseAddrPacketizer = GetPacketizerAddress(stream, GetSampling(SFP_1, stream));

        // payloadType
        mDevice.ReadRegister(kReg4175_pkt_payload_type + baseAddrPacketizer, val);
        txConfig.payloadType = (uint16_t)val;

        // SSRC
        mDevice.ReadRegister(kReg4175_pkt_ssrc + baseAddrPacketizer, txConfig.ssrc);

        // Video format
        GetTxFormat(VideoStreamToChannel(stream), txConfig.videoFormat);
    }
	else if (StreamType(stream) == AUDIO_STREAM)
	{
		// select audio packetizer
		baseAddrPacketizer = GetPacketizerAddress(stream, GetSampling(SFP_1, stream));

        // audio - payloadType
        mDevice.ReadRegister(kReg3190_pkt_payload_type + baseAddrPacketizer, val);
        txConfig.payloadType = (uint16_t)val;

        // ssrc
        mDevice.ReadRegister(kReg3190_pkt_ssrc + baseAddrPacketizer, txConfig.ssrc);

        // audio select
        uint32_t offset  =  Get2110TxStreamIndex(stream) * 4;
        uint32_t aselect;
        mDevice.ReadRegister(SAREK_2110_AUDIO_STREAMSELECT + offset, aselect);

        txConfig.firstAudioChannel = (aselect >> 16) & 0xff;
        txConfig.numAudioChannels  = (aselect & 0xff) + 1;

        // packet interval
        uint32_t samples;
        mDevice.ReadRegister(kReg3190_pkt_num_samples + baseAddrPacketizer, samples);
        txConfig.audioPktInterval = (samples == 6) ? PACKET_INTERVAL_125uS : PACKET_INTERVAL_1mS;
    }
	else if (StreamType(stream) == ANC_STREAM)
	{
		uint32_t regOffset = (uint32_t)(stream-NTV2_ANC1_STREAM);

		// payloadType
		mDevice.ReadRegister(kRegTxAncPayload1 + regOffset + SAREK_2110_TX_ARBITRATOR, val);
		txConfig.payloadType = (uint16_t)val;

		// ssrc
		mDevice.ReadRegister(kRegTxAncSSRC1 + regOffset + SAREK_2110_TX_ARBITRATOR, txConfig.ssrc);
	}

    return true;
}

void CNTV2Config2110::GetFramerStream(const eSFP sfp, const NTV2Stream stream, tx_2110Config  & txConfig)
{
    int index = (int)sfp;

    // get frame address
    uint32_t baseAddrFramer = GetFramerAddress(sfp, stream);

    // Select channel
    SelectTxFramerChannel(stream, baseAddrFramer);

    uint32_t    val;
    ReadChannelRegister(kRegFramer_ip_hdr_media + baseAddrFramer,&val);
    txConfig.ttl = val & 0xff;
    txConfig.tos = (val & 0xff00) >> 8;

    // dest ip address
    ReadChannelRegister(kRegFramer_dst_ip + baseAddrFramer,&val);
    struct in_addr in;
    in.s_addr = NTV2EndianSwap32(val);
    char * ip = inet_ntoa(in);
    txConfig.remoteIP[index] = ip;

    // source port
    ReadChannelRegister(kRegFramer_udp_src_port + baseAddrFramer,&txConfig.localPort[index]);

    // dest port
    ReadChannelRegister(kRegFramer_udp_dst_port + baseAddrFramer,&txConfig.remotePort[index]);
}

bool CNTV2Config2110::SetTxStreamEnable(const NTV2Stream stream, bool enableSfp1, bool enableSfp2)
{
    if (enableSfp1 && (GetSFPActive(SFP_1) == false))
    {
        mIpErrorCode = NTV2IpErrSFP1NotConfigured;
        return false;
    }

    if (enableSfp2 && (GetSFPActive(SFP_2) == false))
    {
        mIpErrorCode = NTV2IpErrSFP2NotConfigured;
        return false;
    }

    EnableFramerStream(SFP_1, stream, enableSfp1);
    EnableFramerStream(SFP_2, stream, enableSfp2);
    SetArbiter(SFP_1, stream, enableSfp1);
    SetArbiter(SFP_2, stream, enableSfp2);

    // Generate and push the SDP
	GenSDP(enableSfp1, enableSfp2, stream);

	if ((StreamType(stream) == VIDEO_STREAM) || (StreamType(stream) == AUDIO_STREAM))
	{
		// ** Packetizer
		uint32_t	packetizerBaseAddr = GetPacketizerAddress(stream, GetSampling(SFP_1, stream));

		if (enableSfp1 || enableSfp2)
		{
			// enable
			mDevice.WriteRegister(kReg4175_pkt_ctrl + packetizerBaseAddr, 0x00);
			mDevice.WriteRegister(kReg4175_pkt_ctrl + packetizerBaseAddr, 0x80);
			mDevice.WriteRegister(kReg4175_pkt_ctrl + packetizerBaseAddr, 0x81);
		}
		else
		{
			// disable
			mDevice.WriteRegister(kReg4175_pkt_ctrl + packetizerBaseAddr, 0x00);
		}
	}

    return true;
}

void CNTV2Config2110::EnableFramerStream(const eSFP sfp, const NTV2Stream stream, bool enable)
{
    // ** Framer
    // get frame address
    uint32_t baseAddrFramer = GetFramerAddress(sfp, stream);

    // select channel
    SelectTxFramerChannel(stream, baseAddrFramer);

    // hold off access while we update channel regs
    AcquireFramerControlAccess(baseAddrFramer);

    if (enable)
    {
        uint32_t localIp;
        if (sfp == SFP_1)
        {
            mDevice.ReadRegister(SAREK_REGS + kRegSarekIP0, localIp);
        }
        else
        {
            mDevice.ReadRegister(SAREK_REGS + kRegSarekIP1, localIp);
        }

        WriteChannelRegister(kRegFramer_src_ip + baseAddrFramer, NTV2EndianSwap32(localIp));

        // enable
        WriteChannelRegister(kRegFramer_chan_ctrl + baseAddrFramer, 0x01);  // enables tx over mac1/mac2
    }
    else
    {
        // disable
        WriteChannelRegister(kRegFramer_chan_ctrl + baseAddrFramer, 0x0);   // disables channel
    }

    // enable  register updates
    ReleaseFramerControlAccess(baseAddrFramer);

    // ** Framer end
}

bool CNTV2Config2110::GetTxStreamEnable(const NTV2Stream stream, bool & sfp1Enabled, bool & sfp2Enabled)
{
    GetArbiter(SFP_1, stream, sfp1Enabled);
    GetArbiter(SFP_2, stream, sfp2Enabled);
    return true;
}

bool CNTV2Config2110::SetPTPPreferredGrandMasterId(const uint8_t id[8])
{
    uint32_t	val = ((uint32_t)id[0] << 24) | ((uint32_t)id[1] << 16) | ((uint32_t)id[2] << 8) | ((uint32_t)id[3] << 0);
    mDevice.WriteRegister(kRegPll_swptp_PreferredGmIdHi + SAREK_PLL, val);
    val = ((uint32_t)id[4] << 24) | ((uint32_t)id[5] << 16) | ((uint32_t)id[6] << 8) | ((uint32_t)id[7] << 0);
    mDevice.WriteRegister(kRegPll_swptp_PreferredGmIdLo + SAREK_PLL, val);
    return true;
}

bool CNTV2Config2110::GetPTPPreferredGrandMasterId(uint8_t (&id)[8])
{
    uint32_t val;
    mDevice.ReadRegister(kRegPll_swptp_PreferredGmIdHi + SAREK_PLL, val);
    id[0] = val >> 24;
    id[1] = val >> 16;
    id[2] = val >> 8;
    id[3] = val >> 0;
    mDevice.ReadRegister(kRegPll_swptp_PreferredGmIdLo + SAREK_PLL, val);
    id[4] = val >> 24;
    id[5] = val >> 16;
    id[6] = val >> 8;
    id[7] = val >> 0;
    return true;
}

bool CNTV2Config2110::SetPTPDomain(const uint8_t domain)
{
    mDevice.WriteRegister(kRegPll_swptp_Domain + SAREK_PLL, (uint32_t)domain);
    return true;
}

bool CNTV2Config2110::GetPTPDomain(uint8_t &domain)
{
    uint32_t val;
    mDevice.ReadRegister(kRegPll_swptp_Domain + SAREK_PLL, val);
    domain = val;
    return true;
}

bool CNTV2Config2110::GetPTPStatus(PTPStatus & ptpStatus)
{
    uint32_t val = 0;

    mDevice.ReadRegister(SAREK_PLL + kRegPll_swptp_GrandMasterIdHi, val);
    ptpStatus.PTP_gmId[0] = val >> 24;
    ptpStatus.PTP_gmId[1] = val >> 16;
    ptpStatus.PTP_gmId[2] = val >> 8;
    ptpStatus.PTP_gmId[3] = val >> 0;

    mDevice.ReadRegister(SAREK_PLL + kRegPll_swptp_GrandMasterIdLo, val);
    ptpStatus.PTP_gmId[4] = val >> 24;
    ptpStatus.PTP_gmId[5] = val >> 16;
    ptpStatus.PTP_gmId[6] = val >> 8;
    ptpStatus.PTP_gmId[7] = val >> 0;

    mDevice.ReadRegister(SAREK_PLL + kRegPll_swptp_MasterIdHi, val);
    ptpStatus.PTP_masterId[0] = val >> 24;
    ptpStatus.PTP_masterId[1] = val >> 16;
    ptpStatus.PTP_masterId[2] = val >> 8;
    ptpStatus.PTP_masterId[3] = val >> 0;

    mDevice.ReadRegister(SAREK_PLL + kRegPll_swptp_MasterIdLo, val);
    ptpStatus.PTP_masterId[4] = val >> 24;
    ptpStatus.PTP_masterId[5] = val >> 16;
    ptpStatus.PTP_masterId[6] = val >> 8;
    ptpStatus.PTP_masterId[7] = val >> 0;

    mDevice.ReadRegister(SAREK_PLL + kRegPll_swptp_LockedState, val);
    ptpStatus.PTP_LockedState = (PTPLockStatus)val;

    return true;
}

bool CNTV2Config2110::Set4KModeEnable(const bool enable)
{
    if (!mDevice.IsMBSystemReady())
    {
        mIpErrorCode = NTV2IpErrNotReady;
        return false;
    }

	uint32_t val;
	mDevice.ReadRegister(kRegArb_4KMode + SAREK_2110_TX_ARBITRATOR, val);
	if (enable)
		val |= BIT(0);
	else
		val &= ~BIT(0);

	mDevice.WriteRegister(kRegArb_4KMode + SAREK_2110_TX_ARBITRATOR,val);

    return true;
}

bool  CNTV2Config2110::Get4KModeEnable(bool & enable)
{
    uint32_t val;
	mDevice.ReadRegister(kRegArb_4KMode + SAREK_2110_TX_ARBITRATOR, val);

    enable = val & 0x01;
    return true;
}

bool CNTV2Config2110::SetAudioCombineEnable(const bool enable)
{
	if (!mDevice.IsMBSystemReady())
	{
		mIpErrorCode = NTV2IpErrNotReady;
		return false;
	}

	uint32_t val;
	mDevice.ReadRegister(kRegArb_4KMode + SAREK_2110_TX_ARBITRATOR, val);
	if (enable)
		val |= BIT(4);
	else
		val &= ~BIT(4);

	mDevice.WriteRegister(kRegArb_4KMode + SAREK_2110_TX_ARBITRATOR,val);

	return true;
}

bool  CNTV2Config2110::GetAudioCombineEnable(bool & enable)
{
	uint32_t val;
	mDevice.ReadRegister(kRegArb_4KMode + SAREK_2110_TX_ARBITRATOR, val);

	enable = val & 0x10;
	return true;
}

bool CNTV2Config2110::SetIPServicesControl(const bool enable, const bool forceReconfig)
{
    uint32_t val = 0;
    if (enable)
        val = 1;

    if (forceReconfig)
        val |= BIT(1);

    mDevice.WriteRegister(SAREK_REGS + kRegSarekServices, val);

    return true;
}

bool CNTV2Config2110::GetIPServicesControl(bool & enable, bool & forceReconfig)
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

bool CNTV2Config2110::SetIGMPDisable(const eSFP sfp, const bool disable)
{
    uint32_t val = (disable) ? 1 : 0;
    if (sfp == SFP_1 )
    {
        mDevice.WriteRegister(SAREK_REGS + kSarekRegIGMPDisable,val);
    }
    else
    {
        mDevice.WriteRegister(SAREK_REGS + kSarekRegIGMPDisable2,val);
    }
    return true;
}

bool CNTV2Config2110::GetIGMPDisable(const eSFP sfp, bool & disabled)
{
    uint32_t val;
    if (sfp == SFP_1 )
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

bool CNTV2Config2110::SetIGMPVersion(const eIGMPVersion_t version)
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

bool CNTV2Config2110::GetIGMPVersion(eIGMPVersion_t & version)
{
    uint32_t version32;
    bool rv = mDevice.ReadRegister(SAREK_REGS + kRegSarekIGMPVersion, version32);
    version =  (version32 == 2) ? eIGMPVersion_2 : eIGMPVersion_3;
    return rv;
}

/////////////////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////////////////

uint32_t CNTV2Config2110::GetDecapsulatorAddress(eSFP sfp, NTV2Stream stream)
{
    uint32_t offset = 0;

    switch (stream)
    {
        case NTV2_VIDEO1_STREAM:
            offset = 0x00;
            break;
        case NTV2_VIDEO2_STREAM:
            offset = 0x20;
            break;
        case NTV2_VIDEO3_STREAM:
            offset = 0x40;
            break;
        case NTV2_VIDEO4_STREAM:
            offset = 0x60;
            break;
        case NTV2_AUDIO1_STREAM:
            offset = 0x10;
            break;
        case NTV2_AUDIO2_STREAM:
            offset = 0x30;
            break;
        case NTV2_AUDIO3_STREAM:
            offset = 0x50;
            break;
        case NTV2_AUDIO4_STREAM:
            offset = 0x70;
            break;
		case NTV2_ANC1_STREAM:
			offset = 0x80;
			break;
		case NTV2_ANC2_STREAM:
			offset = 0x90;
			break;
		case NTV2_ANC3_STREAM:
			offset = 0xA0;
			break;
		case NTV2_ANC4_STREAM:
			offset = 0xB0;
			break;

        default:
            break;
    }

    if (sfp == SFP_1)
        return SAREK_2110_DECAPSULATOR_0 + offset;
    else
        return SAREK_2110_DECAPSULATOR_1 + offset;
}

uint32_t CNTV2Config2110::GetDepacketizerAddress(NTV2Stream stream)
{
	uint32_t basePacketizer = 0;

	// Only video and audio streams have depacketizers
	if (StreamType(stream) == VIDEO_STREAM)
	{
		basePacketizer = videoDepacketizers[stream-NTV2_VIDEO1_STREAM];
	}
	else if (StreamType(stream) == AUDIO_STREAM)
	{
		basePacketizer = audioDepacketizers[stream-NTV2_AUDIO1_STREAM];
	}

	return basePacketizer;
}

uint32_t CNTV2Config2110::GetFramerAddress(const eSFP sfp, const NTV2Stream stream)
{
    if (sfp == SFP_2)
    {
        if (StreamType(stream) == VIDEO_STREAM)
            return SAREK_2110_VIDEO_FRAMER_1;
        else
			return SAREK_2110_AUDIO_ANC_FRAMER_1;
    }
    else
    {
        if (StreamType(stream) == VIDEO_STREAM)
            return SAREK_2110_VIDEO_FRAMER_0;
        else
			return SAREK_2110_AUDIO_ANC_FRAMER_0;
    }
}

void CNTV2Config2110::SelectTxFramerChannel(const NTV2Stream stream, const uint32_t baseAddrFramer)
{
    // select channel
    uint32_t index = Get2110TxStreamIndex(stream);
    SetChannel(kRegFramer_channel_access + baseAddrFramer, index);
}

uint32_t CNTV2Config2110::GetPacketizerAddress(const NTV2Stream stream, const VPIDSampling sampling)
{
	uint32_t basePacketizer = 0;

	// Only video and audio streams have packetizers
    if (StreamType(stream) == VIDEO_STREAM)
    {
		if ((sampling == VPIDSampling_GBR_444) &&
			((mDevice.GetDeviceID() == DEVICE_ID_KONAIP_2110_RGB12) ||
			 (mDevice.GetDeviceID() == DEVICE_ID_IOIP_2110_RGB12)))
			basePacketizer = videoRGB12Packetizers[stream-NTV2_VIDEO1_STREAM];
		else
			basePacketizer = videoPacketizers[stream-NTV2_VIDEO1_STREAM];

		uint32_t index = Get2110TxStreamIndex(stream);
		mDevice.WriteRegister(kReg4175_pkt_chan_num + basePacketizer, index);
    }
    else if (StreamType(stream) == AUDIO_STREAM)
    {
		basePacketizer = audioPacketizers[stream-NTV2_AUDIO1_STREAM];
		uint32_t index = Get2110TxStreamIndex(stream);
		mDevice.WriteRegister(kReg3190_pkt_chan_num + basePacketizer, index);
    }
	
	return basePacketizer;
}

bool  CNTV2Config2110::ConfigurePTP (const eSFP sfp, const std::string localIPAddress)
{
    uint32_t macLo;
    uint32_t macHi;

    // get primaray mac address
    uint32_t macAddressRegister = SAREK_REGS + kRegSarekMAC;
    if (sfp != SFP_1)
    {
        macAddressRegister += 2;
    }
    mDevice.ReadRegister(macAddressRegister, macHi);
    macAddressRegister++;
    mDevice.ReadRegister(macAddressRegister, macLo);

    uint32_t alignedMACHi = macHi >> 16;
    uint32_t alignedMACLo = (macLo >> 16) | ( (macHi & 0xffff) << 16);

    uint32_t addr = inet_addr(localIPAddress.c_str());
    addr = NTV2EndianSwap32(addr);

    // configure pll
    mDevice.WriteRegister(kRegPll_PTP_LclMacLo   + SAREK_PLL, alignedMACLo);
    mDevice.WriteRegister(kRegPll_PTP_LclMacHi   + SAREK_PLL, alignedMACHi);

    mDevice.WriteRegister(kRegPll_PTP_EventUdp   + SAREK_PLL, 0x0000013f);
    mDevice.WriteRegister(kRegPll_PTP_MstrMcast  + SAREK_PLL, 0xe0000181);
    mDevice.WriteRegister(kRegPll_PTP_LclIP      + SAREK_PLL, addr);
    uint32_t val;
    mDevice.ReadRegister(kRegPll_PTP_MstrIP      + SAREK_PLL, val);
    if (val == 0)
        mDevice.WriteRegister(kRegPll_PTP_Match  + SAREK_PLL, 0x1);
    else
        mDevice.WriteRegister(kRegPll_PTP_Match  + SAREK_PLL, 0x9);
    mDevice.WriteRegister(kRegPll_Config         + SAREK_PLL, PLL_CONFIG_PTP | PLL_CONFIG_DCO_MODE);

    //WriteChannelRegister(kRegPll_PTP_LclClkIdLo + SAREK_PLL, (0xfe << 24) | ((macHi & 0x000000ff) << 16) | (macLo >> 16));
    //WriteChannelRegister(kRegPll_PTP_LclClkIdHi + SAREK_PLL, (macHi & 0xffffff00) | 0xff);

    return true;
}

bool CNTV2Config2110::GetSFPMSAData(eSFP sfp, SFPMSAData & data)
{
    return GetSFPInfo(sfp, data);
}

bool CNTV2Config2110::GetLinkStatus(eSFP sfp, SFPStatus & sfpStatus)
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

string CNTV2Config2110::getLastError()
{
    return NTV2IpErrorEnumToString(getLastErrorCode());
}

NTV2IpError CNTV2Config2110::getLastErrorCode()
{
    NTV2IpError error = mIpErrorCode;
    mIpErrorCode = NTV2IpErrNone;
    return error;
}

void CNTV2Config2110::AcquireFramerControlAccess(const uint32_t baseAddr)
{
    WriteChannelRegister(kRegFramer_control + baseAddr, 0x00);
    uint32_t val;
    mDevice.ReadRegister(kRegFramer_status + baseAddr, val);
    while (val & BIT(1))
    {
        // Wait
        #if defined(AJAWindows) || defined(MSWindows)
            ::Sleep (10);
        #else
            usleep (10 * 1000);
        #endif

        mDevice.ReadRegister(kRegFramer_status + baseAddr, val);
    }
}

void CNTV2Config2110::ReleaseFramerControlAccess(const uint32_t baseAddr)
{
    WriteChannelRegister(kRegFramer_control + baseAddr, 0x02);
}

uint32_t CNTV2Config2110::Get2110TxStreamIndex(NTV2Stream stream)
{
    // this stream number is a core 'channel' number
    uint32_t index = 0;
	switch (stream)
    {
        case NTV2_VIDEO1_STREAM:
        case NTV2_VIDEO2_STREAM:
        case NTV2_VIDEO3_STREAM:
        case NTV2_VIDEO4_STREAM:
			index = (uint32_t)(stream-NTV2_VIDEO1_STREAM);
            break;

        case NTV2_AUDIO1_STREAM:
        case NTV2_AUDIO2_STREAM:
        case NTV2_AUDIO3_STREAM:
        case NTV2_AUDIO4_STREAM:
		case NTV2_ANC1_STREAM:
		case NTV2_ANC2_STREAM:
		case NTV2_ANC3_STREAM:
		case NTV2_ANC4_STREAM:
			index = (uint32_t)(stream-NTV2_AUDIO1_STREAM);
            break;

		case NTV2_VIDEO4K_STREAM:
		default:
			break;
    }
    return index;
}

bool CNTV2Config2110::GetMACAddress(const eSFP port, const NTV2Stream stream, string remoteIP, uint32_t & hi, uint32_t & lo)
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
        GetNetworkConfiguration(port, nc);
        if ( (destIp & nc.ipc_subnet) != (nc.ipc_ip & nc.ipc_subnet))
        {
            struct in_addr addr;
            addr.s_addr  = NTV2EndianSwap32(nc.ipc_gateway);
            string gateIp = inet_ntoa(addr);
            rv = GetRemoteMAC(gateIp, port, stream, macAddr);
        }
        else
        {
            rv = GetRemoteMAC(remoteIP, port, stream, macAddr);
        }
        if (!rv)
        {
            SetTxStreamEnable(stream, false);
            mIpErrorCode = NTV2IpErrCannotGetMacAddress;
            return false;
        }

        istringstream ss(macAddr);
        string token;
        int i=0;
        while (i < 6)
        {
            getline (ss, token, ':');
            macaddr.mac[i++] = (uint8_t)strtoul(token.c_str(), NULL, 16);
        }
    }

    hi  = macaddr.mac[0]  << 8;
    hi += macaddr.mac[1];

    lo  = macaddr.mac[2] << 24;
    lo += macaddr.mac[3] << 16;
    lo += macaddr.mac[4] << 8;
    lo += macaddr.mac[5];

    return true;
}

string CNTV2Config2110::GetSDPUrl(const eSFP sfp, const NTV2Stream stream)
{
    string localIPAddress, subnetMask, gateway;
    string preAmble = "http://";
	string namePre = "tx";
    string namePost;

    GetNetworkConfiguration(sfp, localIPAddress, subnetMask, gateway);

    switch (stream)
    {
		case NTV2_VIDEO1_STREAM:    namePost = "video1.sdp";	break;
		case NTV2_VIDEO2_STREAM:    namePost = "video2.sdp";	break;
		case NTV2_VIDEO3_STREAM:    namePost = "video3.sdp";	break;
		case NTV2_VIDEO4_STREAM:    namePost = "video4.sdp";	break;
		case NTV2_AUDIO1_STREAM:    namePost = "audio1.sdp";	break;
		case NTV2_AUDIO2_STREAM:    namePost = "audio2.sdp";	break;
		case NTV2_AUDIO3_STREAM:    namePost = "audio3.sdp";	break;
		case NTV2_AUDIO4_STREAM:    namePost = "audio4.sdp";	break;
		case NTV2_ANC1_STREAM:		namePost = "anc1.sdp";		break;
		case NTV2_ANC2_STREAM:		namePost = "anc2.sdp";		break;
		case NTV2_ANC3_STREAM:		namePost = "anc3.sdp";		break;
		case NTV2_ANC4_STREAM:		namePost = "anc4.sdp";		break;
		case NTV2_VIDEO4K_STREAM:	namePost = "video4K.sdp";	break;

        default:                    namePost = "";         break;
    }

    return preAmble + localIPAddress + "/" + namePre + namePost;
}

string CNTV2Config2110::GetGeneratedSDP(bool enabledSfp1, bool enabledSfp2, const NTV2Stream stream)
{
	GenSDP(enabledSfp1, enabledSfp2, stream, false);
    return txsdp.str();
}

string CNTV2Config2110::To_String(int val)
{
    ostringstream oss;
    oss << val;
    return oss.str();
}

bool CNTV2Config2110::GenSDP(const bool enableSfp1, const bool enableSfp2,
											const NTV2Stream stream, bool pushit)
{
    stringstream & sdp = txsdp;

    sdp.str("");
    sdp.clear();

    // protocol version
    sdp << "v=0" << endl;

	// username session-id  version network-type address-type address
	sdp << "o=- ";

	uint64_t t = GetNTPTimestamp();
	sdp <<  To_String((int)t);

	sdp << " 0 IN IP4 ";

	uint32_t val;
	// o is required but for multi SDP's we will just assume the originator is SFP_1
	if (!enableSfp1 && (StreamType(stream) != VIDEO_4K_STREAM))
		mDevice.ReadRegister(SAREK_REGS + kRegSarekIP1, val);
	else
		mDevice.ReadRegister(SAREK_REGS + kRegSarekIP0, val);

	struct in_addr addr;
	addr.s_addr = val;
	string localIPAddress = inet_ntoa(addr);
	sdp << localIPAddress << endl;

    // session name
    sdp << "s=AJA KonaIP 2110" << endl;

    // time the session is active
    sdp << "t=0 0" <<endl;

    // PTP
    PTPStatus ptpStatus;
	GetPTPStatus(ptpStatus);

    char gmInfo[32];
    sprintf(gmInfo, "%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X",
            ptpStatus.PTP_gmId[0], ptpStatus.PTP_gmId[1], ptpStatus.PTP_gmId[2], ptpStatus.PTP_gmId[3],
            ptpStatus.PTP_gmId[4], ptpStatus.PTP_gmId[5], ptpStatus.PTP_gmId[6], ptpStatus.PTP_gmId[7]);


    if (StreamType(stream) == VIDEO_STREAM)
    {
		GenVideoStreamSDP(sdp, enableSfp1, enableSfp2, stream, &gmInfo[0]);
    }
	else if (StreamType(stream) == VIDEO_4K_STREAM)
	{
		GenVideoStreamMultiSDPInfo(sdp, &gmInfo[0]);
	}
    else if (StreamType(stream) == AUDIO_STREAM)
    {
		GenAudioStreamSDP(sdp, enableSfp1, enableSfp2, stream, &gmInfo[0]);
    }
	else if (StreamType(stream) == ANC_STREAM)
	{
		GenAncStreamSDP(sdp, enableSfp1, enableSfp2, stream, &gmInfo[0]);
	}
    
	//cout << "SDP --------------- " << stream << endl << sdp.str() << endl;

	bool rv = true;

    if (pushit)
	{
		string filename = "tx";

		switch (stream)
		{
			case NTV2_VIDEO1_STREAM:    filename += "video1.sdp";   break;
			case NTV2_VIDEO2_STREAM:    filename += "video2.sdp";   break;
			case NTV2_VIDEO3_STREAM:    filename += "video3.sdp";   break;
			case NTV2_VIDEO4_STREAM:    filename += "video4.sdp";   break;
			case NTV2_AUDIO1_STREAM:    filename += "audio1.sdp";   break;
			case NTV2_AUDIO2_STREAM:    filename += "audio2.sdp";   break;
			case NTV2_AUDIO3_STREAM:    filename += "audio3.sdp";   break;
			case NTV2_AUDIO4_STREAM:    filename += "audio4.sdp";   break;
			case NTV2_ANC1_STREAM:		filename += "anc1.sdp";		break;
			case NTV2_ANC2_STREAM:		filename += "anc2.sdp";		break;
			case NTV2_ANC3_STREAM:		filename += "anc3.sdp";		break;
			case NTV2_ANC4_STREAM:		filename += "anc4.sdp";		break;
			case NTV2_VIDEO4K_STREAM:	filename += "video4K.sdp";  break;
			default:                    filename += "";				break;
		}
        rv = PushSDP(filename,sdp);
	}

    return rv;
}

bool CNTV2Config2110::GenVideoStreamSDP(stringstream &sdp, const bool enableSfp1,
					const bool enableSfp2, const NTV2Stream stream, char *gmInfo)
{
	bool isDash7 = enableSfp1 && enableSfp2;
	if (isDash7)
	{
		sdp << "a=group:DUP 1 2" << endl;
	}
	if (enableSfp1)
	{
		GenVideoStreamSDPInfo(sdp, SFP_1, stream, gmInfo);
		sdp << "a=mid:" << (isDash7?"1":"VID") << endl;
	}
	if (enableSfp2)
	{
		GenVideoStreamSDPInfo(sdp, SFP_2, stream, gmInfo);
		sdp << "a=mid:" << (isDash7?"2":"VID") << endl;
	}
	return true;
}

bool CNTV2Config2110::GenVideoStreamSDPInfo(stringstream & sdp, const eSFP sfp, const NTV2Stream stream, char* gmInfo)
{
    tx_2110Config config;
    GetTxStreamConfiguration(stream, config);

	uint32_t baseAddrPacketizer = GetPacketizerAddress(stream, GetSampling(sfp, stream));

    uint32_t width;
    mDevice.ReadRegister(kReg4175_pkt_width + baseAddrPacketizer, width);

    uint32_t height;
    mDevice.ReadRegister(kReg4175_pkt_height + baseAddrPacketizer, height);

    uint32_t  ilace;
    mDevice.ReadRegister(kReg4175_pkt_interlace_ctrl + baseAddrPacketizer, ilace);

    if (ilace == 1)
    {
        height *= 2;
    }

    NTV2VideoFormat vfmt;
    GetTxFormat(VideoStreamToChannel(stream), vfmt);
    NTV2FrameRate frate = GetNTV2FrameRateFromVideoFormat(vfmt);
    string rateString   = rateToString(frate);

    // media name
    sdp << "m=video ";
    if (sfp == SFP_2)
        sdp << To_String(config.remotePort[1]);
    else
        sdp << To_String(config.remotePort[0]);

    sdp << " RTP/AVP ";
    sdp << To_String(config.payloadType) << endl;

    // connection information
    sdp << "c=IN IP4 ";
    if (sfp == SFP_2)
        sdp << config.remoteIP[1];
    else
        sdp << config.remoteIP[0];
    sdp << "/" << To_String(config.ttl) << endl;

	// source information
	sdp << "a=source-filter: incl IN IP4 ";
	uint32_t val;

	if (sfp == SFP_2)
	{
		sdp << config.remoteIP[1];
		mDevice.ReadRegister(SAREK_REGS + kRegSarekIP1, val);
	}
	else
	{
		sdp << config.remoteIP[0];
		mDevice.ReadRegister(SAREK_REGS + kRegSarekIP0, val);
	}

	struct in_addr addr;
	addr.s_addr = val;
	string localIPAddress = inet_ntoa(addr);
	sdp << ' ' << localIPAddress << endl;

    // rtpmap
    sdp << "a=rtpmap:";
    sdp << To_String(config.payloadType);
    sdp << " raw/90000" << endl;

    //fmtp
    sdp << "a=fmtp:";
    sdp << To_String(config.payloadType);
	if (config.videoSamples == VPIDSampling_GBR_444)
		sdp << " sampling=RGB; width=";
	else
		sdp << " sampling=YCbCr-4:2:2; width=";

    sdp << To_String(width);
    sdp << "; height=";
    sdp << To_String(height);
    sdp << "; exactframerate=";
    sdp << rateString;
    sdp << "; depth=10; TCS=SDR; colorimetry=";
    sdp << ((NTV2_IS_SD_VIDEO_FORMAT(vfmt)) ? "BT601" : "BT709");
    sdp << "; PM=2110GPM; SSN=ST2110-20:2017; TP=2110TPN; ";
    if (!NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(vfmt))
    {
		sdp << "interlace; ";
    }
    else if (NTV2_IS_PSF_VIDEO_FORMAT(vfmt))
    {
        sdp << "interlace segmented";
    }
    sdp << endl;

    // PTP
    sdp << "a=ts-refclk:ptp=IEEE1588-2008:" << gmInfo << endl;
    sdp << "a=mediaclk:direct=0" << endl;

    return true;
}


bool CNTV2Config2110::GenVideoStreamMultiSDPInfo(stringstream & sdp, char* gmInfo)
{
	uint32_t 	quadSwapOut;
	NTV2Stream 	stream;

	// Read virtual to see if we are to quad swap the outputs
	mDevice.ReadRegister(kVRegSwizzle4kOutput, quadSwapOut);

	sdp << "a=group:MULTI-2SI 1 2 3 4 " << endl;

	// generate SDP's for all 4 video streams
	for (int i=0; i<4; i++)
	{
		if (quadSwapOut != 0)
		{
			switch (i)
			{
				case 0: stream = NTV2_VIDEO3_STREAM; break;
				case 1: stream = NTV2_VIDEO4_STREAM; break;
				case 2: stream = NTV2_VIDEO1_STREAM; break;
				case 3: stream = NTV2_VIDEO2_STREAM; break;
			}
		}
		else
			stream = (NTV2Stream)i;

		bool enabledA;
		bool enabledB;
		// See which steam is enabled, the code is written in such a way so that
		// if neither SFP1 or SFP2 is enabled it will use data from SFP1
		GetTxStreamEnable(stream, enabledA, enabledB);

		tx_2110Config config;
		GetTxStreamConfiguration(stream, config);

		uint32_t baseAddrPacketizer = GetPacketizerAddress(stream, GetSampling(SFP_1, stream));

		uint32_t width;
		mDevice.ReadRegister(kReg4175_pkt_width + baseAddrPacketizer, width);

		uint32_t height;
		mDevice.ReadRegister(kReg4175_pkt_height + baseAddrPacketizer, height);

		uint32_t  ilace;
		mDevice.ReadRegister(kReg4175_pkt_interlace_ctrl + baseAddrPacketizer, ilace);

		if (ilace == 1)
		{
			height *= 2;
		}

		NTV2VideoFormat vfmt;
		GetTxFormat(VideoStreamToChannel(stream), vfmt);
		NTV2FrameRate frate = GetNTV2FrameRateFromVideoFormat(vfmt);
		string rateString   = rateToString(frate);

		// media name
		sdp << "m=video ";
		if (enabledB)
			sdp << To_String(config.remotePort[1]);
		else
			sdp << To_String(config.remotePort[0]);

		sdp << " RTP/AVP ";
		sdp << To_String(config.payloadType) << endl;

		// dest information
		sdp << "c=IN IP4 ";
		if (enabledB)
			sdp << config.remoteIP[1];
		else
			sdp << config.remoteIP[0];
		sdp << "/" << To_String(config.ttl) << endl;

		// source information
		sdp << "a=source-filter: incl IN IP4 ";
		uint32_t val;

		if (enabledB)
		{
			sdp << config.remoteIP[1];
			mDevice.ReadRegister(SAREK_REGS + kRegSarekIP1, val);
		}
		else
		{
			sdp << config.remoteIP[0];
			mDevice.ReadRegister(SAREK_REGS + kRegSarekIP0, val);
		}

		struct in_addr addr;
		addr.s_addr = val;
		string localIPAddress = inet_ntoa(addr);
		sdp << ' ' << localIPAddress << endl;

		// rtpmap
		sdp << "a=rtpmap:";
		sdp << To_String(config.payloadType);
		sdp << " raw/90000" << endl;

		//fmtp
		sdp << "a=fmtp:";
		sdp << To_String(config.payloadType);
		sdp << " sampling=YCbCr-4:2:2; width=";
		sdp << To_String(width);
		sdp << "; height=";
		sdp << To_String(height);
		sdp << "; exactframerate=";
		sdp << rateString;
		sdp << "; depth=10; TCS=SDR; colorimetry=";
		sdp << ((NTV2_IS_SD_VIDEO_FORMAT(vfmt)) ? "BT601" : "BT709");
		sdp << "; PM=2110GPM; SSN=ST2110-20:2017; TP=2110TPN; ";
		if (!NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(vfmt))
		{
			sdp << "interlace; ";
		}
		else if (NTV2_IS_PSF_VIDEO_FORMAT(vfmt))
		{
			sdp << "interlace segmented";
		}
		sdp << endl;

		// PTP
		sdp << "a=ts-refclk:ptp=IEEE1588-2008:" << gmInfo << endl;
		sdp << "a=mediaclk:direct=0" << endl;
		sdp << "a=mid:" << i+1 << endl;
	}

	return true;
}


bool CNTV2Config2110::GenAudioStreamSDP(stringstream &sdp, const bool enableSfp1,
					const bool enableSfp2, const NTV2Stream stream, char *gmInfo)
{
	bool isDash7 = enableSfp1 && enableSfp2;
	if (isDash7)
	{
		sdp << "a=group:DUP 1 2" << endl;
	}
	if (enableSfp1)
	{
		GenAudioStreamSDPInfo(sdp, SFP_1, stream, gmInfo);
		sdp << "a=mid:" << (isDash7?"1":"VID") << endl;
	}
	if (enableSfp2)
	{
		GenAudioStreamSDPInfo(sdp, SFP_2, stream, gmInfo);
		sdp << "a=mid:" << (isDash7?"2":"VID") << endl;
	}
	return true;
}

bool CNTV2Config2110::GenAudioStreamSDPInfo(stringstream & sdp, const eSFP sfp, const  NTV2Stream stream, char* gmInfo)
{
    tx_2110Config config;
    GetTxStreamConfiguration(stream, config);

    // media name
    sdp << "m=audio ";
    if (sfp == SFP_2)
        sdp << To_String(config.remotePort[1]);
    else
        sdp << To_String(config.remotePort[0]);

    sdp << " RTP/AVP ";
    sdp << To_String(config.payloadType) << endl;

    // connection information
    sdp << "c=IN IP4 ";
    if (sfp == SFP_2)
        sdp << config.remoteIP[1];
    else
        sdp << config.remoteIP[0];

    sdp << "/" << To_String(config.ttl) << endl;

	// source information
	sdp << "a=source-filter: incl IN IP4 ";
	uint32_t val;

	if (sfp == SFP_2)
	{
		sdp << config.remoteIP[1];
		mDevice.ReadRegister(SAREK_REGS + kRegSarekIP1, val);
	}
	else
	{
		sdp << config.remoteIP[0];
		mDevice.ReadRegister(SAREK_REGS + kRegSarekIP0, val);
	}

	struct in_addr addr;
	addr.s_addr = val;
	string localIPAddress = inet_ntoa(addr);
	sdp << ' ' << localIPAddress << endl;

    // rtpmap
    sdp << "a=rtpmap:";
    sdp << To_String(config.payloadType);
    sdp << " L24/48000/";
    sdp << To_String(config.numAudioChannels) << endl;

    //fmtp
    sdp << "a=fmtp:";
    sdp << To_String(config.payloadType);
    sdp << " channel-order=SMPTE2110.(";
    switch (config.numAudioChannels)
    {
    case 2:
        sdp << "ST)";
        break;
    case 4:
        sdp << "SGRP)";
        break;
    case 8:
    default:
        sdp << "SGRP,SGRP)";
        break;
    case 12:
        sdp << "SGRP,SGRP,SGRP)";
        break;
    case 16:
        sdp << "SGRP,SGRP,SGRP,SGRP)";
        break;
    }
    sdp << endl;

    if (config. audioPktInterval == PACKET_INTERVAL_125uS)
    {
        sdp << "a=ptime:0.125" << endl;
    }
    else
    {
        sdp << "a=ptime:1.000" << endl;
    }

	sdp << "a=ts-refclk:ptp=IEEE1588-2008:" << gmInfo << endl;
    sdp << "a=mediaclk:direct=0" << endl;

    return true;
}

bool CNTV2Config2110::GenAncStreamSDP(stringstream &sdp, const bool enableSfp1,
					const bool enableSfp2, const NTV2Stream stream, char *gmInfo)
{
	bool isDash7 = enableSfp1 && enableSfp2;
	if (isDash7)
	{
		sdp << "a=group:DUP 1 2" << endl;
	}
	if (enableSfp1)
	{
		GenAncStreamSDPInfo(sdp, SFP_1, stream, gmInfo);
		sdp << "a=mid:" << (isDash7?"1":"VID") << endl;
	}
	if (enableSfp2)
	{
		GenAncStreamSDPInfo(sdp, SFP_2, stream, gmInfo);
		sdp << "a=mid:" << (isDash7?"2":"VID") << endl;
	}
	return true;
}

bool CNTV2Config2110::GenAncStreamSDPInfo(stringstream & sdp, const eSFP sfp, const NTV2Stream stream, char* gmInfo)
{
	tx_2110Config config;
	GetTxStreamConfiguration(stream, config);

	// media name
	sdp << "m=video ";
	if (sfp == SFP_2)
		sdp << To_String(config.remotePort[1]);
	else
		sdp << To_String(config.remotePort[0]);

	sdp << " RTP/AVP ";
	sdp << To_String(config.payloadType) << endl;

	// connection information
	sdp << "c=IN IP4 ";
	if (sfp == SFP_2)
		sdp << config.remoteIP[1];
	else
		sdp << config.remoteIP[0];
	sdp << "/" << To_String(config.ttl) << endl;

	// source information
	sdp << "a=source-filter: incl IN IP4 ";
	uint32_t val;

	if (sfp == SFP_2)
	{
		sdp << config.remoteIP[1];
		mDevice.ReadRegister(SAREK_REGS + kRegSarekIP1, val);
	}
	else
	{
		sdp << config.remoteIP[0];
		mDevice.ReadRegister(SAREK_REGS + kRegSarekIP0, val);
	}

	struct in_addr addr;
	addr.s_addr = val;
	string localIPAddress = inet_ntoa(addr);
	sdp << ' ' << localIPAddress << endl;

	// rtpmap
	sdp << "a=rtpmap:";
	sdp << To_String(config.payloadType);
	sdp << " smpte291/90000" << endl;

	// PTP
	sdp << "a=ts-refclk:ptp=IEEE1588-2008:" << gmInfo << endl;
	sdp << "a=mediaclk:direct=0" << endl;

	return true;
}


NTV2StreamType CNTV2Config2110::StreamType(const NTV2Stream stream)
{
    NTV2StreamType type = INVALID_STREAM;

    switch (stream)
    {
        case NTV2_VIDEO1_STREAM:
        case NTV2_VIDEO2_STREAM:
        case NTV2_VIDEO3_STREAM:
        case NTV2_VIDEO4_STREAM:
            type = VIDEO_STREAM;
            break;
        case NTV2_AUDIO1_STREAM:
        case NTV2_AUDIO2_STREAM:
        case NTV2_AUDIO3_STREAM:
        case NTV2_AUDIO4_STREAM:
            type = AUDIO_STREAM;
            break;
        case NTV2_ANC1_STREAM:
        case NTV2_ANC2_STREAM:
        case NTV2_ANC3_STREAM:
        case NTV2_ANC4_STREAM:
            type = ANC_STREAM;
            break;

		case NTV2_VIDEO4K_STREAM:
			type = VIDEO_4K_STREAM;
			break;

        default:
            type = INVALID_STREAM;
            break;
    }
    return type;
}

NTV2Channel CNTV2Config2110::VideoStreamToChannel(const NTV2Stream stream)
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


bool  CNTV2Config2110::GetActualSDP(std::string url, std::string & sdp)
{
    return GetSDP(url, sdp);
}


int CNTV2Config2110::getDescriptionValue(int startLine, string type, string & value)
{
    for (unsigned i(startLine);  i < sdpLines.size();  i++)
    {
        string line = sdpLines[i];
        size_t pos = line.find(type);
        if (pos != string::npos)
        {
            value = line.substr(pos + type.size() + 1);
            return i;
        }
    }
    return -1; // not found
}

string CNTV2Config2110::getVideoDescriptionValue(string type)
{
    vector<string>::iterator it;
    for (it = tokens.begin(); it != tokens.end(); it++)
    {
        string line = *it;
        size_t pos = line.find(type);
        if (pos != string::npos)
        {
            line = line.substr(pos + type.size());
            line.erase(remove(line.begin(), line.end(), ';'), line.end());
            return  line;
        }
    }
    string result;
    return result; // not found
}

vector<string> CNTV2Config2110::split(const char *str, char delim)
{
    vector<string> result;
    do
    {
        const char * begin = str;
        while(*str != delim && *str)
        {
            str++;
        }
        result.push_back(string(begin, str));
    } while (0 != *str++);
    return result;
}

bool CNTV2Config2110::ExtractRxVideoConfigFromSDP(std::string sdp, multiRx_2110Config & rxConfig)
{
	if (sdp.empty())
	{
		mIpErrorCode = NTV2IpErrSDPEmpty;
		return false;
	}

    // remove any carriage returns
    sdp.erase(remove(sdp.begin(), sdp.end(), '\r'), sdp.end());

	// break into a vector of lines and then into tokenw
	sdpLines.clear();
	stringstream ss(sdp);
	string to;

	while(getline(ss,to,'\n'))
	{
		sdpLines.push_back(to);
	}

	// rudimentary check it is an sdp file
	int index;
	string value;

	// is this really an SDP
	index = getDescriptionValue(0,"v=",value);
	if (index == -1)
	{
		mIpErrorCode = NTV2IpErrSDPInvalid;
		return false;
	}

	// make sure this is a multi-2si sdp
	index = getDescriptionValue(index,"a=group",value);
	if (index == -1)
	{
		mIpErrorCode = NTV2IpErrSDPInvalid;
		return false;
	}

	tokens = split(value.c_str(), ' ');
	if (!((tokens.size() != 5) && (tokens[0] == "MULTI-2SI")))
	{
		mIpErrorCode = NTV2IpErrSDPInvalid;
		return false;
	}

	// fill in RX struct for each stream
	for (int i=0; i<4; i++)
	{
		uint32_t rxMatch = 0;
		int rv;

		rxConfig.rx2110Config[i].sourceIP = "0.0.0.0";

		index = getDescriptionValue(index,"m=video",value);
		if (index == -1)
		{
			// does not contain video
			mIpErrorCode = NTV2IpErrSDPNoVideo;
			return false;
		}
		tokens = split(value.c_str(), ' ');
		if ((tokens.size() >= 1) && !tokens[0].empty())
		{
			rxConfig.rx2110Config[i].destPort    = atoi(tokens[0].c_str());
			rxMatch |= RX_MATCH_2110_DEST_PORT;
		}
		if ((tokens.size() >= 3) && !tokens[2].empty())
		{
			rxConfig.rx2110Config[i].payloadType = atoi(tokens[2].c_str());
			rxMatch |= RX_MATCH_2110_PAYLOAD;
		}

		rv = getDescriptionValue(index,"c=IN",value);
		if (rv >= index)
		{
			tokens = split(value.c_str(), ' ');
			if (tokens.size() >= 2)
			{
				tokens = split(tokens[1].c_str(), '/');
				if ((tokens.size() >= 1) && !tokens[0].empty())
				{
					rxConfig.rx2110Config[i].destIP = tokens[0];
					rxMatch |= RX_MATCH_2110_DEST_IP;
				}
			}
		}

        //if there is a source-filter attribute, it overrides the o= source attribute
        rv = getDescriptionValue(index,"a=source-filter:",value);
        if (rv > index)
        {
            tokens = split(value.c_str(), ' ');
            if (tokens.size() >= 5 && !tokens[4].empty())
            {
                rxConfig.rx2110Config[i].sourceIP = tokens[4];
                rxMatch |= RX_MATCH_2110_SOURCE_IP;
            }
        }

		rv = getDescriptionValue(index,"a=rtpmap",value);
		if (rv > index)
		{
			tokens = split(value.c_str(), ' ');
			if ((tokens.size() >= 1) && !tokens[0].empty())
			{
				rxConfig.rx2110Config[i].payloadType = atoi(tokens[0].c_str());
				rxMatch |= RX_MATCH_2110_PAYLOAD;
			}
		}

		rv = getDescriptionValue(index,"a=fmtp",value);
		if (rv > index)
		{
			tokens = split(value.c_str(), ' ');
			string sampling = getVideoDescriptionValue("sampling=");
			if (sampling ==  "YCbCr-4:2:2")
			{
				rxConfig.rx2110Config[i].videoSamples = VPIDSampling_YUV_422;
			}
			string width    = getVideoDescriptionValue("width=");
			string height   = getVideoDescriptionValue("height=");
			string rate     = getVideoDescriptionValue("exactframerate=");
			bool interlace = false;
			bool segmented = false;
			vector<string>::iterator it;

			for (it = tokens.begin(); it != tokens.end(); it++)
			{
				// For interlace, we can get one of the following tokens:
				// interlace
				// interlace;
				// interlace=1
				// interlace segmented

				if (*it == "interlace")
				{
					interlace=true;
					continue;
				}

				if (it->substr(0,10) == "interlace;")
				{
					interlace=true;
					continue;
				}
				if (it->substr(0,11) == "interlace=1")
				{
					interlace=true;
					continue;
				}
				if ((it->substr( 0, 9 ) == "segmented") && interlace)
				{
					interlace=false;
					segmented=true;
					break;
				}
			}
			int w = atoi(width.c_str());
			int h = atoi(height.c_str());
			NTV2FrameRate r = stringToRate(rate);
			NTV2VideoFormat vf = ::GetFirstMatchingVideoFormat(r, h, w, interlace, segmented, false /* no level B */);
			rxConfig.rx2110Config[i].videoFormat = vf;
		}
		rxConfig.rx2110Config[i].rxMatch = rxMatch;
		index++;
	}

	return true;
}


bool CNTV2Config2110::ExtractRxVideoConfigFromSDP(std::string sdp, rx_2110Config & rxConfig)
{
    if (sdp.empty())
    {
        mIpErrorCode = NTV2IpErrSDPEmpty;
        return false;
    }

    // remove any carriage returns
    sdp.erase(remove(sdp.begin(), sdp.end(), '\r'), sdp.end());

    // break into a vector of lines and then into tokenw
    sdpLines.clear();
    stringstream ss(sdp);
    string to;

    while(getline(ss,to,'\n'))
    {
        sdpLines.push_back(to);
    }

    // rudimentary check it is an sdp file
    int index;
    string value;

    // is this really an SDP
    index = getDescriptionValue(0,"v=",value);
    if (index == -1)
    {
        mIpErrorCode = NTV2IpErrSDPInvalid;
        return false;
    }

    // originator
    index = getDescriptionValue(index,"o=",value);
    if (index == -1)
    {
        mIpErrorCode = NTV2IpErrSDPInvalid;
        return false;
    }

	uint32_t rxMatch = 0;

    tokens = split(value.c_str(), ' ');
    if ((tokens.size() >= 6) && (tokens[3] == "IN") && (tokens[4] == "IP4"))
    {
        if (!tokens[5].empty())
        {
            rxConfig.sourceIP = tokens[5];
            rxMatch |= RX_MATCH_2110_SOURCE_IP;
        }
    }

    int rv = getDescriptionValue(0,"c=IN",value);
    if (rv >= index)
    {
        tokens = split(value.c_str(), ' ');
        if (tokens.size() >= 2)
        {
            tokens = split(tokens[1].c_str(), '/');
            if ((tokens.size() >= 1) && !tokens[0].empty())
            {
                rxConfig.destIP = tokens[0];
                rxMatch |= RX_MATCH_2110_DEST_IP;
            }
        }
    }

	index = getDescriptionValue(index,"m=video",value);
	if (index == -1)
	{
		// does not contain video
		mIpErrorCode = NTV2IpErrSDPNoVideo;
		return false;
	}
	tokens = split(value.c_str(), ' ');
	if ((tokens.size() >= 1) && !tokens[0].empty())
	{
		rxConfig.destPort    = atoi(tokens[0].c_str());
		rxMatch |= RX_MATCH_2110_DEST_PORT;
	}
	if ((tokens.size() >= 3) && !tokens[2].empty())
	{
		rxConfig.payloadType = atoi(tokens[2].c_str());
		rxMatch |= RX_MATCH_2110_PAYLOAD;
	}

	rv = getDescriptionValue(index,"c=IN",value);
	if (rv >= index)
	{
		// this overwrites if found before
		tokens = split(value.c_str(), ' ');
		if (tokens.size() >= 2)
		{
			tokens = split(tokens[1].c_str(), '/');
			if ((tokens.size() >= 1) && !tokens[0].empty())
			{
				rxConfig.destIP = tokens[0];
				rxMatch |= RX_MATCH_2110_DEST_IP;
			}
		}
	}

    // if there is a source-filter attribute, it overrides the o= source attribute
    rv = getDescriptionValue(index,"a=source-filter:",value);
    if (rv > index)
    {
		tokens = split(value.c_str(), ' ');
        if (tokens.size() >= 5 && !tokens[4].empty())
        {
            rxConfig.sourceIP = tokens[4];
            rxMatch |= RX_MATCH_2110_SOURCE_IP;
        }
    }

	rv = getDescriptionValue(index,"a=rtpmap",value);
	if (rv > index)
	{
		tokens = split(value.c_str(), ' ');
		if ((tokens.size() >= 1) && !tokens[0].empty())
		{
			rxConfig.payloadType = atoi(tokens[0].c_str());
			rxMatch |= RX_MATCH_2110_PAYLOAD;
		}
	}

	rv = getDescriptionValue(index,"a=fmtp",value);
	if (rv > index)
	{
		tokens = split(value.c_str(), ' ');
		string sampling = getVideoDescriptionValue("sampling=");
		if (sampling ==  "YCbCr-4:2:2")
		{
			rxConfig.videoSamples = VPIDSampling_YUV_422;
		}
		string width    = getVideoDescriptionValue("width=");
		string height   = getVideoDescriptionValue("height=");
		string rate     = getVideoDescriptionValue("exactframerate=");
		bool interlace = false;
		bool segmented = false;
		vector<string>::iterator it;
		for (it = tokens.begin(); it != tokens.end(); it++)
		{
			// For interlace, we can get one of the following tokens:
			// interlace
			// interlace;
			// interlace=1
			// interlace segmented

			if (*it == "interlace")
			{
				interlace=true;
				continue;
			}

			if (it->substr(0,10) == "interlace;")
			{
				interlace=true;
				continue;
			}
			if (it->substr(0,11) == "interlace=1")
			{
				interlace=true;
				continue;
			}
			if ((it->substr( 0, 9 ) == "segmented") && interlace)
			{
				interlace=false;
				segmented=true;
				break;
			}
		}
		int w = atoi(width.c_str());
		int h = atoi(height.c_str());
		NTV2FrameRate r = stringToRate(rate);
		NTV2VideoFormat vf = ::GetFirstMatchingVideoFormat(r, h, w, interlace, segmented, false /* no level B */);
		rxConfig.videoFormat = vf;
	}
	rxConfig.rxMatch = rxMatch;
	return true;
}


bool CNTV2Config2110::ExtractRxAudioConfigFromSDP(std::string sdp, rx_2110Config & rxConfig)
{
	if (sdp.empty())
	{
		mIpErrorCode = NTV2IpErrSDPEmpty;
		return false;
	}

	uint32_t rxMatch = 0;

    // remove any carriage returns
    sdp.erase(remove(sdp.begin(), sdp.end(), '\r'), sdp.end());

	// break into a vector of lines and then into tokenw

	sdpLines.clear();
	stringstream ss(sdp);
	string to;

	while(getline(ss,to,'\n'))
	{
		sdpLines.push_back(to);
	}

	// rudimentary check it is an sdp file
	int index;
	string value;

	// is this really an SDP
	index = getDescriptionValue(0,"v=",value);
	if (index == -1)
	{
		mIpErrorCode = NTV2IpErrSDPInvalid;
		return false;
	}

	// originator
	index = getDescriptionValue(index,"o=",value);
	if (index == -1)
	{
		mIpErrorCode = NTV2IpErrSDPInvalid;
		return false;
	}

	tokens = split(value.c_str(), ' ');
	if ((tokens.size() >= 6) && (tokens[3] == "IN") && (tokens[4] == "IP4"))
	{
		if (!tokens[5].empty())
		{
			rxConfig.sourceIP = tokens[5];
			rxMatch |= RX_MATCH_2110_SOURCE_IP;
		}
	}

	int rv = getDescriptionValue(0,"c=IN",value);
	if (rv >= index)
	{
		tokens = split(value.c_str(), ' ');
		if (tokens.size() >= 2)
		{
			tokens = split(tokens[1].c_str(), '/');
			if ((tokens.size() >= 1) && !tokens[0].empty())
			{
				rxConfig.destIP = tokens[0];
				rxMatch |= RX_MATCH_2110_DEST_IP;
			}
		}
	}

	// audio stream
	index = getDescriptionValue(index,"m=audio",value);
	if (index == -1)
	{
		// does not contain audio
		mIpErrorCode = NTV2IpErrSDPNoAudio;
		return false;
	}

	tokens = split(value.c_str(), ' ');
	if ((tokens.size() >= 1) && !tokens[0].empty())
	{
		rxConfig.destPort    = atoi(tokens[0].c_str());
		rxMatch |= RX_MATCH_2110_DEST_PORT;
	}

	if ((tokens.size() >= 3) && !tokens[2].empty())
	{
		rxConfig.payloadType = atoi(tokens[2].c_str());
		rxMatch |= RX_MATCH_2110_PAYLOAD;
	}

	rv = getDescriptionValue(index,"c=IN",value);
	if (rv >= index)
	{
		// this overwrites if found before
		tokens = split(value.c_str(), ' ');
		if ((tokens.size() >= 2))
		{
			tokens = split(tokens[1].c_str(), '/');
			if ((tokens.size() >= 1)&& !tokens[0].empty())
			{
				rxConfig.destIP = tokens[0];
				rxMatch |= RX_MATCH_2110_DEST_IP;
			}
		}
	}

    // if there is a source-filter attribute, it overrides the o= source attribute
    rv = getDescriptionValue(index,"a=source-filter:",value);
    if (rv > index)
    {
		tokens = split(value.c_str(), ' ');
        if (tokens.size() >= 5 && !tokens[4].empty())
        {
            rxConfig.sourceIP = tokens[4];
            rxMatch |= RX_MATCH_2110_SOURCE_IP;
        }
    }

	rv = getDescriptionValue(index,"a=rtpmap",value);
	if (rv > index)
	{
		tokens = split(value.c_str(), ' ');
		if ((tokens.size() >= 1)&& !tokens[0].empty())
		{
			rxConfig.payloadType = atoi(tokens[0].c_str());
			rxMatch |= RX_MATCH_2110_PAYLOAD;
		}
		if ((tokens.size() >= 2))
		{
			tokens = split(tokens[1].c_str(), '/');
			if ((tokens.size() >= 3) && !tokens[2].empty())
			{
				rxConfig.numAudioChannels = atoi(tokens[2].c_str());
			}
		}
	}

	rv = getDescriptionValue(index,"a=ptime",value);
	if (rv > index)
	{
		tokens = split(value.c_str(), ' ');
		if ((tokens.size() >= 1)&& !tokens[0].empty())
		{
			tokens = split(tokens[0].c_str(), '.');
			if (tokens.size() >= 2)
			{
				if ((atoi(tokens[0].c_str()) == 1) && (atoi(tokens[1].c_str()) == 0))
					rxConfig.audioPktInterval = PACKET_INTERVAL_1mS;
				else if ((atoi(tokens[0].c_str()) == 0) && (atoi(tokens[1].c_str()) == 125))
					rxConfig.audioPktInterval = PACKET_INTERVAL_125uS;
			}
		}
	}

	rxConfig.rxMatch = rxMatch;
	return true;
}


bool CNTV2Config2110::ExtractRxAncConfigFromSDP(std::string sdp, rx_2110Config & rxConfig)
{
	if (sdp.empty())
	{
		mIpErrorCode = NTV2IpErrSDPEmpty;
		return false;
	}

    // remove any carriage returns
    sdp.erase(remove(sdp.begin(), sdp.end(), '\r'), sdp.end());

	// break into a vector of lines and then into tokenw
	sdpLines.clear();
	stringstream ss(sdp);
	string to;

	while(getline(ss,to,'\n'))
	{
		sdpLines.push_back(to);
	}

	// rudimentary check it is an sdp file
	int index;
	string value;

	// is this really an SDP
	index = getDescriptionValue(0,"v=",value);
	if (index == -1)
	{
		mIpErrorCode = NTV2IpErrSDPInvalid;
		return false;
	}

	// originator
	index = getDescriptionValue(index,"o=",value);
	if (index == -1)
	{
		mIpErrorCode = NTV2IpErrSDPInvalid;
		return false;
	}

	uint32_t rxMatch = 0;

	tokens = split(value.c_str(), ' ');
	if ((tokens.size() >= 6) && (tokens[3] == "IN") && (tokens[4] == "IP4"))
	{
		if (!tokens[5].empty())
		{
			rxConfig.sourceIP = tokens[5];
			rxMatch |= RX_MATCH_2110_SOURCE_IP;
		}
	}

	int rv = getDescriptionValue(0,"c=IN",value);
	if (rv >= index)
	{
		tokens = split(value.c_str(), ' ');
		if (tokens.size() >= 2)
		{
			tokens = split(tokens[1].c_str(), '/');
			if ((tokens.size() >= 1) && !tokens[0].empty())
			{
				rxConfig.destIP = tokens[0];
				rxMatch |= RX_MATCH_2110_DEST_IP;
			}
		}
	}

	index = getDescriptionValue(index,"m=video",value);
	if (index == -1)
	{
		// does not contain video
		mIpErrorCode = NTV2IpErrSDPNoVideo;
		return false;
	}
	tokens = split(value.c_str(), ' ');
	if ((tokens.size() >= 1) && !tokens[0].empty())
	{
		rxConfig.destPort    = atoi(tokens[0].c_str());
		rxMatch |= RX_MATCH_2110_DEST_PORT;
	}
	if ((tokens.size() >= 3) && !tokens[2].empty())
	{
		rxConfig.payloadType = atoi(tokens[2].c_str());
		rxMatch |= RX_MATCH_2110_PAYLOAD;
	}

	rv = getDescriptionValue(index,"c=IN",value);
	if (rv >= index)
	{
		// this overwrites if found before
		tokens = split(value.c_str(), ' ');
		if (tokens.size() >= 2)
		{
			tokens = split(tokens[1].c_str(), '/');
			if ((tokens.size() >= 1) && !tokens[0].empty())
			{
				rxConfig.destIP = tokens[0];
				rxMatch |= RX_MATCH_2110_DEST_IP;
			}
		}
	}

    // if there is a source-filter attribute, it overrides the o= source attribute
    rv = getDescriptionValue(index,"a=source-filter:",value);
    if (rv > index)
    {
		tokens = split(value.c_str(), ' ');
        if (tokens.size() >= 5 && !tokens[4].empty())
        {
            rxConfig.sourceIP = tokens[4];
            rxMatch |= RX_MATCH_2110_SOURCE_IP;
        }
    }

	rv = getDescriptionValue(index,"a=rtpmap",value);
	if (rv > index)
	{
		tokens = split(value.c_str(), ' ');
		if ((tokens.size() >= 1) && !tokens[0].empty())
		{
			rxConfig.payloadType = atoi(tokens[0].c_str());
			rxMatch |= RX_MATCH_2110_PAYLOAD;
		}
	}

	rxConfig.rxMatch = rxMatch;
	return true;
}


std::string CNTV2Config2110::rateToString(NTV2FrameRate rate)
{
    string rateString;
    switch (rate)
    {
     default:
     case   NTV2_FRAMERATE_UNKNOWN  :
        rateString = "00";
        break;
     case   NTV2_FRAMERATE_6000     :
        rateString = "60";
        break;
     case   NTV2_FRAMERATE_5994     :
        rateString = "60000/1001";
        break;
     case   NTV2_FRAMERATE_3000     :
        rateString = "30";
        break;
     case   NTV2_FRAMERATE_2997     :
        rateString = "30000/1001";
        break;
     case   NTV2_FRAMERATE_2500     :
        rateString = "25";
        break;
     case   NTV2_FRAMERATE_2400     :
        rateString = "24";
        break;
     case   NTV2_FRAMERATE_2398     :
        rateString = "24000/1001";
        break;
     case   NTV2_FRAMERATE_5000     :
        rateString = "50";
        break;
     case   NTV2_FRAMERATE_4800     :
        rateString = "48";
        break;
     case   NTV2_FRAMERATE_4795     :
        rateString = "48000/1001";
        break;
     case   NTV2_FRAMERATE_12000    :
		rateString = "120";
        break;
     case   NTV2_FRAMERATE_11988    :
		rateString = "120000/1001";
        break;
     case   NTV2_FRAMERATE_1500     :
        rateString = "15";
        break;
     case   NTV2_FRAMERATE_1498     :
        rateString = "1500/1001";
        break;
    }
    return rateString;
}

NTV2FrameRate CNTV2Config2110::stringToRate(std::string rateString)
{
    NTV2FrameRate rate;
    if (rateString == "60")
        rate = NTV2_FRAMERATE_6000;
    else if (rateString == "60000/1001")
        rate = NTV2_FRAMERATE_5994;
    else if (rateString == "30")
        rate = NTV2_FRAMERATE_3000;
    else if (rateString == "30000/1001")
        rate = NTV2_FRAMERATE_2997;
    else if (rateString == "25")
        rate = NTV2_FRAMERATE_2500;
    else if (rateString == "24")
        rate = NTV2_FRAMERATE_2400;
    else if (rateString == "24000/1001")
        rate = NTV2_FRAMERATE_2398;
    else if (rateString == "50")
        rate = NTV2_FRAMERATE_5000;
    else if (rateString == "48")
        rate = NTV2_FRAMERATE_4800;
    else if (rateString == "48000/1001")
        rate = NTV2_FRAMERATE_4795;
	else if (rateString == "120")
        rate = NTV2_FRAMERATE_12000;
	else if (rateString == "120000/1001")
        rate = NTV2_FRAMERATE_11988;
    else if (rateString == "15")
        rate = NTV2_FRAMERATE_1500;
    else if (rateString == "1500/1001")
        rate = NTV2_FRAMERATE_1498;
    else
        rate = NTV2_FRAMERATE_UNKNOWN;
    return rate;
}

void CNTV2Config2110::SetArbiter(const eSFP sfp, const NTV2Stream stream, bool enable)
{
    uint32_t reg;
    if (StreamType(stream) == VIDEO_STREAM)
    {
        reg = kRegArb_video + SAREK_2110_TX_ARBITRATOR;
    }
    else
    {
        reg = kRegArb_audio +  SAREK_2110_TX_ARBITRATOR;
    }
    uint32_t val;
    mDevice.ReadRegister(reg, val);

    uint32_t bit = (1 << Get2110TxStreamIndex(stream)) << (int(sfp) * 16);
	if ((GetSampling(sfp, stream) == VPIDSampling_GBR_444) &&
		((mDevice.GetDeviceID() == DEVICE_ID_KONAIP_2110_RGB12) ||
		 (mDevice.GetDeviceID() == DEVICE_ID_IOIP_2110_RGB12)))
		bit = bit << 4;

    if (enable)
        val |= bit;
    else
        val &= ~bit;

    mDevice.WriteRegister(reg, val);
}

void CNTV2Config2110::GetArbiter(const eSFP sfp, NTV2Stream stream, bool & enable)
{
    uint32_t reg;
    if (StreamType(stream) == VIDEO_STREAM)
    {
        reg = kRegArb_video + SAREK_2110_TX_ARBITRATOR;
    }
    else
    {
        reg = kRegArb_audio + SAREK_2110_TX_ARBITRATOR;
    }
    uint32_t val;
    mDevice.ReadRegister(reg, val);

    uint32_t bit = (1 << Get2110TxStreamIndex(stream)) << (int(sfp) * 16);
	if ((GetSampling(sfp, stream) == VPIDSampling_GBR_444) &&
		((mDevice.GetDeviceID() == DEVICE_ID_KONAIP_2110_RGB12) ||
		 (mDevice.GetDeviceID() == DEVICE_ID_IOIP_2110_RGB12)))
		bit = bit << 4;

    enable = (val & bit);
}

void CNTV2Config2110::SetSampling(const eSFP sfp, const NTV2Stream stream, const VPIDSampling sampling)
{
	if (StreamType(stream) == VIDEO_STREAM)
	{
		uint32_t samp = sampling;
		uint32_t mask = 0;

		switch (stream)
		{
			case NTV2_VIDEO4_STREAM:
				mask = 0xffff0fff;
				samp = samp << 12;
				break;

			case NTV2_VIDEO3_STREAM:
				mask = 0xfffff0ff;
				samp = samp << 8;
				break;

			case NTV2_VIDEO2_STREAM:
				mask = 0xffffff0f;
				samp = samp << 4;
				break;

			default:
			case NTV2_VIDEO1_STREAM:
				mask = 0xfffffff0;
				break;
		}

		if (sfp == SFP_2)
		{
			mask = mask << 16;
			mask |= 0xffff;
			samp = samp << 16;
		}

		uint32_t val;
		mDevice.ReadRegister(SAREK_REGS + kRegSarekSampling, val);
		val &= mask;
		val |= samp;
		mDevice.WriteRegister(SAREK_REGS + kRegSarekSampling, val);
	}
}

VPIDSampling CNTV2Config2110::GetSampling(const eSFP sfp, const NTV2Stream stream)
{
	VPIDSampling sampling = VPIDSampling_YUV_422;

	if (StreamType(stream) == VIDEO_STREAM)
	{
		uint32_t val;
		mDevice.ReadRegister(SAREK_REGS + kRegSarekSampling, val);

		if (sfp == SFP_2)
			val = val >> 16;

		switch (stream)
		{
			case NTV2_VIDEO4_STREAM:
				val = val >> 12;
				break;

			case NTV2_VIDEO3_STREAM:
				val = val >> 8;
				break;

			case NTV2_VIDEO2_STREAM:
				val = val >> 4;
				break;

			default:
			case NTV2_VIDEO1_STREAM:
				break;

		}
		sampling =	(VPIDSampling)(val &= 0x0000000f);
	}

	return sampling;
}

bool CNTV2Config2110::SetLLDPInfo(std::string sysname)
{
    return CNTV2MBController::SetLLDPInfo(sysname);
}

bool CNTV2Config2110::GetLLDPInfo(std::string &chassisId0, std::string &portId0,
					std::string &chassisId1, std::string &portId1)
{
	return CNTV2MBController::GetLLDPInfo(chassisId0, portId0, chassisId1, portId1);
}

uint64_t CNTV2Config2110::GetNTPTimestamp()
{
	return CNTV2MBController::GetNTPTimestamp();
}

