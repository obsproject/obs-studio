/* SPDX-License-Identifier: MIT */
/**
    @file		ntv2config2022.h
    @brief		Declares the CNTV2Config2022 class.
    @copyright	(C) 2014-2021 AJA Video Systems, Inc.
**/

#ifndef NTV2_2022CONFIG_H
#define NTV2_2022CONFIG_H

#include "ntv2card.h"
#include "ntv2enums.h"
#include "ntv2registers2022.h"
#include "ntv2mbcontroller.h"
#include "ntv2tshelper.h"
#include <string.h>

/**
    @brief	Configures a SMPTE 2022 Transmit Channel.
**/

class AJAExport tx_2022_channel
{
public:
    tx_2022_channel() { init(); }

    void init();

    bool operator != ( const tx_2022_channel &other );
    bool operator == ( const tx_2022_channel &other );

public:
    bool        sfp1Enable;
    bool        sfp2Enable;

    uint32_t	sfp1LocalPort;		///< @brief	Specifies the local (source) port number.
    std::string	sfp1RemoteIP;       ///< @brief	Specifies remote (destination) IP address.
    uint32_t	sfp1RemotePort;     ///< @brief	Specifies the remote (destination) port number.

    uint32_t	sfp2LocalPort;		///< @brief	Specifies the local (source) port number.
    std::string	sfp2RemoteIP;       ///< @brief	Specifies remote (destination) IP address.
    uint32_t	sfp2RemotePort;     ///< @brief	Specifies the remote (destination) port number.

    uint8_t     tos;            // type of service
    uint8_t     ttl;            // time to live
    uint32_t    ssrc;
};

/**
    @brief	Configures a SMPTE 2022 Receive Channel.
**/

class AJAExport rx_2022_channel
{
public:
    rx_2022_channel() { init(); }

    void init();

    bool operator != ( const rx_2022_channel &other );
    bool operator == ( const rx_2022_channel &other );

public:
    bool        sfp1Enable;
    bool        sfp2Enable;

    uint8_t	    sfp1RxMatch;            ///< @brief	Bitmap of rxMatch criteria used
    std::string	sfp1SourceIP;           ///< @brief	Specifies the source (sender) IP address (if RX_MATCH_2022_SOURCE_IP set). If it's in the multiclass range, then
                                        ///			by default, the IGMP multicast group will be joined (see CNTV2Config2022::SetIGMPDisable).
    std::string	sfp1DestIP;             ///< @brief	Specifies the destination (target) IP address (if RX_MATCH_2022_DEST_IP set)
    uint32_t	sfp1SourcePort;         ///< @brief	Specifies the source (sender) port number (if RX_MATCH_2022_SOURCE_PORT set)
    uint32_t	sfp1DestPort;           ///< @brief	Specifies the destination (target) port number (if RX_MATCH_2022_DEST_PORT set)
    uint16_t	sfp1Vlan;               ///< @brief	Specifies the VLAN TCI (if RX_MATCH_2022_VLAN set)

    uint8_t	    sfp2RxMatch;            ///< @brief	Bitmap of rxMatch criteria used
    std::string	sfp2SourceIP;           ///< @brief	Specifies the source (sender) IP address (if RX_MATCH_2022_SOURCE_IP set). If it's in the multiclass range, then
                                        ///			by default, the IGMP multicast group will be joined (see CNTV2Config2022::SetIGMPDisable).
    std::string	sfp2DestIP;             ///< @brief	Specifies the destination (target) IP address (if RX_MATCH_2022_DEST_IP set)
    uint32_t	sfp2SourcePort;         ///< @brief	Specifies the source (sender) port number (if RX_MATCH_2022_SOURCE_PORT set)
    uint32_t	sfp2DestPort;           ///< @brief	Specifies the destination (target) port number (if RX_MATCH_2022_DEST_PORT set)
    uint16_t	sfp2Vlan;               ///< @brief	Specifies the VLAN TCI (if RX_MATCH_2022_VLAN set)

    uint32_t    ssrc;                   ///< @brief	Specifies the SSRC identifier (if RX_MATCH_2022_SSRC set)
    uint32_t	playoutDelay;           ///< @brief	Specifies the wait time in milliseconds to SDI playout from incoming packet (0-150).
};

class AJAExport j2kEncoderConfig
{
public:
    j2kEncoderConfig() { init(); }

    void init();

    bool operator == ( const j2kEncoderConfig &other );
    bool operator != ( const j2kEncoderConfig &other );

    NTV2VideoFormat         videoFormat;        ///< @brief	Specifies the video format for J2K encode.
    uint32_t                ullMode;            ///< @brief	Specifies the ull mode for J2K encode.
    uint32_t                bitDepth;           ///< @brief	Specifies the bit depth for J2K encode.
    J2KChromaSubSampling    chromaSubsamp;      ///< @brief	Specifies the chroma sub sampling for J2K encode.
    uint32_t                mbps;               ///< @brief	Specifies the mbits per-second for J2K encode.
    J2KStreamType           streamType;         ///< @brief	Specifies the stream type for J2K encode.
    uint32_t                audioChannels;      ///< @brief	Specifies the number of audio channels for J2K encode, a value of 0 indicates no audio.
    uint32_t                pmtPid;             ///< @brief	Specifies the PID for the PMT.
    uint32_t                videoPid;           ///< @brief	Specifies the PID for the video.
    uint32_t                pcrPid;             ///< @brief	Specifies the PID for the PCR.
    uint32_t                audio1Pid;          ///< @brief	Specifies the PID for audio 1.
};

class AJAExport j2kDecoderConfig
{
public:
    typedef enum
    {
        eProgSel_Off,
        eProgSel_AutoFirstProg,
        eProgSel_LowestProgNum,
        eProgSel_SpecificProgNum,
        eProgSel_SpecificProgPID,
        eProgSel_Default = eProgSel_AutoFirstProg
    } eProgSelMode_t;

    j2kDecoderConfig() {init();}
    void init();

    bool operator == ( const j2kDecoderConfig &other );
    bool operator != ( const j2kDecoderConfig &other );

    eProgSelMode_t  selectionMode;
    uint32_t        programNumber;
    uint32_t        programPID;
    uint32_t        audioNumber;
};

class AJAExport j2kDecoderStatus
{
public:
    j2kDecoderStatus() {init();}
    void init();

    uint32_t              numAvailablePrograms;
    uint32_t              numAvailableAudios;
    std::vector<uint32_t> availableProgramNumbers;
    std::vector<uint32_t> availableProgramPIDs;
    std::vector<uint32_t> availableAudioPIDs;
};

struct s2022RxChannelStatus
{
    uint32_t sfp1RxPackets;
    uint32_t sfp1ValidRxPackets;
    uint32_t sfp2RxPackets;
    uint32_t sfp2ValidRxPackets;
};

/**
    @brief	The CNTV2Config2022 class is the interface to Kona-IP network I/O using SMPTE 2022
**/
class AJAExport CNTV2Config2022 : public CNTV2MBController
{
public:
    CNTV2Config2022 (CNTV2Card & device);
    ~CNTV2Config2022();

    bool        SetNetworkConfiguration(const eSFP sfp, const IPVNetConfig & netConfig);
    bool        GetNetworkConfiguration(const eSFP sfp, IPVNetConfig & netConfig);
    bool        SetNetworkConfiguration(const eSFP sfp, const std::string localIPAddress, const std::string subnetMask, const std::string gateway);
    bool        GetNetworkConfiguration(const eSFP sfp, std::string & localIPAddress, std::string & subnetMask, std::string & gateway);
    bool        DisableNetworkInterface(const eSFP sfp);

    bool        SetRxChannelConfiguration(const NTV2Channel channel, const rx_2022_channel & rxConfig);
    bool        GetRxChannelConfiguration(const NTV2Channel channel, rx_2022_channel & rxConfig);

    bool        SetRxChannelEnable(const NTV2Channel channel, bool enable);
    bool        GetRxChannelEnable(const NTV2Channel channel, bool & enabled);

    bool        SetTxChannelConfiguration(const NTV2Channel channel, const tx_2022_channel & txConfig);
    bool        GetTxChannelConfiguration(const NTV2Channel channel, tx_2022_channel & txConfig);

    bool        SetTxChannelEnable(const NTV2Channel channel, bool enable);
    bool        GetTxChannelEnable(const NTV2Channel channel, bool & enabled);

    bool        SetJ2KEncoderConfiguration(const NTV2Channel channel, const j2kEncoderConfig & j2kConfig);
    bool        GetJ2KEncoderConfiguration(const NTV2Channel channel, j2kEncoderConfig &j2kConfig);

    bool        SetJ2KDecoderConfiguration(const j2kDecoderConfig & j2kConfig);
    bool        GetJ2KDecoderConfiguration(j2kDecoderConfig &j2kConfig);
    bool        GetJ2KDecoderStatus(j2kDecoderStatus & j2kStatus);

    bool        Set2022_7_Mode(bool enable, uint32_t rx_networkPathDifferential);
    bool        Get2022_7_Mode(bool & enable, uint32_t & rx_networkPathDifferential);

    /**
        @brief		Disables the automatic (default) joining of multicast groups using IGMP, based on remote IP address for Rx Channels
        @param[in]	sfp                 Specifies SFP connector used.
        @param[in]	disable             If true, IGMP messages will not be sent to join multicast groups
        @note       When Rx channels are enabled for multicast IP addresses, by default the multicast group is joined. When Rx Channels
                    are disabled, if the channel is the last user of the group, then the subscription to the multicast group will be ended.
                    When IGMP is disabled, the above actions are not performed,
    **/
    bool        SetIGMPDisable(eSFP sfp, bool disable);
    bool        GetIGMPDisable(eSFP sfp, bool & disabled);

    bool        SetIGMPVersion(eIGMPVersion_t version);
    bool        GetIGMPVersion(eIGMPVersion_t & version);

    void        SetBiDirectionalChannels(bool bidirectional) { _biDirectionalChannels = bidirectional;}
    bool        GetBiDirectionalChannels() {return _biDirectionalChannels;}

    bool        GetMACAddress(eSFP sfp, NTV2Stream stream, std::string remoteIP, uint32_t & hi, uint32_t & lo);

    bool        GetSFPMSAData(eSFP sfp, SFPMSAData & data);
    bool        GetLinkStatus(eSFP sfp, SFPStatus & sfpStatus);
    bool        Get2022ChannelRxStatus(NTV2Channel channel, s2022RxChannelStatus & status);

    bool        SetIPServicesControl(const bool enable, const bool forceReconfig);
    bool        GetIPServicesControl(bool & enable, bool & forceReconfig);


    // If method returns false call this to get details
    std::string getLastError();
    NTV2IpError getLastErrorCode();

private:
    void        ChannelSemaphoreSet(uint32_t controlReg, uint32_t baseaddr);
    void        ChannelSemaphoreClear(uint32_t controlReg, uint32_t baseaddr);

    bool        SelectRxChannel(NTV2Channel channel, eSFP sfp, uint32_t & baseAddr);
    bool        SelectTxChannel(NTV2Channel channel, eSFP sfp, uint32_t & baseAddr);

    NTV2Stream VideoChannelToStream(const NTV2Channel channel);
    NTV2Channel VideoStreamToChannel(const NTV2Stream stream);

    class CNTV2ConfigTs2022 * _tstreamConfig;

    eSFP        GetTxLink(NTV2Channel chan);

    uint32_t    _numRx0Chans;
    uint32_t    _numRx1Chans;
    uint32_t    _numTx0Chans;
    uint32_t    _numTx1Chans;
    uint32_t    _numRxChans;
    uint32_t    _numTxChans;
    bool        _is2022_6;
    bool        _is2022_2;
    bool        _is2022_7;
    bool        _biDirectionalChannels;             // logically bi-directional channels
    bool        _is_txTop34;
    bool        _isIoIp;
};	//	CNTV2Config2022

#endif // NTV2_2022CONFIG_H
