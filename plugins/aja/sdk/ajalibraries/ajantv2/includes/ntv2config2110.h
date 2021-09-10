/* SPDX-License-Identifier: MIT */
/**
    @file		ntv2config2110.h
    @brief		Declares the CNTV2Config2110 class.
    @copyright	(C) 2014-2021 AJA Video Systems, Inc.
**/

#ifndef NTV2_2110CONFIG_H
#define NTV2_2110CONFIG_H

#include "ntv2card.h"
#include "ntv2enums.h"
#include "ntv2registers2110.h"
#include "ntv2mbcontroller.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

/**
    @brief	Structs and enums that hold the virtual config data used by services the ControlPanel application and JSON parsers
**/

#define RX_USE_SFP_IP		BIT(31)
#define IP_STRSIZE			32


typedef enum
{
	kNetworkData2110		= NTV2_FOURCC('n','t','1','3'), // 4CC of network config data
	kTransmitVideoData2110	= NTV2_FOURCC('t','v','1','3'), // 4CC of video transmit config data
	kTransmitAudioData2110	= NTV2_FOURCC('t','a','1','3'), // 4CC of audio transmit config data
	kTransmitAncData2110	= NTV2_FOURCC('t','n','1','3'), // 4CC of anc transmit config data
	kReceiveVideoData2110	= NTV2_FOURCC('r','v','1','3'), // 4CC of video receive config data
	kReceiveAudioData2110	= NTV2_FOURCC('r','a','1','3'), // 4CC of audio receive config data
	kReceiveAncData2110		= NTV2_FOURCC('r','n','1','3'), // 4CC of anc receive config data
	kChStatusData2110		= NTV2_FOURCC('s','t','1','3')  // 4CC of channel status config data
} VirtualDataTag2110;

typedef enum
{
	kIpStatusFail			= 0,
	kIpStatusStopped		= 1,
	kIpStatusRunning		= 2
} IpChStatusState;

// force 1 byte alignment so can work across 32/64 bit apps
#pragma pack(push,1)

typedef struct
{
	NTV2Stream				stream;
	char					remoteIP[2][IP_STRSIZE];
	uint32_t				remotePort[2];
	uint32_t				localPort[2];
	uint32_t				sfpEnable[2];
	uint32_t				ttl;
	uint32_t				ssrc;
	uint32_t				payloadType;
	NTV2VideoFormat			videoFormat;
	VPIDSampling			sampling;
	uint32_t				enable;
	uint8_t					unused[12];
} TxVideoChData2110;

typedef struct
{
	NTV2Stream				stream;
	NTV2Channel				channel;
	char					remoteIP[2][IP_STRSIZE];
	uint32_t				localPort[2];
	uint32_t				remotePort[2];
	uint32_t				sfpEnable[2];
	uint32_t				ttl;
	uint32_t				ssrc;
	uint32_t				payloadType;
	uint32_t				numAudioChannels;
	uint32_t				firstAudioChannel;
	eNTV2PacketInterval		audioPktInterval;
	uint32_t				enable;
	uint8_t					unused[16];
} TxAudioChData2110;

typedef struct
{
	NTV2Stream				stream;
	char					remoteIP[2][IP_STRSIZE];
	uint32_t				localPort[2];
	uint32_t				remotePort[2];
	uint32_t				sfpEnable[2];
	uint32_t				ttl;
	uint32_t				ssrc;
	uint32_t				payloadType;
	uint32_t				enable;
	uint8_t					unused[16];
} TxAncChData2110;

typedef struct
{
	NTV2Stream				stream;
	char					sourceIP[2][IP_STRSIZE];
	char					destIP[2][IP_STRSIZE];
	uint32_t				sourcePort[2];
	uint32_t				destPort[2];
	uint32_t				sfpEnable[2];
	uint32_t				vlan;
	uint32_t				ssrc;
	uint32_t				payloadType;
	NTV2VideoFormat			videoFormat;
	uint32_t				enable;
	uint8_t					unused[16];
} RxVideoChData2110;

typedef struct
{
	NTV2Stream				stream;
	NTV2Channel				channel;
	char					sourceIP[2][IP_STRSIZE];
	char					destIP[2][IP_STRSIZE];
	uint32_t				sourcePort[2];
	uint32_t				destPort[2];
	uint32_t				sfpEnable[2];
	uint32_t				vlan;
	uint32_t				ssrc;
	uint32_t				payloadType;
	uint32_t				numAudioChannels;
	eNTV2PacketInterval		audioPktInterval;
	uint32_t				enable;
	uint8_t					unused[16];
} RxAudioChData2110;

typedef struct
{
	NTV2Stream				stream;
	char					sourceIP[2][IP_STRSIZE];
	char					destIP[2][IP_STRSIZE];
	uint32_t				sourcePort[2];
	uint32_t				destPort[2];
	uint32_t				sfpEnable[2];
	uint32_t				vlan;
	uint32_t				ssrc;
	uint32_t				payloadType;
	uint32_t				enable;
	uint8_t					unused[16];
} RxAncChData2110;

typedef struct
{
	eSFP					sfp;
	char					ipAddress[IP_STRSIZE];
	char					subnetMask[IP_STRSIZE];
	char					gateWay[IP_STRSIZE];
	uint32_t				enable;
	uint8_t					unused[16];
} SFPData2110;

typedef struct
{
	uint32_t				txChStatus[4];
	uint32_t				rxChStatus[4];
	uint8_t					unused[16];
} IpStatus2110;

typedef struct
{
	bool                    setup4k;
	uint32_t                ptpDomain;
	uint8_t                 ptpPreferredGMID[8];
	uint32_t                numSFPs;
	SFPData2110             sfp[2];
	bool					multiSDP;
	bool					audioCombine;
	uint32_t                rxMatchOverride;
	uint8_t					unused[10];
} NetworkData2110;

typedef struct
{
    uint32_t                numTxVideoChannels;
    TxVideoChData2110       txVideoCh[4];
} TransmitVideoData2110;

typedef struct
{
    uint32_t                numTxAudioChannels;
    TxAudioChData2110       txAudioCh[4];
} TransmitAudioData2110;

typedef struct
{
	uint32_t                numTxAncChannels;
	TxAncChData2110			txAncCh[4];
} TransmitAncData2110;

typedef struct
{
    uint32_t                numRxVideoChannels;
    RxVideoChData2110       rxVideoCh[4];
} ReceiveVideoData2110;

typedef struct
{
    uint32_t                numRxAudioChannels;
    RxAudioChData2110       rxAudioCh[4];
} ReceiveAudioData2110;

typedef struct
{
	uint32_t                numRxAncChannels;
	RxAncChData2110			rxAncCh[4];
} ReceiveAncData2110;

#pragma pack(pop)

inline NTV2Stream ChToVideoStream(int ch)
	{ return (NTV2Stream)(NTV2_VIDEO1_STREAM+ch); }
inline int VideoStreamToCh(NTV2Stream s)
	{ return (int)(s); }
	
inline NTV2Stream ChToAudioStream(int ch)
	{ return (NTV2Stream)(NTV2_AUDIO1_STREAM+ch); }
inline int AudioStreamToCh(NTV2Stream s)
	{ return (int)(s >= NTV2_AUDIO1_STREAM ? s-NTV2_AUDIO1_STREAM : 0); }

inline NTV2Stream ChToAncStream(int ch)
	{ return (NTV2Stream)(NTV2_ANC1_STREAM+ch); }
inline int AncStreamToCh(NTV2Stream s)
	{ return (int)(s >= NTV2_ANC1_STREAM ? s-NTV2_ANC1_STREAM : 0); }

/**
    @brief	Configures a SMPTE 2110 Transmit Channel.
**/

class AJAExport tx_2110Config
{
	public:
		tx_2110Config() { init(); }

		void init();

		bool operator != ( const tx_2110Config &other );
		bool operator == ( const tx_2110Config &other );

	public:
		std::string         remoteIP[2];        ///< @brief	Specifies remote (destination) IP address.
		uint32_t            localPort[2];		///< @brief	Specifies the local (source) port number.
		uint32_t            remotePort[2];		///< @brief	Specifies the remote (destination) port number.
		uint16_t            payloadType;
		uint8_t             tos;                // type of service
		uint8_t             ttl;                // time to live
		uint32_t            ssrc;
		NTV2Channel         channel;
		NTV2VideoFormat     videoFormat;
		VPIDSampling        videoSamples;
		uint8_t             numAudioChannels;
		uint8_t             firstAudioChannel;
		eNTV2PacketInterval audioPktInterval;
};

/**
    @brief	Configures a SMPTE 2110 Receive Channel.
**/

class AJAExport rx_2110Config
{
	public:
		rx_2110Config() { init(); }

		void init();

		bool operator != ( const rx_2110Config &other );
		bool operator == ( const rx_2110Config &other );

	public:
		uint32_t            rxMatch;            ///< @brief	Bitmap of rxMatch criteria used
		std::string         sourceIP;           ///< @brief	Specifies the source (sender) IP address (if RX_MATCH_2110_SOURCE_IP set). If it's in the multiclass range, then
												///			by default, the IGMP multicast group will be joined (see CNTV2Config2110::SetIGMPDisable).
		std::string         destIP;             ///< @brief	Specifies the destination (target) IP address (if RX_MATCH_2110_DEST_IP set)
		uint32_t            sourcePort;         ///< @brief	Specifies the source (sender) port number (if RX_MATCH_2110_SOURCE_PORT set)
		uint32_t            destPort;           ///< @brief	Specifies the destination (target) port number (if RX_MATCH_2110_DEST_PORT set)
		uint32_t            ssrc;               ///< @brief	Specifies the SSRC identifier (if RX_MATCH_2110_SSRC set)
		uint16_t            vlan;               ///< @brief	Specifies the VLAN TCI (if RX_MATCH_2110_VLAN set)
		uint16_t            payloadType;
		NTV2VideoFormat     videoFormat;
		VPIDSampling        videoSamples;
		uint32_t            numAudioChannels;
		eNTV2PacketInterval audioPktInterval;
};

typedef struct
{
	rx_2110Config		rx2110Config[4];
} multiRx_2110Config;


/**
	@brief	The CNTV2Config2110 class inquires and configures SMPTE 2110 network I/O for \ref konaip and \ref ioip.
**/

class AJAExport CNTV2Config2110 : public CNTV2MBController
{
	friend class CKonaIpJsonSetup;
public:
	CNTV2Config2110 (CNTV2Card & device);
	~CNTV2Config2110();

	bool		SetNetworkConfiguration (const eSFP sfp, const IPVNetConfig & netConfig);
	bool		GetNetworkConfiguration (const eSFP sfp, IPVNetConfig & netConfig);
	bool		SetNetworkConfiguration (const eSFP sfp, const std::string localIPAddress, const std::string subnetMask, const std::string gateway);
	bool		GetNetworkConfiguration (const eSFP sfp, std::string & localIPAddress, std::string & subnetMask, std::string & gateway);
	bool		DisableNetworkInterface (const eSFP sfp);
	bool		SetRxStreamConfiguration (const eSFP sfp, const NTV2Stream stream, const rx_2110Config & rxConfig);
	bool		GetRxStreamConfiguration (const eSFP sfp, const NTV2Stream stream, rx_2110Config & rxConfig);
	bool		SetRxStreamEnable (const eSFP sfp, const NTV2Stream stream, bool enable);
	bool		GetRxStreamEnable (const eSFP sfp, const NTV2Stream stream, bool & enabled);
	bool		GetRxPacketCount (const NTV2Stream stream, uint32_t &packets);
	bool		GetRxByteCount (const NTV2Stream stream, uint32_t &bytes);
	bool		GetRxByteCount (const eSFP sfp, uint64_t &bytes);

	bool		SetTxStreamConfiguration (const NTV2Stream stream, const tx_2110Config & txConfig);
	bool		GetTxStreamConfiguration (const NTV2Stream stream, tx_2110Config & txConfig);
	bool		SetTxStreamEnable (const NTV2Stream stream, bool enableSfp1, bool enableSfp2 = false);
	bool		GetTxStreamEnable (const NTV2Stream stream, bool & sfp1Enabled, bool & sfp2Enabled);
	bool		GetTxPacketCount (NTV2Stream stream, uint32_t &packets);
	bool		GetTxByteCount (const eSFP sfp, uint64_t &bytes);

	bool		SetPTPDomain (const uint8_t domain);
	bool		GetPTPDomain (uint8_t &domain);
	bool		SetPTPPreferredGrandMasterId (const uint8_t id[8]);
	bool		GetPTPPreferredGrandMasterId (uint8_t (&id)[8]);
	bool		GetPTPStatus (PTPStatus & ptpStatus);

	bool		Set4KModeEnable (const bool enable);
	bool		Get4KModeEnable (bool & enable);

	/**
		@brief		Enables or disables the audio combiner.
		@param[in]	enable	Specify true to enable the audio combiner;  specify false to disable it.
		@return		True if successful;  otherwise false.
		@note		This only affects 4K RX mode.
		@note		This parameter can be set in the \ref ntv2konaipjsonsetup. Both JSON examples specify it in the "network2110" area.
		@detail		Normally the audio combiner is disabled when transmitting 16 audio channels over a single flow.
					However, it is possible to divide the audio into multiple flows of 4 channels each, so in this case, on the RX side,
					you would enable the audio combiner, so that it combines all the separate flows into a single 16-channel audio buffer.
		@see		CNTV2Config2110::GetAudioCombineEnable
	**/
	bool		SetAudioCombineEnable (const bool enable);

	/**
		@brief		Answers with the enable/disable state of the audio combiner.
		@param[in]	enable	Receives true if the audio combiner is enabled;  otherwise false if it's disabled.
		@return		True if successful;  otherwise false.
		@note		This only affects 4K RX mode.
		@detail		Normally the audio combiner is off when transmitting 16 audio channels over a single flow.
					However, it is possible to divide the audio into multiple flows of 4 channels each, so in this case, on the RX side,
					you would enable the audio combiner so, that it combines all the separate flows into a single 16-channel audio buffer.
		@see		CNTV2Config2110::SetAudioCombineEnable
	**/
	bool		GetAudioCombineEnable (bool & enable);

	bool		SetIPServicesControl (const bool enable, const bool forceReconfig);
	bool		GetIPServicesControl (bool & enable, bool & forceReconfig);

	std::string GetSDPUrl (const eSFP sfp, const NTV2Stream stream);
	std::string GetGeneratedSDP (bool enabledSfp1, bool enabledSfp2, const NTV2Stream stream);
	bool		GetActualSDP (std::string url, std::string & sdp);
	bool		ExtractRxVideoConfigFromSDP (std::string sdp, rx_2110Config & rxConfig);
	bool		ExtractRxVideoConfigFromSDP (std::string sdp, multiRx_2110Config & rxConfig);
	bool		ExtractRxAudioConfigFromSDP (std::string sdp, rx_2110Config & rxConfig);
	bool		ExtractRxAncConfigFromSDP (std::string sdp, rx_2110Config & rxConfig);

	/**
		@brief		Disables the automatic (default) joining of multicast groups using IGMP, based on remote IP address for Rx Channels
		@param[in]	sfp			Specifies SFP connector used.
		@param[in]	disable		If true, IGMP messages will not be sent to join multicast groups
		@note       When Rx channels are enabled for multicast IP addresses, by default the multicast group is joined. When Rx Channels
					are disabled, if the channel is the last user of the group, then the subscription to the multicast group will be ended.
					When IGMP is disabled, the above actions are not performed,
	**/
	bool		SetIGMPDisable (const eSFP sfp, const bool disable);
	bool		GetIGMPDisable (const eSFP sfp, bool & disabled);

	bool		SetIGMPVersion (const eIGMPVersion_t version);
	bool		GetIGMPVersion (eIGMPVersion_t & version);

	void		SetBiDirectionalChannels (const bool bidirectional)	{ _biDirectionalChannels = bidirectional;}
	bool		GetBiDirectionalChannels (void)						{return _biDirectionalChannels;}

	bool		GetMACAddress (const eSFP port, const NTV2Stream stream, std::string remoteIP, uint32_t & hi, uint32_t & lo);

	bool		GetSFPMSAData (eSFP port, SFPMSAData & data);
	bool		GetLinkStatus (eSFP port, SFPStatus & sfpStatus);

	bool		GenSDP (bool enableSfp1, bool enableSfp2, const NTV2Stream stream, bool pushit=true);

	static uint32_t	Get2110TxStreamIndex (NTV2Stream stream );
	static uint32_t	GetDecapsulatorAddress (eSFP sfp, NTV2Stream stream);

	bool		SetLLDPInfo (std::string sysname);
	bool		GetLLDPInfo (std::string &chassisId0, std::string &portId0,
							std::string &chassisId1, std::string &portId1);
	uint64_t	GetNTPTimestamp (void);

	// If method returns false call this to get details
	std::string	getLastError (void);
	NTV2IpError	getLastErrorCode (void);

	static uint32_t	videoPacketizers[4];
	static uint32_t	videoRGB12Packetizers[4];
	static uint32_t	audioPacketizers[4];

	static uint32_t	videoDepacketizers[4];
	static uint32_t	audioDepacketizers[4];

protected:
	uint32_t	GetDepacketizerAddress (const NTV2Stream stream);
	uint32_t	GetPacketizerAddress (const NTV2Stream stream, const VPIDSampling sampling);
	uint32_t	GetFramerAddress (const eSFP sfp, const NTV2Stream stream);
	void		SelectTxFramerChannel (const NTV2Stream stream, const uint32_t baseAddr);
	void		AcquireFramerControlAccess (const uint32_t baseAddr);
	void		ReleaseFramerControlAccess (const uint32_t baseAddr);

	void		EnableFramerStream (const eSFP sfp, const NTV2Stream stream, bool enable);
	bool		SetFramerStream (const eSFP sfp, const NTV2Stream stream, const tx_2110Config & txConfig);
	void		GetFramerStream (const eSFP sfp, const NTV2Stream stream, tx_2110Config  & txConfig);
	void		SetArbiter (const eSFP sfp, const NTV2Stream stream, bool enable);
	void		GetArbiter (const eSFP sfp, const NTV2Stream stream, bool & enable);

	void		SetSampling(const eSFP sfp, const NTV2Stream stream, const VPIDSampling sampling);
	VPIDSampling GetSampling(const eSFP sfp, const NTV2Stream stream);

	void		DisableDepacketizerStream (const NTV2Stream stream);
	void		EnableDepacketizerStream (const NTV2Stream stream);
	void		DisableDecapsulatorStream (const eSFP sfp, const NTV2Stream stream);
	void		EnableDecapsulatorStream (const eSFP sfp, const NTV2Stream stream);

	void		SetupDecapsulatorStream (const eSFP sfp, const NTV2Stream stream, const rx_2110Config &rxConfig);

	void		ResetPacketizerStream (const NTV2Stream stream);

	void		SetupDepacketizerStream (const NTV2Stream stream, const rx_2110Config & rxConfig);
	void		ResetDepacketizerStream (const NTV2Stream stream);

	void		SetVideoFormatForRxTx (const NTV2Stream stream, const NTV2VideoFormat format, const bool rx);
	void		GetVideoFormatForRxTx (const NTV2Stream stream, NTV2VideoFormat & format, uint32_t & hwFormat, const bool rx);

	bool		ConfigurePTP (const eSFP sfp, const std::string localIPAddress);

	bool		GenVideoStreamSDP (std::stringstream &sdp, const bool enableSfp1,
									const bool enableSfp2, const NTV2Stream stream, char *gmInfo);
	bool		GenVideoStreamSDPInfo (std::stringstream & sdp, const eSFP sfp, const NTV2Stream stream, char* gmInfo);
	bool		GenVideoStreamMultiSDPInfo (std::stringstream & sdp, char* gmInfo);
	bool		GenAudioStreamSDP (std::stringstream &sdp, const bool enableSfp1,
									const bool enableSfp2, const NTV2Stream stream, char *gmInfo);
	bool		GenAudioStreamSDPInfo (std::stringstream & sdp, const eSFP sfp, const NTV2Stream stream, char* gmInfo);
	bool		GenAncStreamSDP (std::stringstream &sdp, const bool enableSfp1,
								const bool enableSfp2, const NTV2Stream stream, char *gmInfo);
	bool		GenAncStreamSDPInfo (std::stringstream & sdp, const eSFP sfp, const NTV2Stream stream, char* gmInfo);

	NTV2StreamType	StreamType (const NTV2Stream stream);
	NTV2Channel		VideoStreamToChannel (const NTV2Stream stream);

private:
	std::string	To_String (int val);

	std::vector<std::string> split (const char *str, char delim);

	std::string		rateToString (NTV2FrameRate rate);
	NTV2FrameRate	stringToRate (std::string str);

	int			LeastCommonMultiple (int a,int b);
	int			getDescriptionValue (int startLine, std::string type, std::string & value);
	std::string	getVideoDescriptionValue (std::string type);

	std::stringstream	txsdp;

	uint32_t	_numRx0Chans;
	uint32_t	_numRx1Chans;
	uint32_t	_numTx0Chans;
	uint32_t	_numTx1Chans;
	uint32_t	_numRxChans;
	uint32_t	_numTxChans;
	bool		_biDirectionalChannels;	// logically bi-directional channels

	std::vector<std::string>	sdpLines;
	std::vector<std::string>	tokens;

};	//	CNTV2Config2110

#endif // NTV2_2110CONFIG_H
