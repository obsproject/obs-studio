/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2mbcontroller.h
	@brief		Declares the CNTV2MBController class.
	@copyright	(C) 2015-2021 AJA Video Systems, Inc.    All rights reserved.
**/

#ifndef NTV2MBCONTROLLER_H
#define NTV2MBCONTROLLER_H

#include "ntv2card.h"
#include "ntv2mailbox.h"
#include <iostream>
#include <vector>

enum eMBCmd
{
    MB_CMD_SET_NET                = 0,
    MB_CMD_GET_MAC_FROM_ARP_TABLE = 3,
    MB_CMD_SEND_ARP_REQ           = 4,
    MB_CMD_UNKNOWN                = 5,
    MB_CMD_SET_IGMP_VERSION       = 6,
    MB_CMD_FETCH_GM_INFO          = 7,
    MB_CMD_TAKE_SDP               = 8,
    MB_CMD_FETCH_SDP              = 9,
    MB_CMD_DISABLE_NET_IF         = 10,
    MB_CMD_FETCH_SFP_INFO         = 11,
	MB_CMD_SET_LLDP_INFO          = 12,
	MB_CMD_GET_LLDP_INFO          = 13
};

enum eNTV2PacketInterval
{
    PACKET_INTERVAL_125uS,
    PACKET_INTERVAL_1mS
};

enum eSFP
{
    SFP_1,
    SFP_2,
    SFP_MAX_NUM_SFPS,
    SFP_INVALID		= SFP_MAX_NUM_SFPS
};

#define	NTV2_IS_VALID_SFP(__sfp__)		(((__sfp__) >= SFP_1)  &&  ((__sfp__) < SFP_INVALID))

enum eArpState
{
    ARP_ERROR,
    ARP_VALID,
    ARP_INCOMPLETE,
    ARP_NOT_FOUND
};

typedef enum
{
    eIGMPVersion_2,
    eIGMPVersion_3,
    eIGMPVersion_Default = eIGMPVersion_3
} eIGMPVersion_t;

typedef struct
{
    uint8_t	mac[6];
} MACAddr;

typedef struct
{
    uint8_t data[64];
} SFPMSAData;

struct SFPStatus
{
    bool SFP_present;           // true indicates SFP plugged in
    bool SFP_rxLoss;            // true indicates loss of signal
    bool SFP_txFault;           // true indicates tx fault
    bool SFP_linkUp;            // true indicates link is up
};

typedef enum
{
    PTP_NO_PTP,
    PTP_ERROR,
    PTP_NOT_LOCKED,
    PTP_LOCKING,
    PTP_LOCKED
} PTPLockStatus;

struct PTPStatus
{
    uint8_t PTP_gmId[8];		// GrandMasterID
    uint8_t PTP_masterId[8];	// MasterID
    PTPLockStatus PTP_LockedState;	// locked state
};


// IGMP Control Block
#define IGMPCB_REG_STATE       0
#define IGMPCB_REG_MCAST_ADDR  1
#define IGMPCB_REG_SRC_ADDR    2
#define IGMPCB_SIZE            3

#define IGMPCB_STATE_USED     BIT(0)
#define IGMPCB_STATE_ENABLED  BIT(1)
#define IGMPCB_STATE_BUSY     BIT(31)   // ignore when busy

#define S2022_LINK_A_ACTIVE   BIT(31)
#define S2022_LINK_B_ACTIVE   BIT(30)
#define S2022_DUAL_LINK       BIT(29)

class IPVNetConfig
{
public:
    IPVNetConfig() { init(); }

    void init();

    bool operator == ( const IPVNetConfig &other );
    bool operator != ( const IPVNetConfig &other );

    uint32_t    ipc_ip;
    uint32_t    ipc_subnet;
    uint32_t    ipc_gateway;
};

class AJAExport CNTV2MBController : public CNTV2MailBox
{
public:
    CNTV2MBController(CNTV2Card & device);

protected:
    // all these methods block until response received or timeout
    bool SetMBNetworkConfiguration(eSFP port, std::string ipaddr, std::string netmask,std::string gateway);
    bool DisableNetworkInterface(eSFP port);
    bool GetRemoteMAC(std::string remote_IPAddress, eSFP port, NTV2Stream stream, std::string & MACaddress);
    bool SetIGMPVersion(uint32_t version);

    void SetIGMPGroup(eSFP port, NTV2Stream stream, uint32_t mcast_addr, uint32_t src_addr, bool enable);
    void UnsetIGMPGroup(eSFP port, NTV2Stream stream);
    void EnableIGMPGroup(eSFP port, NTV2Stream stream, bool enable);

    bool SetTxLinkState(NTV2Channel channel, bool sfp1Enable,   bool sfp2Enable);
    bool GetTxLinkState(NTV2Channel channel, bool & sfp1Enable, bool & sfp2Enable);
    bool SetRxLinkState(NTV2Channel channel, bool sfp1Enable,   bool sfp2Enable);
    bool GetRxLinkState(NTV2Channel channel, bool & sfp1Enable, bool & sfp2Enable);

    bool SetDualLinkMode(bool enable);
    bool GetDualLinkMode(bool & enable);

    bool SetRxMatch(NTV2Channel channel, eSFP link, uint8_t match);
    bool GetRxMatch(NTV2Channel channel, eSFP link, uint8_t & match);

    bool SetSFPActive(eSFP sfp);
    bool SetSFPInactive(eSFP sfp);
    bool GetSFPActive(eSFP sfp);

    bool SetTxFormat(NTV2Channel chan, NTV2VideoFormat fmt);
    bool GetTxFormat(NTV2Channel chan, NTV2VideoFormat & fmt);

    uint64_t GetNTPTimestamp();
    bool PushSDP(std::string filename, std::stringstream & sdpstream);
    bool GetSDP(std::string url, std::string & sdp);

    bool GetSFPInfo(eSFP port, SFPMSAData & sfpdata);

	bool SetLLDPInfo(std::string sysname);
	bool GetLLDPInfo(std::string &chassisId0, std::string &portId0,
					std::string &chassisId1, std::string &portId1);


private:
    eArpState GetRemoteMACFromArpTable(std::string remote_IPAddress, eSFP port, NTV2Stream stream, std::string & MACaddress);
    bool SendArpRequest(std::string remote_IPAddress, eSFP port);

    void splitResponse(const std::string response, std::vector<std::string> & results);
    bool getDecimal(const std::string & resp, const std::string & parm, uint32_t & result);
    bool getHex(const std::string & resp, const std::string & parm, uint32_t &result);
    bool getString(const std::string & resp, const std::string & parm, std::string & result);
    uint32_t getIGMPCBOffset(eSFP port, NTV2Stream stream);

private:
};

#endif // NTV2MBCONTROLLER_H
