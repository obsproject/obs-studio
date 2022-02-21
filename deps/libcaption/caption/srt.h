/**********************************************************************************************/
/* The MIT License                                                                            */
/*                                                                                            */
/* Copyright 2016-2017 Twitch Interactive, Inc. or its affiliates. All Rights Reserved.       */
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
#ifndef LIBCAPTION_SRT_H
#define LIBCAPTION_SRT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "caption.h"
#include "eia608.h"
#include "vtt.h"

// timestamp and duration are in seconds
typedef vtt_t srt_t;
typedef vtt_block_t srt_cue_t;

/*! \brief
    \param
*/
srt_t* srt_new();
/*! \brief
    \param
*/
srt_t* srt_free_head(srt_t* head);
// returns the head of the link list. must bee freed when done
/*! \brief
    \param
*/
srt_t* srt_parse(const utf8_char_t* data, size_t size);
/*! \brief
    \param
*/
void srt_free(srt_t* srt);

/*! \brief
    \param
*/
static inline vtt_block_t* srt_next(vtt_block_t* srt) { return srt->next; }

/*! \brief
    \param
*/
static inline utf8_char_t* srt_cue_data(srt_cue_t* cue) { return vtt_block_data(cue); }

/*! \brief
    \param
*/
static inline srt_cue_t* srt_cue_from_caption_frame(caption_frame_t* frame, srt_t* srt) { return vtt_cue_from_caption_frame(frame, srt); };

/*! \brief
    \param
*/
static inline void srt_cue_free_head(srt_t* srt) { vtt_cue_free_head(srt); };

/*! \brief
    \param
*/
static inline srt_cue_t* srt_cue_new(srt_t* srt, const utf8_char_t* data, size_t size) { return vtt_block_new(srt, data, size, VTT_CUE); };

/*! \brief
    \param
*/
static inline int srt_cue_to_caption_frame(srt_cue_t* cue, caption_frame_t* frame) { return vtt_cue_to_caption_frame(cue, frame); };

void srt_dump(srt_t* srt);
/*! \brief
    \param
*/
void vtt_dump(srt_t* srt);

#ifdef __cplusplus
}
#endif
#endif
