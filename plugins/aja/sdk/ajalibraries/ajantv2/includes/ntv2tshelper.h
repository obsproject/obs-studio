/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2tshelper.h
	@brief		Declares Transport Stream helper classes.
	@copyright	(C) 2008-2021 AJA Video Systems, Inc.
**/

#ifndef NTV2TSHELPER_H
#define NTV2TSHELPER_H

#include <stdint.h>
#include <map>

#define KIPDPRINT                       0

#if defined(MSWindows)

    #if (KIPDPRINT==0)
        // no log
        #define kipdprintf(...)

    #elif (KIPDPRINT==1)
        // printf
        #include <stdio.h>
        #define kipdprintf(...) printf(__VA_ARGS__)

    #endif

#elif defined(AJALinux)

    #if (KIPDPRINT==0)
        // no log
        #define kipdprintf(...)

    #elif (KIPDPRINT==1)
        // printf
        #include <stdio.h>
        #define kipdprintf(...) printf(__VA_ARGS__)

    #endif

#elif defined(AJAMac)

    #if (KIPDPRINT==0)
        // no log
        #define kipdprintf(_format_...)

    #elif (KIPDPRINT==1)
        // printf
        #include <stdio.h>
        #define kipdprintf(_format_...) printf(_format_)
    #endif

#endif


typedef enum
{
    kJ2KStreamTypeStandard,
    kJ2KStreamTypeNonElsm
} J2KStreamType;

typedef enum
{
    kJ2KChromaSubSamp_444,
    kJ2KChromaSubSamp_422_444,
    kJ2KChromaSubSamp_422_Standard
} J2KChromaSubSampling;

typedef enum
{
    kJ2KCodeBlocksize_32x32,
    kJ2KCodeBlocksize_32x64,
    kJ2KCodeBlocksize_64x32 = 4,
    kJ2KCodeBlocksize_64x64,
    kJ2KCodeBlocksize_128x32 = 12
} J2KCodeBlocksize;

typedef enum
{
    kTsEncapTypeJ2k,
    kTsEncapTypePcr,
    kTsEncapTypeAes
} TsEncapType;

typedef struct TsEncapStreamData
{
    J2KStreamType   j2kStreamType;
    uint32_t        width;
    uint32_t        height;
    uint32_t        denFrameRate;
    uint32_t        numFrameRate;
    uint32_t        numAudioChannels;
    uint32_t        programPid;
    uint32_t        videoPid;
    uint32_t        pcrPid;
    uint32_t        audio1Pid;
    bool            interlaced;
} TsEncapStreamData;


typedef struct TsVideoStreamData
{
    J2KStreamType   j2kStreamType;
    uint32_t        width;
    uint32_t        height;
    uint32_t        denFrameRate;
    uint32_t        numFrameRate;
    uint32_t        numAudioChannels;
    bool            interlaced;
} TsVideoStreamData;

class TSGenerator
{
    public:
        // Input
        uint16_t    _tsId;
        uint8_t     _version;
        uint32_t    _tableLength;
        TsEncapType _tsEncapType;

        // Generated packet
        uint8_t     _pkt8[188];
        uint32_t    _pkt32[188];

    public:
        TSGenerator()
        {
            init();
            initPacket();
        }

        ~TSGenerator()
        {
        }

        void init()
        {
            _tsId = 1;
            _version = 1;
            _tableLength = 0;
            _tsEncapType = kTsEncapTypeJ2k;
        }

        void initPacket()
        {
            for ( int i = 0; i < 188; i++ )
            {
                _pkt8[i] = 0xff;
                _pkt32[i] = 0xffff;
            }
        }

        uint32_t chksum_crc32(unsigned char *data, int len)
        {
            uint32_t crc_table[256] =
            {
                0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
                0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
                0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
                0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
                0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
                0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
                0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
                0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
                0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
                0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
                0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
                0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
                0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
                0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
                0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
                0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
                0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
                0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
                0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
                0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
                0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
                0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
                0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
                0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
                0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
                0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
                0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
                0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
                0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
                0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
                0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
                0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
                0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
                0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
                0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
                0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
                0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
                0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
                0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
                0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
                0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
                0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
                0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
            };

            int32_t i;
            uint32_t crc = 0xffffffff;

            for (i=0; i<len; i++)
                crc = (crc << 8) ^ crc_table[((crc >> 24) ^ *data++) & 0xff];

            return crc;
        }

        void dump8()
        {
            for (uint32_t i=0; i<_tableLength; i++)
            {
                if (i % 16 == 15) {
                    kipdprintf("0x%02x\n", _pkt8[i]);
				} else {
                    kipdprintf("0x%02x, ", _pkt8[i]);
				}
            }
            kipdprintf("\n\n");
        }

        void dump32()
        {
            for (uint32_t i=0; i<_tableLength; i++)
            {
                if (i % 16 == 15) {
                    kipdprintf("0x%04x\n", _pkt32[i]);
				} else {
                    kipdprintf("0x%04x ", _pkt32[i]);
				}
            }
            kipdprintf("\n\n");
        }

    protected:
        void put16( uint16_t val, int &pos )
        {
            _pkt8[pos++] = (uint8_t)(val>>8);
            _pkt8[pos++] = (uint8_t)val;
        }

        void put32( uint32_t val, int &pos )
        {
            _pkt8[pos++] = (uint8_t)(val>>24);
            _pkt8[pos++] = (uint8_t)(val>>16);
            _pkt8[pos++] = (uint8_t)(val>>8);
            _pkt8[pos++] = (uint8_t)val;
        }
};

class PESGen : public TSGenerator
{
public:
    TsVideoStreamData   _videoStreamData;
    std::map <uint16_t, uint16_t> _elemNumToPID;
    uint64_t _pts;                              // these can be passed in and will be initialized to reaonsable values
    int32_t _auf1;
    int32_t _auf2;
    uint32_t _bitRate;
    int32_t _ptsOffset;                         // theses values are filled in by the generator
    int32_t _j2kTsOffset;
    int32_t _auf1Offset;
    int32_t _auf2Offset;
    int32_t _hh;
    int32_t _mm;
    int32_t _ss;
    int32_t _ff;


public:
    PESGen()
    {
        initLocal();
    }

    ~PESGen()
    {
    }

    void initLocal()
    {
        _elemNumToPID.clear();
        _pts = 0;
        _auf1 = 0;
        _auf2 = 0;
        _bitRate = 75000000;
        _hh = 0;
        _mm = 0;
        _ss = 0;
        _ff = 0;
    }

    int makePacket()
    {
        initPacket();
        int pos = 0;
        _ptsOffset = 0xff;                                              // For non-elsm streams these are all 0xff
        _j2kTsOffset = 0xff;                                            // for standard streams they will be filled in accordingly
        _auf1Offset = 0xff;
        _auf2Offset = 0xff;

        // Header
        _pkt8[pos++] = 0x47;                                            // sync byte
        _pkt8[pos] = 1<<6;                                              // payload unit start indicator
        _pkt8[pos++] |= (uint8_t) ((_elemNumToPID[1] >> 8) & 0x1f);     // PID for Video
        _pkt8[pos++] =  (uint8_t) (_elemNumToPID[1] & 0xff);            // PID for Video
        _pkt8[pos++] = 0x10;                                            // Continuity Counter must increment when transmitted

        // generate PES data for AES streams
        if (_tsEncapType == kTsEncapTypeAes)
        {
            _pkt8[pos++] = 0;                                           // packet_start_code_prefix
            _pkt8[pos++] = 0;
            _pkt8[pos++] = (uint8_t) (1 & 0xff);
            _pkt8[pos++] = (uint8_t) (0xbd);

            _pkt8[pos++] = 0;                                           // (packet_length >> 8) & 0xff
            _pkt8[pos++] = 0;                                           // packet_length & 0xff
            _pkt8[pos++] = 0x80;                                        // alignment
            _pkt8[pos++] = 0x80;                                        // 11
            _pkt8[pos++] = 5;                                           // 12

            _ptsOffset = pos;

            _pkt8[pos] = 0x21;                                          // 13
            _pkt8[pos++] |= (uint8_t) ((_pts >> 29) & 0xe);
            _pkt8[pos] = 0x0;                                           // 14
            _pkt8[pos++] |= (uint8_t) ((_pts >> 22) & 0xff);
            _pkt8[pos] = 0x1;                                           // 15
            _pkt8[pos++] |= (uint8_t) ((_pts >> 14) & 0xfe);
            _pkt8[pos] = 0x0;                                           // 16
            _pkt8[pos++] |= (uint8_t) ((_pts >> 7) & 0xff);
            _pkt8[pos] = 0x1;                                           // 17
            _pkt8[pos++] |= (uint8_t) ((_pts << 1) & 0xfe);

            _pkt8[pos++] = 0x0;                                         // 18
            _pkt8[pos++] = 0x0;

            // These two bytes are defined in Table 1 of ST302 spec starting with num channels
            if (_videoStreamData.j2kStreamType == kJ2KStreamTypeStandard)
            {
                _pkt8[pos++] = 0x0;                                     // 2 channels, 6 bits of channel ID
                _pkt8[pos++] = 0x10;                                    // two bits of channel ID, 20 bits per sample, alignment 0 reserved
            }
            else
            {
                _pkt8[pos++] = 0x0;                                     // 2 channels, 6 bits of channel ID
                _pkt8[pos++] = 0x20;                                    // two bits of channel ID, 24 bits per sample, alignment 0 reserved
            }

            _auf1Offset = 0x1000012;
            _auf2Offset = 0x1000c08;

        }
        else
        {
            // generate PES data for standard streams, for non-elsm just do the header
            if (_videoStreamData.j2kStreamType == kJ2KStreamTypeStandard)
            {
                _pkt8[pos++] = 0;                                           // packet_start_code_prefix
                _pkt8[pos++] = 0;
                _pkt8[pos++] = (uint8_t) (1 & 0xff);
                _pkt8[pos++] = (uint8_t) (0xbd);

                _pkt8[pos++] = 0;                                           // (packet_length >> 8) & 0xff
                _pkt8[pos++] = 0;                                           // packet_length & 0xff
                _pkt8[pos++] = 0x84;                                        // alignment
                _pkt8[pos++] = 0x80;                                        // 11
                _pkt8[pos++] = 5;                                           // 12

                _ptsOffset = pos;

                _pkt8[pos] = 0x21;                                          // 13
                _pkt8[pos++] |= (uint8_t) ((_pts >> 29) & 0xe);
                _pkt8[pos] = 0x0;                                           // 14
                _pkt8[pos++] |= (uint8_t) ((_pts >> 22) & 0xff);
                _pkt8[pos] = 0x1;                                           // 15
                _pkt8[pos++] |= (uint8_t) ((_pts >> 14) & 0xfe);
                _pkt8[pos] = 0x0;                                           // 16
                _pkt8[pos++] |= (uint8_t) ((_pts >> 7) & 0xff);
                _pkt8[pos] = 0x1;                                           // 17
                _pkt8[pos++] |= (uint8_t) ((_pts << 1) & 0xfe);

                _pkt8[pos++] = 0x65;                                        // "e"
                _pkt8[pos++] = 0x6c;                                        // "l"
                _pkt8[pos++] = 0x73;                                        // "s"
                _pkt8[pos++] = 0x6D;                                        // "m"

                _pkt8[pos++] = 0x66;                                        // "f"
                _pkt8[pos++] = 0x72;                                        // "r"
                _pkt8[pos++] = 0x61;                                        // "a"
                _pkt8[pos++] = 0x74;                                        // "t"

                _pkt8[pos++] = (uint8_t) ((_videoStreamData.denFrameRate >> 8) & 0xff);
                _pkt8[pos++] = (uint8_t) (_videoStreamData.denFrameRate & 0xff);
                _pkt8[pos++] = (uint8_t) ((_videoStreamData.numFrameRate >> 8) & 0xff);
                _pkt8[pos++] = (uint8_t) (_videoStreamData.numFrameRate & 0xff);

                _pkt8[pos++] = 0x62;                                        // "b"
                _pkt8[pos++] = 0x72;                                        // "r"
                _pkt8[pos++] = 0x61;                                        // "a"
                _pkt8[pos++] = 0x74;                                        // "t"

                _pkt8[pos++] = (uint8_t) (_bitRate >> 24);                  // 34
                _pkt8[pos++] = (uint8_t) ((_bitRate >> 16) & 0xff);
                _pkt8[pos++] = (uint8_t) ((_bitRate >> 8) & 0xff);
                _pkt8[pos++] = (uint8_t) (_bitRate & 0xff);

                _auf1Offset = pos;

                _pkt8[pos++] = (uint8_t) (_auf1 >> 24);                     // 38
                _pkt8[pos++] = (uint8_t) ((_auf1 >> 16) & 0xff);
                _pkt8[pos++] = (uint8_t) ((_auf1 >> 8) & 0xff);
                _pkt8[pos++] = (uint8_t) (_auf1 & 0xff);

                if (_videoStreamData.interlaced)
                {
                    _auf2Offset = pos;

                    _pkt8[pos++] = (uint8_t) (_auf2 >> 24);                 // 42
                    _pkt8[pos++] = (uint8_t) ((_auf2 >> 16) & 0xff);
                    _pkt8[pos++] = (uint8_t) ((_auf2 >> 8) & 0xff);
                    _pkt8[pos++] = (uint8_t) (_auf2 & 0xff);

                    _pkt8[pos++] = 0x66;                                    // "f"
                    _pkt8[pos++] = 0x69;                                    // "i"
                    _pkt8[pos++] = 0x65;                                    // "e"
                    _pkt8[pos++] = 0x6c;                                    // "l"

                    _pkt8[pos++] = (uint8_t) (2 & 0xff);
                    _pkt8[pos++] = (uint8_t) (1 & 0xff);

                    _pkt8[pos++] = 0x74;                                    // "t"
                    _pkt8[pos++] = 0x63;                                    // "c"
                    _pkt8[pos++] = 0x6f;                                    // "o"
                    _pkt8[pos++] = 0x64;                                    // "d"

                    _j2kTsOffset = pos;
                    _pkt8[pos++] = (uint8_t) (_hh & 0xff);
                    _pkt8[pos++] = (uint8_t) (_mm & 0xff);
                    _pkt8[pos++] = (uint8_t) (_ss & 0xff);
                    _pkt8[pos++] = (uint8_t) (_ff & 0xff);

                    _pkt8[pos++] = 0x62;                                    // "b"
                    _pkt8[pos++] = 0x63;                                    // "c"
                    _pkt8[pos++] = 0x6f;                                    // "o"
                    _pkt8[pos++] = 0x6c;                                    // "l"

                    _pkt8[pos++] = 3;
                    _pkt8[pos++] = 0x0;
                }
                else
                {
                    _pkt8[pos++] = 0x74;                                    // "t"
                    _pkt8[pos++] = 0x63;                                    // "c"
                    _pkt8[pos++] = 0x6f;                                    // "o"
                    _pkt8[pos++] = 0x64;                                    // "d"

                    _j2kTsOffset = pos;
                    _pkt8[pos++] = (uint8_t) (0 & 0xff);                    // hh
                    _pkt8[pos++] = (uint8_t) (0 & 0xff);                    // mm
                    _pkt8[pos++] = (uint8_t) (0 & 0xff);                    // ss
                    _pkt8[pos++] = (uint8_t) (0 & 0xff);                    // ff

                    _pkt8[pos++] = 0x62;                                    // "b"
                    _pkt8[pos++] = 0x63;                                    // "c"
                    _pkt8[pos++] = 0x6f;                                    // "o"
                    _pkt8[pos++] = 0x6c;                                    // "l"

                    _pkt8[pos++] = 3;
                    _pkt8[pos++] = 0xff;
                }
            }
        }

        _tableLength = pos;
        return pos;
    }

    int32_t calcPatPmtPeriod()
    {
        double d1, d2;

        // Next PAT / PMT Transmission Rate
        d2 = 1.0 / 125000000;               // Clock Period
        d1 = 90e-3 / d2 - 1.0;
        return (int32_t) d1;
    }
};

class PATGen : public TSGenerator
{
    public:
        // Input
        std::map <uint16_t, uint16_t> _progNumToPID;

    public:
        PATGen()
        {
            initLocal();
        }

        ~PATGen()
        {
        }

        void initLocal()
        {
            _progNumToPID.clear();
        }

        int makePacket()
        {
            initPacket();
            int pos = 0;

            // Header
            _pkt8[pos++] = 0x47;                                // sync byte
            _pkt8[pos++] = 1<<6;                                // payload unit start indicator
            _pkt8[pos++] = 0;                                   // PID for PAT = 0
            _pkt8[pos++] = 0x10;                                // Continuity Counter must increment when transmitted
            _pkt8[pos++] = 0;                                   // pointer

            int crcStart = pos;

            _pkt8[pos++] = 0;                                   // table id = 0

            int length = 9 + (4 * (int)_progNumToPID.size());
            put16( (uint16_t)0xb000 + (length & 0x3ff), pos);   // syntax indicator, reserved, length
            put16( _tsId, pos );

            _pkt8[pos++] = 0xc1 + ((_version & 0x1f)<< 1);      // version, current/next
            put16( 0, pos );                                    // section number = last section number = 0

            std::map <uint16_t, uint16_t>::const_iterator it = _progNumToPID.begin();
            while (it != _progNumToPID.end())
            {
                put16( it->first, pos );
                put16( (uint16_t)0xe000 + (it->second & 0x1fff), pos );
                it++;
            }
            int crcEnd = pos - 1;

            int crc = chksum_crc32(_pkt8 + crcStart, crcEnd - crcStart + 1 );
            put32( crc, pos );

            _tableLength = pos;
            return pos;
        }
};

class PMTGen : public TSGenerator
{
    public:
        TsVideoStreamData   _videoStreamData;
        std::map <uint16_t, uint16_t> _progNumToPID;
        std::map <uint16_t, uint16_t> _pcrNumToPID;
        std::map <uint16_t, uint16_t> _videoNumToPID;
        std::map <uint16_t, uint16_t> _audioNumToPID;

    public:
        PMTGen()
        {
            initLocal();
        }

        ~PMTGen()
        {
        }

        void initLocal()
        {
            _progNumToPID.clear();
            _pcrNumToPID.clear();
            _videoNumToPID.clear();
            _audioNumToPID.clear();
        }

        int makePacket()
        {
            initPacket();
            int pos = 0;
            int len, lengthPos, j2kLengthPos, audioLengthPos;

            // Header
            _pkt8[pos++] = 0x47;                                            // sync byte
            _pkt8[pos] = 1<<6;                                              // payload unit start indicator
            _pkt8[pos++] |= (uint8_t) ((_progNumToPID[1] >> 8) & 0x1f);     // PID for PMT
            _pkt8[pos++] =  (uint8_t) (_progNumToPID[1] & 0xff);            // PID for PMT
            _pkt8[pos++] = 0x10;                                            // Continuity Counter must increment when transmitted
            _pkt8[pos++] = 0;                                               // pointer

            int crcStart = pos;

            _pkt8[pos++] = 2;                                               // table id = 0
            lengthPos = pos;                                                // need to come back and fill in length so save position (assume < 256)
            _pkt8[pos++] = 0xb0;
            pos++;

            put16(0x01, pos);                                               // program number

            _pkt8[pos++] = 0xc1 + ((_version & 0x1f)<< 1);                  // version, current/next
            put16( 0, pos );                                                // section number = last section number = 0

            _pkt8[pos] = 0xe0;                                              // PCR pid and reserved bits
            _pkt8[pos++] |= (uint8_t) ((_pcrNumToPID[1] >> 8) & 0x1f);
            _pkt8[pos++] =  (uint8_t) (_pcrNumToPID[1] & 0xff);

            _pkt8[pos++] = 0xf0;                                            // reserved bits and program info length
            _pkt8[pos++] = 0x00;                                            // reserved bits and program info length

            // Do the streams

            if (_videoNumToPID[1] != 0)
            {
                // J2K
                _pkt8[pos++] = 0x21;                                            // J2K Type
                _pkt8[pos] = 0xe0;                                              // elementary pid and reserved bits
                _pkt8[pos++] |= (uint8_t) ((_videoNumToPID[1] >> 8) & 0x1f);
                _pkt8[pos++] =  (uint8_t) (_videoNumToPID[1] & 0xff);

                j2kLengthPos = pos;                                             // need to come back and fill in descriptor length so save position
                pos+=2;

                len = makeJ2kDescriptor(pos);                                   // generate the J2K descriptor

                _pkt8[j2kLengthPos] = 0xf0;                                     // fill in the length and reserved bits now
                _pkt8[j2kLengthPos++] |= (uint8_t) ((len >> 8) & 0x1f);
                _pkt8[j2kLengthPos] =  (uint8_t) (len & 0xff);
            }

            if ((_audioNumToPID[1] != 0) && (_videoStreamData.numAudioChannels != 0))
            {
                // Audio
                _pkt8[pos++] = 0x06;                                            // Audio Type
                _pkt8[pos] = 0xe0;                                              // audio pid and reserved bits
                _pkt8[pos++] |= (uint8_t) ((_audioNumToPID[1] >> 8) & 0x1f);
                _pkt8[pos++] =  (uint8_t) (_audioNumToPID[1] & 0xff);

                audioLengthPos = pos;                                           // need to come back and fill in descriptor length so save position
                pos+=2;

                len = makeAudioDescriptor(pos);                                 // generate the audio descriptor

                _pkt8[audioLengthPos] = 0xf0;                                   // fill in the length and reserved bits now
                _pkt8[audioLengthPos++] |= (uint8_t) ((len >> 8) & 0x1f);
                _pkt8[audioLengthPos] =  (uint8_t) (len & 0xff);
            }

            // now we know the length so fill that in
            _pkt8[lengthPos] = 0xb0;
            _pkt8[lengthPos++] |= (uint8_t) (((pos-crcStart+1) >> 8) & 0x1f);
            _pkt8[lengthPos] =  (uint8_t) ((pos-crcStart+1) & 0xff);

            int crcEnd = pos - 1;

            int crc = chksum_crc32(_pkt8 + crcStart, crcEnd - crcStart + 1 );
            put32( crc, pos );

            _tableLength = pos;
            return pos;
        }

        int makeJ2kDescriptor(int &pos)
        {
            int         startPos = pos;
            uint32_t    profileLevel;
            uint32_t    maxBitRate;
            uint32_t    height;

            if (_videoStreamData.j2kStreamType == kJ2KStreamTypeStandard)
            {
                profileLevel    = 0x102;
                maxBitRate      = 160000000;
            }
            else
            {
                profileLevel    = 0x101;
                maxBitRate      = 213000000;
            }

            if (_videoStreamData.interlaced) height = _videoStreamData.height/2;
            else height = _videoStreamData.height;

            // Header
            _pkt8[pos++] = 0x32;                                            // descriptor tag
            _pkt8[pos++] = 24;                                              // descriptor length
            put16( profileLevel, pos );                                     // profile level

            if (_videoStreamData.j2kStreamType == kJ2KStreamTypeStandard)
            {
                // Standard stream
                put32( _videoStreamData.width, pos );                       // width
                put32( height, pos );                                       // height
                put32( maxBitRate, pos );                                   // max bit rate
                put32( 1250000, pos );                                      // max buffer size (constant for now)
                put16( _videoStreamData.denFrameRate, pos );                // frame rate
                put16( _videoStreamData.numFrameRate, pos );
                _pkt8[pos++] = 3;                                           // color spec
                _pkt8[pos] = _videoStreamData.interlaced <<6;               // interlaced and still mode
                _pkt8[pos++] |= 0x3F;                                       // reserved bits all 1's for standard streams
            }
            else
            {
                // Non-elsm stream
                put16( 0x0100, pos );
                put16( _videoStreamData.width, pos );                       // width
                put16( height, pos );                                       // height
                if (_videoStreamData.interlaced)
                    put16( height, pos );                                   // height again for interlaced
                else
                    put16( 0, pos );                                        // otherwise nothing
                put32( maxBitRate, pos );                                   // max bit rate (constant for now)
                _pkt8[pos++] = 0;
                _pkt8[pos++] = 0;
                _pkt8[pos++] = 0x05;
                _pkt8[pos++] = 0x33;
                put16( _videoStreamData.denFrameRate, pos );                // frame rate
                put16( _videoStreamData.numFrameRate, pos );
                _pkt8[pos++] = 0;
                _pkt8[pos++] = 0;
            }

            return pos-startPos;
        }

        int makeAudioDescriptor(int &pos)
        {
            int         startPos = pos;
            // Header
            _pkt8[pos++] = 0x0a;                                            // descriptor tag
            _pkt8[pos++] = 4;                                               // descriptor length
            _pkt8[pos++] = 0x45;                                            // "E"
            _pkt8[pos++] = 0x4e;                                            // "N"
            _pkt8[pos++] = 0x47;                                            // "G"
            _pkt8[pos++] = 0;

            _pkt8[pos++] = 0x05;                                            // descriptor tag
            if (_videoStreamData.j2kStreamType == kJ2KStreamTypeNonElsm)
                _pkt8[pos++] = 6;                                           // length non-elsm
            else
                _pkt8[pos++] = 4;                                           // length standard stream (omitted ST302 channel spec)

            _pkt8[pos++] = 0x42;                                            // "B"
            _pkt8[pos++] = 0x53;                                            // "S"
            _pkt8[pos++] = 0x53;                                            // "S"
            _pkt8[pos++] = 0x44;                                            // "D"

            // These two bytes are defined in Table 1 of ST302 spec starting with num channels, we dont add these for standard streams
            if (_videoStreamData.j2kStreamType == kJ2KStreamTypeNonElsm)
            {
                _pkt8[pos++] =  (_videoStreamData.numAudioChannels/2) - 1;  // number of audio pairs (0 is 1 pair of audio), 6 bits of channel ID
                _pkt8[pos++] = 0x20;                                        // two bits of channel ID, 24 bits per sample, alignment 0 reserved
            }

            return pos-startPos;
        }
};

class ADPGen : public TSGenerator
{
public:
    std::map <uint16_t, uint16_t> _elemNumToPID;

public:
    ADPGen()
    {
        initLocal();
    }

    ~ADPGen()
    {
    }

    void initLocal()
    {
        _elemNumToPID.clear();
    }

    int makePacket()
    {
        initPacket();
        int pos = 0;

        // Header
        _pkt32[pos++] = 0x47;                                           // sync byte
        _pkt32[pos++] = ((_elemNumToPID[1] >> 8) & 0x1f);               // PID for stream
        _pkt32[pos++] = (_elemNumToPID[1] & 0xff);

        if (_tsEncapType == kTsEncapTypePcr)
        {
            _pkt32[pos++] = (2 << 4);                                   // Continuity Counter must increment when transmitted
            _pkt32[pos++] = 0;                                          // pointer
            _pkt32[pos++] = 0x10;                                       // pointer

            _pkt32[pos++] = 0x800;                                      // PCR
            _pkt32[pos++] = 0x900;
            _pkt32[pos++] = 0xa00;
            _pkt32[pos++] = 0xb00;
            _pkt32[pos++] = 0xc00;
            _pkt32[pos++] = 0xd00;
        }
        else
        {
            _pkt32[pos++] = (3 << 4);                                   // Continuity Counter must increment when transmitted
            _pkt32[pos++] = 0;                                          // pointer
            _pkt32[pos++] = 0;                                          // pointer
        }

        _tableLength = pos;
        return pos;
    }
};

#endif
