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
#ifndef LIBCAPTION_VTT_H
#define LIBCAPTION_VTT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "caption.h"
#include "eia608.h"

enum VTT_BLOCK_TYPE {
    VTT_REGION = 0,
    VTT_STYLE = 1,
    VTT_NOTE = 2,
    VTT_CUE = 3
};

// CUE represents a block of caption text
typedef struct _vtt_block_t {
    struct _vtt_block_t* next;
    enum VTT_BLOCK_TYPE type;
    // CUE-Only
    double timestamp;
    double duration; // -1.0 for no duration
    char* cue_settings;
    char* cue_id;
    // Standard block data
    size_t text_size;
    char* block_text;
} vtt_block_t;

// VTT files are a collection of REGION, STYLE and CUE blocks.
// XXX: Comments (NOTE blocks) are ignored
typedef struct _vtt_t {
    vtt_block_t* region_head;
    vtt_block_t* region_tail;
    vtt_block_t* style_head;
    vtt_block_t* style_tail;
    vtt_block_t* cue_head;
    vtt_block_t* cue_tail;
} vtt_t;

/*! \brief
    \param
*/
vtt_t* vtt_new();
/*! \brief
    \param
*/
void vtt_free(vtt_t* vtt);

/*! \brief
    \param
*/
vtt_block_t* vtt_block_new(vtt_t* vtt, const utf8_char_t* data, size_t size, enum VTT_BLOCK_TYPE type);

/*! \brief
    \param
*/
void vtt_cue_free_head(vtt_t* vtt);

/*! \brief
    \param
*/
void vtt_style_free_head(vtt_t* vtt);

/*! \brief
    \param
*/
void vtt_region_free_head(vtt_t* vtt);

// returns a vtt_t, containing linked lists of blocks. must be freed when done
/*! \brief
    \param
*/
vtt_t* vtt_parse(const utf8_char_t* data, size_t size);

/*! \brief
    \param
*/
vtt_t* _vtt_parse(const utf8_char_t* data, size_t size, int srt_mode);

/*! \brief
    \param
*/
static inline vtt_block_t* vtt_cue_next(vtt_block_t* block) { return block->next; }

/*! \brief
    \param
*/
static inline utf8_char_t* vtt_block_data(vtt_block_t* block) { return (utf8_char_t*)(block) + sizeof(vtt_block_t); }

/*! \brief
    \param
*/
static inline void vtt_crack_time(double tt, int* hh, int* mm, int* ss, int* ms)
{
    (*ms) = (int)((int64_t)(tt * 1000) % 1000);
    (*ss) = (int)((int64_t)(tt) % 60);
    (*mm) = (int)((int64_t)(tt / (60)) % 60);
    (*hh) = (int)((int64_t)(tt / (60 * 60)));
}

// This only converts the current CUE, it does not walk the list
/*! \brief
    \param
*/
int vtt_cue_to_caption_frame(vtt_block_t* cue, caption_frame_t* frame);

// returns the new cue
/*! \brief
    \param
*/
vtt_block_t* vtt_cue_from_caption_frame(caption_frame_t* frame, vtt_t* vtt);
/*! \brief
    \param
*/
void vtt_dump(vtt_t* vtt);

#ifdef __cplusplus
}
#endif
#endif
