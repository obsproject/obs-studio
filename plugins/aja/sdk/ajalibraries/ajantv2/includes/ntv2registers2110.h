/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2registers2110.h
	@brief		Defines the KonaIP/IoIP S2110 registers.
	@copyright	(C) 2014-2021 AJA Video Systems, Inc.
**/

#ifndef REGISTERS_2110_H
#define REGISTERS_2110_H

#include "ntv2registersmb.h"

#define SAREK_4175_TX_PACKETIZER_1              (0x200000/4)
#define SAREK_4175_TX_PACKETIZER_2              (0x201000/4)
#define SAREK_4175_TX_PACKETIZER_3              (0x202000/4)
#define SAREK_4175_TX_PACKETIZER_4              (0x203000/4)

#define SAREK_4175_TX_PACKETIZER_RGB12_1		(0x204000/4)
#define SAREK_4175_TX_PACKETIZER_RGB12_2		(0x205000/4)
#define SAREK_4175_TX_PACKETIZER_RGB12_3		(0x206000/4)
#define SAREK_4175_TX_PACKETIZER_RGB12_4		(0x207000/4)

#define SAREK_3190_TX_PACKETIZER_0              (0x220000/4)
#define SAREK_3190_TX_PACKETIZER_1              (0x221000/4)
#define SAREK_3190_TX_PACKETIZER_2              (0x222000/4)
#define SAREK_3190_TX_PACKETIZER_3              (0x223000/4)

#define SAREK_4175_RX_DEPACKETIZER_1            (0x208000/4)
#define SAREK_4175_RX_DEPACKETIZER_2            (0x209000/4)
#define SAREK_4175_RX_DEPACKETIZER_3            (0x20a000/4)
#define SAREK_4175_RX_DEPACKETIZER_4            (0x20b000/4)

#define SAREK_3190_RX_DEPACKETIZER_1            (0x20c000/4)
#define SAREK_3190_RX_DEPACKETIZER_2            (0x20d000/4)
#define SAREK_3190_RX_DEPACKETIZER_3            (0x20e000/4)
#define SAREK_3190_RX_DEPACKETIZER_4            (0x20f000/4)

#define SAREK_2110_VIDEO_FRAMER_0               (0x210000/4)
#define SAREK_2110_AUDIO_ANC_FRAMER_0			(0x212000/4)
#define SAREK_2110_VIDEO_FRAMER_1               (0x213000/4)
#define SAREK_2110_AUDIO_ANC_FRAMER_1			(0x214000/4)
#define SAREK_2110_TX_ARBITRATOR                (0x215000/4)
#define SAREK_2110_DECAPSULATOR_0               (0x211000/4)
#define SAREK_2110_DECAPSULATOR_1               (0x216000/4)
#define SAREK_2110_VOIPFEC              		(0x217000/4)
#define SAREK_2110_VOIPFEC_CTL          		(0x218000/4)
#define SAREK_2110_FRAMESYNC          			(0x219000/4)
#define SAREK_2110_AUDIO_STREAMSELECT           (0x230000/4)
#define SAREK_2110_VIDEO_TIMING_CTRLR			(0x240000/4)
#define SAREK_2110_VIDEO_TIMING_CTRLR2			(0x241000/4)

#define SAREK_2110_TEST_GENERATOR               (0x308000/4)
#define SAREK_AXIS_PCAP							(0x309000/4)


/////////////////////////////////////////////////////////////////////
//
// 4175 Packeteizer
//
/////////////////////////////////////////////////////////////////////

#define kReg4175_pkt_ctrl                       0x00
#define kReg4175_pkt_width                      0x04
#define kReg4175_pkt_height                     0x06
#define kReg4175_pkt_vid_fmt                    0x08
#define kReg4175_pkt_pkts_per_line              0x0a
#define kReg4175_pkt_payload_len                0x0c
#define kReg4175_pkt_payload_len_last           0x0e
#define kReg4175_pkt_ssrc                       0x10
#define kReg4175_pkt_payload_type               0x12
#define kReg4175_pkt_bpc_reg                    0x14
#define kReg4175_pkt_chan_num                   0x16
#define kReg4175_pkt_tx_pkt_cnt                 0x18
#define kReg4175_pkt_tx_pkt_cnt_valid           0x19
#define kReg4175_pkt_pix_per_pkt                0x1a
#define kReg4175_pkt_stat_reset                 0x1c
#define kReg4175_pkt_interlace_ctrl             0x1e

/////////////////////////////////////////////////////////////////////
//
// 4175 Depacketizer
//
/////////////////////////////////////////////////////////////////////

#define kReg4175_depkt_control                  (0x0000/4)
#define kReg4175_depkt_width_o                  (0x0010/4)
#define kReg4175_depkt_height_o                 (0x0018/4)
#define kReg4175_depkt_vid_fmt_o                (0x0020/4)
#define kReg4175_depkt_pkts_per_line_o          (0x0028/4)
#define kReg4175_depkt_payload_len_o            (0x0030/4)
#define kReg4175_depkt_payload_len_last_o       (0x0038/4)
#define kReg4175_depkt_bpc_reg_o                (0x0040/4)
#define kReg4175_depkt_rx_pkt_cnt_o             (0x0048/4)
#define kReg4175_depkt_rx_pkt_cnt_valid_o       (0x0050/4)
#define kReg4175_depkt_stat_reset_o             (0x0054/4)
#define kReg4175_depkt_rx_field_cnt             (0x0058/4)
#define kReg4175_depkt_rx_pkt_cnt               (0x005C/4)
#define kReg4175_depkt_rx_byte_cnt              (0x0060/4)
#define kReg4175_depkt_bytes_per_line           (0x0064/4)
#define kReg4175_depkt_rows_per_field           (0x0068/4)
#define kReg4175_depkt_version_id               (0x006C/4)
#define	kReg4175_depkt_pixels_per_field			(0x0070/4)
#define kReg4175_depkt_bytes_per_field			(0x0074/4)
#define kReg4175_depkt_sequence_err             (0x007C/4)

/////////////////////////////////////////////////////////////////////
//
// 3190 Packeteizer
//
/////////////////////////////////////////////////////////////////////

#define kReg3190_pkt_ctrl                       (0x0000/4)
#define kReg3190_pkt_num_samples                (0x0010/4)
#define kReg3190_pkt_num_audio_channels         (0x0018/4)
#define kReg3190_pkt_payload_len                (0x0020/4)
#define kReg3190_pkt_chan_num                   (0x0028/4)
#define kReg3190_pkt_payload_type               (0x0030/4)
#define kReg3190_pkt_ssrc                       (0x0038/4)
#define kReg3190_pkt_tx_pkt_cnt                 (0x0040/4)
#define kReg3190_pkt_stat_reset                 (0x00B0/4)


/////////////////////////////////////////////////////////////////////
//
// 3190 Depacketizer
//
/////////////////////////////////////////////////////////////////////

#define kReg3190_depkt_enable                   0
#define kReg3190_depkt_config                   1
#define kReg3190_depkt_rx_pkt_count             2

/////////////////////////////////////////////////////////////////////
//
// Framer
//
/////////////////////////////////////////////////////////////////////

#define kRegFramer_control              (0x0000/4)
#define kRegFramer_status               (0x0004/4)
#define kRegFramer_channel_access       (0x0008/4)
#define kRegFramer_sys_config           (0x000c/4)
#define kRegFramer_version              (0x0010/4)
#define kRegFramer_src_mac_lo           (0x0014/4)
#define kRegFramer_src_mac_hi           (0x0018/4)
#define kRegFramer_peak_buf_level       (0x001c/4)
#define kRegFramer_rx_pkt_cnt           (0x0020/4)
#define kRegFramer_drop_pkt_cnt         (0x0024/4)
#define kRegFramer_stat_reset           (0x0030/4)

// channel
#define kRegFramer_chan_ctrl            (0x0080/4)
#define kRegFramer_dest_mac_lo          (0x0084/4)
#define kRegFramer_dest_mac_hi          (0x0088/4)
#define kRegFramer_vlan_tag_info        (0x008c/4)
#define kRegFramer_ip_hdr_media         (0x0090/4)
#define kRegFramer_ip_hdr_fec           (0x0094/4)
#define kRegFramer_src_ip               (0x0098/4)
#define kRegFramer_dst_ip               (0x00a8/4)
#define kRegFramer_udp_src_port         (0x00b8/4)
#define kRegFramer_udp_dst_port         (0x00bc/4)
#define kRegFramer_tk_pkt_cnt           (0x00c0/4)
#define kRegFramer_chan_stat_reset      (0x00c4/4)

/////////////////////////////////////////////////////////////////////
//
// Decapsulator
//
/////////////////////////////////////////////////////////////////////

#define kRegDecap_chan_enable           0x00
#define kRegDecap_match_reserved        0x01
#define kRegDecap_match_src_ip          0x02
#define kRegDecap_match_dst_ip          0x03
#define kRegDecap_match_udp_src_port    0x04
#define kRegDecap_match_udp_dst_port    0x05
#define kRegDecap_match_payload         0x06
#define kRegDecap_match_ssrc            0x07
#define kRegDecap_match_sel             0x08
#define kRegDecap_unused                0x09
#define kRegDecap_rx_payload            0x0a
#define kRegDecap_rx_ssrc               0x0b
#define kRegDecap_rx_pkt_cnt            0x0c
#define kRegDecap_reordered_pkt_cnt     0x0d
#define kRegDecap_unused2               0x0e
#define kRegDecap_descriptiom           0x0f

/////////////////////////////////////////////////////////////////////
//
// VOIP FEC
//
/////////////////////////////////////////////////////////////////////

#define kRegVfec_control                (0x00/4)
#define kRegVfec_status                 (0x04/4)
#define kRegVfec_channel_access			(0x08/4)
#define kRegVfec_sys_config             (0x0c/4)
#define kRegVfec_version                (0x10/4)
#define kRegVfec_fec_processing_delay   (0x20/4)
#define kRegVfec_fec_packet_drop_cnt    (0x24/4)

#define kRegVfec_chan_conf              (0x80/4)
#define kRegVfec_valid_pkt_cnt          (0x90/4)
#define kRegVfec_unrecv_pkt_cnt         (0x94/4)
#define kRegVfec_corr_pkt_cnt           (0x98/4)
#define kRegVfec_dup_pkt_cnt            (0x9c/4)
#define kRegVfec_channel_status         (0xa8/4)
#define kRegVfec_curr_buffer_depth      (0xac/4)
#define kRegVfec_oor_pkt_cnt            (0xb0/4)
#define kRegVfec_oor_ts_offset          (0xb4/4)
#define kRegVfec_link_ts_diff           (0xb8/4)

#define kRegVfec_delay_control			(0x00/4)
#define kRegVfec_delay_ch0_playout		(0x04/4)
#define	kRegVfec_delay_ch1_playout		(0x08/4)
#define	kRegVfec_delay_ch2_playout		(0x0c/4)
#define	kRegVfec_delay_ch3_playout		(0x10/4)
#define	kRegVfec_audio_pkt_read_intrvl	(0x14/4)
#define	kRegVfec_video_pkt_read_intrvl	(0x18/4)
#define	kRegVfec_delay_status			(0x20/4)
#define	kRegVfec_delay_version			(0x3c/4)

/////////////////////////////////////////////////////////////////////
//
// Frame Sync
//
/////////////////////////////////////////////////////////////////////

#define kRegVFS_mm2s_control           	(0x00/4)
#define kRegVFS_mm2s_status            	(0x04/4)
#define kRegVFS_mm2s_index             	(0x14/4)
#define kRegVFS_park_ptr               	(0x28/4)
#define kRegVFS_version                	(0x2c/4)
#define kRegVFS_s2mm_control            (0x30/4)
#define kRegVFS_s2mm_status             (0x34/4)
#define kRegVFS_s2mm_irq_mask          	(0x3c/4)
#define kRegVFS_s2mm_index             	(0x44/4)
#define kRegVFS_mm2s_vsize             	(0x50/4)
#define kRegVFS_mm2s_hsize           	(0x54/4)
#define kRegVFS_mm2s_frmdly_stride    	(0x58/4)
#define kRegVFS_mm2s_start_addr01      	(0x5c/4)
#define kRegVFS_mm2s_start_addr02      	(0x60/4)
#define kRegVFS_mm2s_start_addr03      	(0x64/4)
#define kRegVFS_mm2s_start_addr04      	(0x68/4)
#define kRegVFS_mm2s_start_addr05      	(0x6c/4)
#define kRegVFS_mm2s_start_addr06      	(0x70/4)
#define kRegVFS_mm2s_start_addr07      	(0x74/4)
#define kRegVFS_mm2s_start_addr08      	(0x78/4)
#define kRegVFS_mm2s_start_addr09      	(0x7c/4)
#define kRegVFS_mm2s_start_addr10      	(0x80/4)
#define kRegVFS_mm2s_start_addr11      	(0x84/4)
#define kRegVFS_mm2s_start_addr12      	(0x88/4)
#define kRegVFS_mm2s_start_addr13      	(0x8c/4)
#define kRegVFS_mm2s_start_addr14      	(0x90/4)
#define kRegVFS_mm2s_start_addr15      	(0x94/4)
#define kRegVFS_mm2s_start_addr16      	(0x98/4)
#define kRegVFS_s2mm_vsize             	(0xa0/4)
#define kRegVFS_s2mm_hsize           	(0xa4/4)
#define kRegVFS_s2mm_frmdly_stride    	(0xa8/4)
#define kRegVFS_s2mm_start_addr01      	(0xac/4)
#define kRegVFS_s2mm_start_addr02      	(0xb0/4)
#define kRegVFS_s2mm_start_addr03      	(0xb4/4)
#define kRegVFS_s2mm_start_addr04      	(0xb8/4)
#define kRegVFS_s2mm_start_addr05      	(0xbc/4)
#define kRegVFS_s2mm_start_addr06      	(0xc0/4)
#define kRegVFS_s2mm_start_addr07      	(0xc4/4)
#define kRegVFS_s2mm_start_addr08      	(0xc8/4)
#define kRegVFS_s2mm_start_addr09      	(0xcc/4)
#define kRegVFS_s2mm_start_addr10      	(0xd0/4)
#define kRegVFS_s2mm_start_addr11      	(0xd4/4)
#define kRegVFS_s2mm_start_addr12      	(0xd8/4)
#define kRegVFS_s2mm_start_addr13      	(0xdc/4)
#define kRegVFS_s2mm_start_addr14      	(0xe0/4)
#define kRegVFS_s2mm_start_addr15      	(0xe4/4)
#define kRegVFS_s2mm_start_addr16      	(0xe8/4)

/////////////////////////////////////////////////////////////////////
//
// Video Timing Controller
//
/////////////////////////////////////////////////////////////////////

#define kRegVTC_control					(0x000/4)
#define kRegVTC_status					(0x004/4)
#define kRegVTC_error					(0x008/4)
#define kRegVTC_irq_enable				(0x00c/4)
#define kRegVTC_version					(0x010/4)
#define kRegVTC_detector_active_size	(0x020/4)
#define kRegVTC_detector_timing_status	(0x024/4)
#define kRegVTC_detector_encoding		(0x028/4)
#define kRegVTC_detector_polarity		(0x02c/4)
#define kRegVTC_detector_hsize			(0x030/4)
#define kRegVTC_detector_vsize			(0x034/4)
#define kRegVTC_detector_hsync			(0x038/4)
#define kRegVTC_detector_f0_vblank_h	(0x03c/4)
#define kRegVTC_detector_f0_vsync_v		(0x040/4)
#define kRegVTC_detector_f0_vsync_h		(0x044/4)
#define kRegVTC_generator_active_size	(0x060/4)
#define kRegVTC_generator_tmng_status	(0x064/4)
#define kRegVTC_generator_encoding		(0x068/4)
#define kRegVTC_generator_polarity		(0x06c/4)
#define kRegVTC_generator_hsize			(0x070/4)
#define kRegVTC_generator_vsize			(0x074/4)
#define kRegVTC_generator_hsync			(0x078/4)
#define kRegVTC_generator_f0_vblank_h	(0x07c/4)
#define kRegVTC_generator_f0_vsync_v	(0x080/4)
#define kRegVTC_generator_f0_vsync_h	(0x084/4)
#define kRegVTC_generator_f1_vblank_h	(0x088/4)
#define kRegVTC_generator_f1_vsync_v	(0x08c/4)
#define kRegVTC_generator_f1_vsync_h	(0x090/4)
#define kRegVTC_frame_sync_cfg_00		(0x100/4)
#define kRegVTC_frame_sync_cfg_01		(0x104/4)
#define kRegVTC_frame_sync_cfg_02		(0x108/4)
#define kRegVTC_frame_sync_cfg_03		(0x10c/4)
#define kRegVTC_frame_sync_cfg_04		(0x110/4)
#define kRegVTC_frame_sync_cfg_05		(0x114/4)
#define kRegVTC_frame_sync_cfg_06		(0x118/4)
#define kRegVTC_frame_sync_cfg_07		(0x11c/4)
#define kRegVTC_frame_sync_cfg_08		(0x120/4)
#define kRegVTC_frame_sync_cfg_09		(0x124/4)
#define kRegVTC_frame_sync_cfg_10		(0x128/4)
#define kRegVTC_frame_sync_cfg_11		(0x12c/4)
#define kRegVTC_frame_sync_cfg_12		(0x130/4)
#define kRegVTC_frame_sync_cfg_13		(0x134/4)
#define kRegVTC_frame_sync_cfg_14		(0x138/4)
#define kRegVTC_frame_sync_cfg_15		(0x13c/4)
#define kRegVTC_generator_global_delay	(0x140/4)

/////////////////////////////////////////////////////////////////////
//
// Arbitrator
//
/////////////////////////////////////////////////////////////////////

#define kRegArb_video                   0x00
#define kRegArb_audio                   0x01
#define kRegArb_4KMode                  0x07
#define kRegRxVideoDecode1              0x08
#define kRegRxVideoDecode2              0x09
#define kRegRxVideoDecode3              0x0a
#define kRegRxVideoDecode4              0x0b
#define kRegTxVideoDecode1              0x0c
#define kRegTxVideoDecode2              0x0d
#define kRegTxVideoDecode3              0x0e
#define kRegTxVideoDecode4              0x0f

#define kRegTxAncSSRC1					0x10
#define kRegTxAncSSRC2					0x11
#define kRegTxAncSSRC3					0x12
#define kRegTxAncSSRC4					0x13
#define kRegTxAncPayload1				0x14
#define kRegTxAncPayload2				0x15
#define kRegTxAncPayload3				0x16
#define kRegTxAncPayload4				0x17

/////////////////////////////////////////////////////////////////////
//
// 2110 Block scratch pad registers
//
/////////////////////////////////////////////////////////////////////

#define kRegRxNtv2VideoDecode1      (S2110_BLOCK_BASE+0)
#define kRegRxNtv2VideoDecode2      (S2110_BLOCK_BASE+1)
#define kRegRxNtv2VideoDecode3      (S2110_BLOCK_BASE+2)
#define kRegRxNtv2VideoDecode4      (S2110_BLOCK_BASE+3)
#define kRegTxNtv2VideoDecode1      (S2110_BLOCK_BASE+4)
#define kRegTxNtv2VideoDecode2      (S2110_BLOCK_BASE+5)
#define kRegTxNtv2VideoDecode3      (S2110_BLOCK_BASE+6)
#define kRegTxNtv2VideoDecode4      (S2110_BLOCK_BASE+7)

#endif // REGISTERS_2110_H
