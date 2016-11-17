#include "flv.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <librtmp/log.h>
#include <librtmp/rtmp.h>


int MyRTMP_Write (RTMP* r, const char* buf, int size)
{
    RTMPPacket* pkt = &r->m_write;
    char* enc;
    int s2 = size, ret, num;

    pkt->m_nChannel = 0x04;   /* source channel */
    pkt->m_nInfoField2 = r->m_stream_id;

    while (s2) {
        if (!pkt->m_nBytesRead) {
            if (size < 11) {
                /* FLV pkt too small */
                return 0;
            }

            if (buf[0] == 'F' && buf[1] == 'L' && buf[2] == 'V') {
                buf += 13;
                s2 -= 13;
            }

            pkt->m_packetType = *buf++;
            pkt->m_nBodySize = AMF_DecodeInt24 (buf);
            buf += 3;
            pkt->m_nTimeStamp = AMF_DecodeInt24 (buf);
            buf += 3;
            pkt->m_nTimeStamp |= *buf++ << 24;
            buf += 3;
            s2 -= 11;

            if ( ( (pkt->m_packetType == RTMP_PACKET_TYPE_AUDIO
                    || pkt->m_packetType == RTMP_PACKET_TYPE_VIDEO) &&
                    !pkt->m_nTimeStamp) || pkt->m_packetType == RTMP_PACKET_TYPE_INFO) {
                pkt->m_headerType = RTMP_PACKET_SIZE_LARGE;
            } else {
                pkt->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
            }

            if (!RTMPPacket_Alloc (pkt, pkt->m_nBodySize)) {
                RTMP_Log (RTMP_LOGDEBUG, "%s, failed to allocate packet", __FUNCTION__);
                return FALSE;
            }

            enc = pkt->m_body;
        } else {
            enc = pkt->m_body + pkt->m_nBytesRead;
        }

        num = pkt->m_nBodySize - pkt->m_nBytesRead;

        if (num > s2) {
            num = s2;
        }

        memcpy (enc, buf, num);
        pkt->m_nBytesRead += num;
        s2 -= num;
        buf += num;

        if (pkt->m_nBytesRead == pkt->m_nBodySize) {
            ret = RTMP_SendPacket (r, pkt, FALSE);
            RTMPPacket_Free (pkt);
            pkt->m_nBytesRead = 0;

            if (!ret) {
                return -1;
            }

            buf += 4;
            s2 -= 4;

            if (s2 < 0) {
                break;
            }
        }
    }

    return size+s2;
}

int main (int argc, const char** argv)
{
    FILE* flv;
    RTMP* rtmp;
    RTMPPacket rtmpPacket;

    flvtag_t tag;
    int32_t timestamp = 0;
    int has_audio, has_video;
    char* url = 0;

    if (2 >= argc) {
        fprintf (stderr,"Usage %s [input] [url]\n",argv[0]);
    }

    url = (char*) argv[2];
    flv = flv_open_read (argv[1]);

    if (! flv) {
        fprintf (stderr,"Could not open %s\n",argv[1]);
        return EXIT_FAILURE;
    }

    if (! flv_read_header (flv, &has_audio, &has_video)) {
        fprintf (stderr,"Not an flv file %s\n",argv[1]);
        return EXIT_FAILURE;
    }

    flvtag_init (&tag);
    rtmp = RTMP_Alloc();
    RTMP_Init (rtmp);
    fprintf (stderr,"Connecting to %s\n", url);
    RTMP_SetupURL (rtmp, url);
    RTMP_EnableWrite (rtmp);

    RTMP_Connect (rtmp, NULL);
    RTMP_ConnectStream (rtmp, 0);
    memset (&rtmpPacket, 0, sizeof (RTMPPacket));

    if (! RTMP_IsConnected (rtmp)) {
        fprintf (stderr,"RTMP_IsConnected() Error\n");
        return EXIT_FAILURE;
    }

    while (flv_read_tag (flv,&tag)) {
        if (! RTMP_IsConnected (rtmp) || RTMP_IsTimedout (rtmp)) {
            fprintf (stderr,"RTMP_IsConnected() Error\n");
            return EXIT_FAILURE;
        }

        if (flvtag_timestamp (&tag) > timestamp) {
            usleep (1000 * (flvtag_timestamp (&tag) - timestamp));
            timestamp = flvtag_timestamp (&tag);
        }

        MyRTMP_Write (rtmp, (const char*) flvtag_raw_data (&tag),flvtag_raw_size (&tag));

        // Handle RTMP ping and such
        fd_set sockset; struct timeval timeout = {0,0};
        FD_ZERO (&sockset); FD_SET (RTMP_Socket (rtmp), &sockset);
        register int result = select (RTMP_Socket (rtmp) + 1, &sockset, NULL, NULL, &timeout);

        if (result == 1 && FD_ISSET (RTMP_Socket (rtmp), &sockset)) {
            RTMP_ReadPacket (rtmp, &rtmpPacket);

            if (! RTMPPacket_IsReady (&rtmpPacket)) {
                fprintf (stderr,"Received RTMP packet\n");
                RTMP_ClientPacket (rtmp,&rtmpPacket);
                RTMPPacket_Free (&rtmpPacket);
            }
        }
    }

    return EXIT_SUCCESS;
}
