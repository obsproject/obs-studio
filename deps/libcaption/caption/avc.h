/**********************************************************************************************/
/* The MIT License                                                                            */
/*                                                                                            */
/* Copyright 2016-2016 Twitch Interactive, Inc. or its affiliates. All Rights Reserved.       */
/*                                                                                            */
/* Permission is hereby granted, free of charge, to any person obtaining a copy               */
/* of this software and associated documentation files (the "Software"), to deal              */
/* in the Software without restriction, including without limitation the rights               */
/* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell                  */
/* copies of the Software, and to permit persons to whom the Software is                      */
/* furnished to do so, subject to the following conditions:                                   */
/*                                                                                            */
/* The above copyright notice and this permission notice shall be included in                 */
/* all copies or substantial portions of the Software.                                        */
/*                                                                                            */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR                 */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,                   */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE                */
/* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER                     */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,              */
/* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN                  */
/* THE SOFTWARE.                                                                              */
/**********************************************************************************************/
#ifndef LIBCAPTION_AVC_H
#define LIBCAPTION_AVC_H
#include "cea708.h"
#include "caption.h"
#include <float.h>
////////////////////////////////////////////////////////////////////////////////
#define MAX_NALU_SIZE (4*1024*1024)
typedef struct {
    size_t size;
    uint8_t data[MAX_NALU_SIZE];
} avcnalu_t;

void avcnalu_init (avcnalu_t* nalu);
int avcnalu_parse_annexb (avcnalu_t* nalu, const uint8_t** data, size_t* size);
static inline uint8_t  avcnalu_type (avcnalu_t* nalu) { return nalu->data[0] & 0x1F; }
static inline uint8_t* avcnalu_data (avcnalu_t* nalu) { return &nalu->data[0]; }
static inline size_t   avcnalu_size (avcnalu_t* nalu) { return nalu->size; }
////////////////////////////////////////////////////////////////////////////////
typedef struct _sei_message_t sei_message_t;

typedef enum {
    sei_type_buffering_period = 0,
    sei_type_pic_timing = 1,
    sei_type_pan_scan_rect = 2,
    sei_type_filler_payload = 3,
    sei_type_user_data_registered_itu_t_t35 = 4,
    sei_type_user_data_unregistered = 5,
    sei_type_recovery_point = 6,
    sei_type_dec_ref_pic_marking_repetition = 7,
    sei_type_spare_pic = 8,
    sei_type_scene_info = 9,
    sei_type_sub_seq_info = 10,
    sei_type_sub_seq_layer_characteristics = 11,
    sei_type_sub_seq_characteristics = 12,
    sei_type_full_frame_freeze = 13,
    sei_type_full_frame_freeze_release = 14,
    sei_type_full_frame_snapshot = 15,
    sei_type_progressive_refinement_segment_start = 16,
    sei_type_progressive_refinement_segment_end = 17,
    sei_type_motion_constrained_slice_group_set = 18,
    sei_type_film_grain_characteristics = 19,
    sei_type_deblocking_filter_display_preference = 20,
    sei_type_stereo_video_info = 21,
} sei_msgtype_t;
////////////////////////////////////////////////////////////////////////////////
// time in seconds
typedef struct  {
    double dts;
    double cts;
    sei_message_t* head;
    sei_message_t* tail;
} sei_t;

/*! \brief
    \param
*/
void sei_init (sei_t* sei);
/*! \brief
    \param
*/
void sei_free (sei_t* sei);
/*! \brief
    \param
*/
static inline double sei_dts (sei_t* sei) { return sei->dts; }
static inline double sei_cts (sei_t* sei) { return sei->cts; }
static inline double sei_pts (sei_t* sei) { return sei->dts + sei->cts; }
/*! \brief
    \param
*/
int sei_parse_nalu (sei_t* sei, const uint8_t* data, size_t size, double dts, double cts);
/*! \brief
    \param
*/
// TODO add dts,cts to nalu
static inline int sei_parse_avcnalu (sei_t* sei, avcnalu_t* nalu, double dts, double cts) { return sei_parse_nalu (sei,avcnalu_data (nalu),avcnalu_size (nalu),dts,cts); }
/*! \brief
    \param
*/
static inline int sei_finish (sei_t* sei) { return sei_parse_nalu (sei,0,0,0.0,DBL_MAX); }
/*! \brief
    \param
*/
static inline sei_message_t* sei_message_head (sei_t* sei) { return sei->head; }
/*! \brief
    \param
*/
static inline sei_message_t* sei_message_tail (sei_t* sei) { return sei->tail; }
/*! \brief
    \param
*/
sei_message_t* sei_message_next (sei_message_t* msg);
/*! \brief
    \param
*/
sei_msgtype_t sei_message_type (sei_message_t* msg);
/*! \brief
    \param
*/
size_t sei_message_size (sei_message_t* msg);
/*! \brief
    \param
*/
uint8_t* sei_message_data (sei_message_t* msg);
/*! \brief
    \param
*/
sei_message_t* sei_message_new (sei_msgtype_t type, uint8_t* data, size_t size);
/*! \brief
    \param
*/
static inline sei_message_t* sei_message_copy (sei_message_t* msg)
{
    return sei_message_new (sei_message_type (msg), sei_message_data (msg), sei_message_size (msg));
}
/**
Free message and all accoiated data. Messaged added to sei_t by using sei_append_message MUST NOT be freed
These messages will be freed by calling sei_free()
*/
/*! \brief
    \param
*/
void  sei_message_free (sei_message_t* msg);
////////////////////////////////////////////////////////////////////////////////
/*! \brief
    \param
*/
static inline int sei_decode_cea708 (sei_message_t* msg, cea708_t* cea708)
{
    if (sei_type_user_data_registered_itu_t_t35 == sei_message_type (msg)) {
        return cea708_parse (sei_message_data (msg), sei_message_size (msg), cea708);
    } else {
        return 0;
    }
}
////////////////////////////////////////////////////////////////////////////////
/*! \brief
    \param
*/
size_t sei_render_size (sei_t* sei);
/*! \brief
    \param
*/
size_t sei_render (sei_t* sei, uint8_t* data);
/*! \brief
    \param
*/
void sei_dump (sei_t* sei);
/*! \brief
    \param
*/
void sei_dump_messages (sei_message_t* head);
////////////////////////////////////////////////////////////////////////////////
/*! \brief
    \param
*/
int sei_from_caption_frame (sei_t* sei, caption_frame_t* frame);
/*! \brief
    \param
*/
libcaption_stauts_t sei_to_caption_frame (sei_t* sei, caption_frame_t* frame);
/*! \brief
    \param
*/
static inline int nalu_to_caption_frame (caption_frame_t* frame, const uint8_t* data, size_t size, double pts, double dts)
{
    sei_t sei;
    sei_init (&sei);
    sei_parse_nalu (&sei, data, size, pts, dts);
    sei_to_caption_frame (&sei,frame);
    sei_free (&sei);
    return 1;
}
////////////////////////////////////////////////////////////////////////////////
#endif
