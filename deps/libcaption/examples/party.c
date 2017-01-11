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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "flv.h"
#include "avc.h"

#define CAPTION_METHOD_SEI_708  0 // embedded 708
#define CAPTION_METHOD_AMF_708  1 // onCaptionInfo type = 708
#define CAPTION_METHOD_AMF_UTF8 2 // onCaptionInfo type = utf8
#define CAPTION_METHOD CAPTION_METHOD_SEI_708

void get_dudes (char* str)
{
    sprintf (str, " %s%s %s(-_-)%s %s(-_-)%s.%s(-_-)%s %s%s", EIA608_CHAR_EIGHTH_NOTE, EIA608_CHAR_EIGHTH_NOTE,
             ! (rand() % 2) ? EIA608_CHAR_BOX_DRAWINGS_LIGHT_DOWN_AND_RIGHT : EIA608_CHAR_BOX_DRAWINGS_LIGHT_UP_AND_RIGHT,
             ! (rand() % 2) ? EIA608_CHAR_BOX_DRAWINGS_LIGHT_DOWN_AND_LEFT : EIA608_CHAR_BOX_DRAWINGS_LIGHT_UP_AND_LEFT,
             ! (rand() % 2) ? EIA608_CHAR_BOX_DRAWINGS_LIGHT_DOWN_AND_RIGHT : EIA608_CHAR_BOX_DRAWINGS_LIGHT_UP_AND_RIGHT,
             ! (rand() % 2) ? EIA608_CHAR_BOX_DRAWINGS_LIGHT_DOWN_AND_LEFT : EIA608_CHAR_BOX_DRAWINGS_LIGHT_UP_AND_LEFT,
             ! (rand() % 2) ? EIA608_CHAR_BOX_DRAWINGS_LIGHT_DOWN_AND_RIGHT : EIA608_CHAR_BOX_DRAWINGS_LIGHT_UP_AND_RIGHT,
             ! (rand() % 2) ? EIA608_CHAR_BOX_DRAWINGS_LIGHT_DOWN_AND_LEFT : EIA608_CHAR_BOX_DRAWINGS_LIGHT_UP_AND_LEFT,
             EIA608_CHAR_EIGHTH_NOTE, EIA608_CHAR_EIGHTH_NOTE);
}

void write_amfcaptions_708 (FILE* out, uint32_t timestamp, const char* text)
{
    sei_t sei;
    flvtag_t tag;
    sei_message_t* msg;
    caption_frame_t frame;
    sei_init (&sei);
    flvtag_init (&tag);
    caption_frame_init (&frame);
    caption_frame_from_text (&frame, text);
    sei_from_caption_frame (&sei, &frame);
    // caption_frame_dump (&frame);

    for (msg = sei_message_head (&sei); msg; msg=sei_message_next (msg),++timestamp) {
        flvtag_amfcaption_708 (&tag,timestamp,msg);
        flv_write_tag (out,&tag);
    }

    sei_free (&sei);
    flvtag_free (&tag);
}


void write_amfcaptions_utf8 (FILE* out, uint32_t timestamp, const utf8_char_t* text)
{
    flvtag_t tag;
    flvtag_init (&tag);
    flvtag_amfcaption_utf8 (&tag,timestamp,text);
    flv_write_tag (out,&tag);
    flvtag_free (&tag);
}

int main (int argc, char** argv)
{
    flvtag_t tag;
    uint32_t nextParty = 1000;
    int has_audio, has_video;
    FILE* flv = flv_open_read (argv[1]);
    FILE* out = flv_open_write (argv[2]);
    char partyDudes[64];

    flvtag_init (&tag);

    if (!flv_read_header (flv,&has_audio,&has_video)) {
        fprintf (stderr,"%s is not an flv file\n", argv[1]);
        return EXIT_FAILURE;
    }

    flv_write_header (out,has_audio,has_video);

    while (flv_read_tag (flv,&tag)) {

        if (flvtag_avcpackettype_nalu == flvtag_avcpackettype (&tag) && nextParty <= flvtag_timestamp (&tag)) {
            get_dudes (partyDudes);

            if (CAPTION_METHOD == CAPTION_METHOD_SEI_708) {
                flvtag_addcaption (&tag, partyDudes);
            } else if (CAPTION_METHOD == CAPTION_METHOD_AMF_708) {
                write_amfcaptions_708 (out,nextParty,partyDudes);
            } else if (CAPTION_METHOD == CAPTION_METHOD_AMF_708) {
                write_amfcaptions_utf8 (out, nextParty, partyDudes);
            } else {
                fprintf (stderr,"Unknnow method\n");
                return EXIT_FAILURE;

            }

            fprintf (stderr,"%d: %s\n",nextParty, partyDudes);
            nextParty += 500; // party all the time
        }

        flv_write_tag (out,&tag);
    }

    return EXIT_SUCCESS;
}
