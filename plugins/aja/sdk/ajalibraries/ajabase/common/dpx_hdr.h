/* SPDX-License-Identifier: MIT */
/**
	@file		dpx_hdr.h
	@brief		Declaration of DpxHdr class adapted from STwo's dpx file I/O.
	@copyright	(C) 2015-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef DPX_HDR_H
#define DPX_HDR_H

#include "public.h"
#include "ajabase/common/types.h"
#include "ajabase/system/system.h"

#include <string>


#define DPX_C_MAGIC           0x53445058  /**< little-endian magic number */
#define DPX_C_MAGIC_BE        0x58504453  /**< big-endian magic number */
#define DPX_C_VERSION         "V1.0"
#define DPX_C_UNDEFINED_FLOAT 0xffffffff  /**< DPX undefined float */

#define SWAP16(e,v) ((e) ? htons(v) : (v))
#define SWAP32(e,v) ((e) ? htonl(v) : (v))
#define SWAPR32(e,v) ((e) ? htonf(v) : (v))
#define UNSWAP16(e,v) ((e) ? ntohs(v) : (v))
#define UNSWAP32(e,v) ((e) ? ntohl(v) : (v))
#define UNSWAPR32(e,v) ((e) ? ntohf(v) : (v))

#define DPX_VALID(p) \
    ((p) && (((p)->file_info.magic_num==DPX_C_MAGIC) || \
             ((p)->file_info.magic_num==DPX_C_MAGIC_BE)))

#define DPX_IS_BE(p) \
    ((p) && ((p)->file_info.magic_num == DPX_C_MAGIC_BE))

#define DPX_C_IMAGE_ELEM_DESC_RGB   50
#define DPX_C_IMAGE_ELEM_DESC_RGBA  51
#define DPX_C_IMAGE_ELEM_DESC_ARGB  52
#define DPX_C_IMAGE_ELEM_DESC_422  100
#define DPX_C_IMAGE_ELEM_DESC_444  102


typedef struct dpx_file_info_struct
{
    uint32_t magic_num;    /* magic number 0x53445058 (SDPX) or 0x58504453 (XPDS) */
    uint32_t offset;       /* offset to image data in bytes */
    uint8_t  vers[8];         /* which header format version is being used (v1.0)*/
    uint32_t file_size;    /* file size in bytes */
    uint32_t ditto_key;    /* read time short cut - 0 = same, 1 = new */
    uint32_t gen_hdr_size; /* generic header length in bytes */
    uint32_t ind_hdr_size; /* industry header length in bytes */
    uint32_t user_data_size;/* user-defined data length in bytes */
    uint8_t  file_name[100];   /* iamge file name */
    uint8_t  create_time[24];  /* file creation date "yyyy:mm:dd:hh:mm:ss:LTZ" */
    uint8_t  creator[100];     /* file creator's name */
    uint8_t  project[200];     /* project name */
    uint8_t  copyright[200];   /* right to use or copyright info */
    uint32_t key;           /* encryption ( FFFFFFFF = unencrypted ) */
    union 
    {
        uint8_t  Reserved[104];  /* reserved field TBD (need to pad) */
        struct 
        {
            uint32_t tcFrame;    /* Timecode as a frame number       */
            uint32_t filenum;    /* FileNumber of preallocated files */
            uint8_t  rid;
            uint8_t  sid;
            uint8_t  tid;
            uint8_t  pad;
        } s2;
    } resv;
} DPX_file_info_t;


typedef struct dpx_image_element_struct
{
    uint32_t data_sign;        /* data sign (0 = unsigned, 1 = signed ) */
                               /* "Core set images are unsigned" */
    uint32_t ref_low_data;     /* reference low data code value */
    float ref_low_quantity;    /* reference low quantity represented */
    uint32_t ref_high_data;    /* reference high data code value */
    float ref_high_quantity;   /* reference high quantity represented */
    uint8_t  descriptor;       /* descriptor for image element */
    uint8_t  transfer;         /* transfer uint8_tacteristics for element */
    uint8_t  colorimetric;     /* colormetric specification for element */
    uint8_t  bit_size;         /* bit size for element */
    uint16_t packing;          /* packing for element */
    uint16_t encoding;         /* encoding for element */
    uint32_t data_offset;      /* offset to data of element */
    uint32_t eol_padding;      /* end of line padding used in element */
    uint32_t eo_image_padding; /* end of image padding used in element */
    uint8_t  description[32];  /* description of element */
} DPX_image_element_t;


typedef struct dpx_image_info_struct 
{
    uint16_t orientation;              /* image orientation */
    uint16_t element_number;           /* number of image elements */
    uint32_t pixels_per_line;          /* or x value */
    uint32_t lines_per_image_ele;      /* or y value, per element */
    DPX_image_element_t image_element[8];
    uint8_t  reserved[52];             /* reserved for future use (padding) */
} DPX_image_info_t;



typedef struct dpx_image_source_struct
{
    uint32_t x_offset;            /* X offset */
    uint32_t y_offset;            /* Y offset */
    float x_center;               /* X center */
    float y_center;               /* Y center */
    uint32_t x_orig_size;         /* X original size */
    uint32_t y_orig_size;         /* Y original size */
    uint8_t  file_name[100];      /* source image file name */
    uint8_t  creation_time[24];   /* source image creation date and time */
    uint8_t  input_dev[32];       /* input device name */
    uint8_t  input_serial[32];    /* input device serial number */
    uint16_t border[4];           /* border validity (XL, XR, YT, YB) */
    uint32_t pixel_aspect[2];     /* pixel aspect ratio (H:V) */
    uint8_t  reserved[28];        /* reserved for future use (padding) */
} DPX_image_source_t;



typedef struct dpx_motion_picture_film_header_struct
{
    uint8_t  film_mfg_id[2];    /* film manufacturer ID code (2 digits from film edge code) */
    uint8_t  film_type[2];      /* file type (2 digits from film edge code) */
    uint8_t  offset[2];         /* offset in perfs (2 digits from film edge code)*/
    uint8_t  prefix[6];         /* prefix (6 digits from film edge code) */
    uint8_t  count[4];          /* count (4 digits from film edge code)*/
    uint8_t  format[32];        /* format (i.e. academy) */
    uint32_t frame_position;    /* frame position in sequence */
    uint32_t sequence_len;      /* sequence length in frames */
    uint32_t held_count;        /* held count (1 = default) */
    float frame_rate;        /* frame rate of original in frames/sec */
    float shutter_angle;     /* shutter angle of camera in degrees */
    uint8_t  frame_id[32];      /* frame identification (i.e. keyframe) */
    uint8_t  slate_info[100];   /* slate information */
    uint8_t  reserved[56];      /* reserved for future use (padding) */
} DPX_film_t;




typedef struct dpx_television_header_struct
{
    uint32_t tim_code;            /* SMPTE time code */
    uint32_t userBits;            /* SMPTE user bits */
    uint8_t  interlace;           /* interlace ( 0 = noninterlaced, 1 = 2:1 interlace*/
    uint8_t  field_num;           /* field number */
    uint8_t  video_signal;        /* video signal standard (table 4)*/
    uint8_t  unused;              /* used for byte alignment only */
    float hor_sample_rate;     /* horizontal sampling rate in Hz */
    float ver_sample_rate;     /* vertical sampling rate in Hz */
    float frame_rate;          /* temporal sampling rate or frame rate in Hz */
    float time_offset;         /* time offset from sync to first pixel */
    float gamma;               /* gamma value */
    float black_level;         /* black level code value */
    float black_gain;          /* black gain */
    float break_point;         /* breakpoint */
    float white_level;         /* reference white level code value */
    float integration_times;   /* integration time(s) */
    uint8_t  reserved[76];        /* reserved for future use (padding) */
} DPX_television_t;


typedef struct dpx_header_struct
{
    DPX_file_info_t    file_info;
    DPX_image_info_t   image_info;
    DPX_image_source_t image_source;
    DPX_film_t         film_header;
    DPX_television_t   tv_header;
#ifdef NTV4_RAID_SUPPORT
	uint8_t			   reserved[2048];
#endif
} DPX_header_t;


uint32_t AJA_EXPORT dpx_get_u32(const uint32_t *ptr, bool BE );
void     AJA_EXPORT dpx_set_u32(uint32_t *ptr, bool BE, uint32_t val );
uint16_t AJA_EXPORT dpx_get_u16(const uint16_t *ptr, bool BE );
void     AJA_EXPORT dpx_set_u16(uint16_t *ptr, bool BE, uint16_t val );
float    AJA_EXPORT dpx_get_r32(const float *ptr, bool BE );
void     AJA_EXPORT dpx_set_r32(float *ptr, bool BE, float val );


#define FLD_OFFET( TYPE,field) \
   ((uint32_t)(&(((TYPE *)0)->field)))
 
#define DPX_GET_U32(hdr,fld) \
    DPX_VALID(hdr) ? \
        dpx_get_u32(&(hdr)->fld,DPX_IS_BE(hdr)) : \
        (uint32_t)(-1)

#define DPX_SET_U32(hdr,fld,val) \
    do { \
        if ( DPX_VALID(hdr)) \
            dpx_set_u32(&(hdr)->fld,DPX_IS_BE(hdr),val); \
       } while(0)


#define DPX_GET_R32(hdr,fld) \
    DPX_VALID(hdr) ? \
        dpx_get_r32(&(hdr)->fld,DPX_IS_BE(hdr)) : \
        (float)(0xffffffff)

#define DPX_SET_R32(hdr,fld,val) \
    do { \
        if ( DPX_VALID(hdr)) \
            dpx_set_r32(&(hdr)->fld,DPX_IS_BE(hdr),val); \
       } while(0)

#define DPX_GET_U16(hdr,fld) \
    DPX_VALID(hdr) ? \
        dpx_get_u16(&(hdr)->fld,DPX_IS_BE(hdr)) : \
        (uint16_t)(-1)

#define DPX_SET_U16(hdr,fld,val) \
    do { \
        if ( DPX_VALID(hdr)) \
            dpx_set_u16(&(hdr)->fld,DPX_IS_BE(hdr),val); \
       } while(0)


#define DPX_GET_U8(hdr,fld) \
    DPX_VALID(hdr) ? (hdr)->fld : (uint8_t)(-1)


#define DPX_SET_U8(hdr,fld,val) \
    if ( DPX_VALID(hdr)) \
        (hdr)->fld = val

#define DPX_SET_TEXT(hdr,fld,buf,len) \
    if ( DPX_VALID(hdr)) \
        memcpy((void*)&((hdr)->fld),(const void*)buf,len)


#define DPX_GET_TEXT(hdr,fld,buf,len) \
    DPX_VALID(hdr) ?  \
        memcpy((void*)buf,(const void*)&((hdr)->fld),len) : \
        memset((void*)buf,0xff,len)


class AJA_EXPORT DpxHdr
{
public:
    DpxHdr(void) { init(DPX_C_MAGIC_BE); }
	~DpxHdr();

    DpxHdr& operator=(const DpxHdr& rhs);

    void  init(uint32_t endianness);
    bool  valid() const;

	bool IsBigEndian() { return DPX_IS_BE(&m_hdr); }

	std::string get_fi_time_stamp() const;
    void   set_fi_time_stamp();

    void   set_s2_tcframe(uint32_t frame);
    size_t get_s2_tcframe() const;

    uint32_t  get_s2_filenumber() const;
    void   set_s2_filenumber(uint32_t fr);

    void   set_s2_ids(uint8_t rid, uint8_t sid, uint8_t tid);
    void   get_s2_ids(uint8_t *rid, uint8_t *sid, uint8_t *tid) const;

    size_t get_file_size();
    size_t get_generic_hdr_size();
    size_t get_industry_hdr_size();
    size_t get_user_hdr_size();

    // File Info access
    //
    size_t get_fi_image_offset() const;
    void set_fi_image_offset(size_t offs );

    size_t get_fi_file_size() const;
    void set_fi_file_size(size_t offs );

    std::string get_fi_version() const;
    void   set_fi_version(const std::string& str);

    std::string get_fi_file_name() const;
    void   set_fi_file_name(const std::string& str);

    std::string get_fi_creator() const;
    void   set_fi_creator(const std::string& str);

    std::string get_fi_create_time() const;
    void   set_fi_create_time(const std::string& str);

    std::string get_fi_project() const;
    void   set_fi_project(const std::string& str);

    std::string get_fi_copyright() const;
    void   set_fi_copyright(const std::string& str);

    // Image Info
    void set_ii_orientation(uint16_t orientation );
    uint16_t get_ii_orientation() const;

    void set_ii_element_number(uint16_t element_number );
    uint16_t get_ii_element_number() const;

    void set_ii_pixels(uint32_t pixels );
    size_t get_ii_pixels() const;

    void set_ii_lines(uint32_t lines );
    size_t get_ii_lines(void ) const;

    size_t get_ii_image_size()
    {
      // Note: Incorrect for some configs
		uint32_t expectedSize = -1;
		if (get_ie_descriptor() == 50)
		{
                        if ( get_ie_bit_size() == 10 )
                            expectedSize = (uint32_t)(get_ii_pixels() * get_ii_lines() * 4);
                        else
                            expectedSize = (uint32_t)(get_ii_pixels() * get_ii_lines() * 6); // 16 bit
                }
		else if (get_ie_descriptor() == 100)
		{
                        if (get_ii_pixels()  % 48 == 0)
                            expectedSize = (uint32_t)(get_ii_pixels() * get_ii_lines() * 8 / 3);
                        else
                            expectedSize = (uint32_t)(((get_ii_pixels()/48+1)*48) * get_ii_lines() * 8 / 3);
                }
//                rowBytes = (( width % 48 == 0 ) ? width : (((width / 48 ) + 1) * 48)) * 8 / 3;

   
      if (0 == get_fi_file_size())
	  {
        return (0);
	  }
 
	  uint32_t sizeInFile = (uint32_t)(get_fi_file_size() - get_fi_image_offset());

	  if ( sizeInFile < expectedSize)
		  return sizeInFile;
	  else
		  return expectedSize;

    }

    // Image Element
    void set_ie_data_sign (uint32_t sign, int i=0);
    size_t get_ie_data_sign(int i = 0);

    void set_ie_ref_low_data(uint32_t data, int i=0);
    size_t get_ie_ref_low_data(int i = 0) const;

    void set_ie_ref_high_data(uint32_t data, int i=0);
    size_t get_ie_ref_high_data(int i = 0) const;

    void set_ie_ref_low_quantity(float data, int i=0);
    float get_ie_ref_low_quantity(int i = 0) const;

    void set_ie_ref_high_quantity(float data, int i=0);
    float get_ie_ref_high_quantity(int i = 0) const;

    void set_ie_descriptor(uint8_t desc, int i = 0);
    uint8_t get_ie_descriptor(int i = 0) const;

    void set_ie_transfer(uint8_t trans, int i = 0);
    uint8_t get_ie_transfer(int i = 0) const;

    void set_ie_colorimetric(uint8_t c, int i = 0);
    uint8_t get_ie_colorimetric(int i = 0) const;

    void set_ie_bit_size(uint8_t bits, int i = 0);
    uint8_t get_ie_bit_size(int i = 0) const;

    void set_ie_packing(uint16_t pack, int i = 0);
    uint16_t get_ie_packing (int i = 0) const;

    void set_ie_encoding(uint16_t enc, int i = 0);
    uint16_t get_ie_encoding(int i = 0) const;

    void set_ie_data_offset(uint32_t offs, int i = 0);
    uint32_t get_ie_data_offset (int i = 0) const;

    void set_ie_eol_padding(uint32_t padding, int i = 0);
    uint32_t get_ie_eol_padding(int i = 0) const;

    void set_ie_eo_image_padding(uint32_t padding, int i = 0);
    uint32_t get_ie_eo_image_padding(int i = 0) const;

    std::string get_ie_description(int i=0) const;
    void   set_ie_description(const std::string& str, int i = 0);

    // Image Source access.
    std::string get_is_filename() const;
    void   set_is_filename(const std::string& str);

    std::string get_is_creation_time() const;
    void   set_is_creation_time(const std::string& str);

    std::string get_is_input_device() const;
    void   set_is_input_device(const std::string& str);

    std::string get_is_input_serial() const;
    void   set_is_input_serial(const std::string& str);

    // Film header
    std::string get_film_mfg_id() const;
    void   set_film_mfg_id(const std::string& str);

    std::string get_film_type() const;
    void   set_film_type(const std::string& str);

    std::string get_film_offset() const;
    void   set_film_offset(const std::string& str);

    std::string get_film_prefix() const;
    void        set_film_prefix(const std::string& str);

    std::string get_film_count() const;
    void   set_film_count(const std::string& str);

    std::string get_film_format() const;
    void   set_film_format(const std::string& str);

    size_t get_film_frame_position() const; 
    void   set_film_frame_position(size_t pos ); 

    size_t get_film_sequence_len() const; 
    void   set_film_sequence_len(size_t len ); 

    size_t get_film_held_count() const; 
    void   set_film_held_count(size_t count ); 

    float  get_film_frame_rate () const; 
    void   set_film_frame_rate (float len ); 

    float  get_film_shutter_angle() const;
    void   set_film_shutter_angle(float );

    std::string get_film_frame_id() const;
    void   set_film_frame_id(const std::string& str);

    std::string get_film_slate_info() const;
    void   set_film_slate_info(const std::string& str);

    // TV header
    size_t get_tv_timecode() const;
    void   set_tv_timecode(size_t tc); 

    size_t get_tv_userbits() const;
    void   set_tv_userbits(size_t tc); 

    float  get_tv_frame_rate() const;
    void   set_tv_frame_rate(float fr);

	uint8_t get_tv_interlace() const;
	void    set_tv_interlace(uint8_t interlace);

	const DPX_header_t& GetHdr(void) const
	{
		return (m_hdr);
	}

	DPX_header_t& GetHdr(void)
	{
		return (m_hdr);
	}

	size_t GetHdrSize(void) const
	{
		return (sizeof(DPX_header_t));
	}

private:
    DPX_header_t m_hdr;
};


#endif // DPX_HDR_H
