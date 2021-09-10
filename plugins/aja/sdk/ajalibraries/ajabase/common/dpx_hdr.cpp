/* SPDX-License-Identifier: MIT */
/**
	@file		dpx_hdr.cpp
	@brief		Implementation of DpxHdr class adapted from STwo's dpx file I/O.
	@copyright	(C) 2015-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ajabase/common/types.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <algorithm>
#include "dpx_hdr.h"

#if defined(AJA_LINUX) || defined(AJA_MAC)
#	include <arpa/inet.h>
#	include <algorithm>

	using std::min;

#	define _cpp_min min
#	define INVALID_HANDLE_VALUE -1
#endif

#if defined(AJA_WINDOWS)
	#include <WinSock2.h>
#endif

 
typedef union
{
    uint32_t i;
    float f;
} FloatSwap;


inline float htonf(float f)
{
    FloatSwap s;
    s.f = f;
    s.i = htonl(s.i);
    return s.f;
}


inline float ntohf(float f)
{
    FloatSwap s;

    s.f = f;
    s.i = ntohl(s.i);
    return s.f;
}


uint32_t dpx_get_u32(const uint32_t *ptr, bool BE) 
{
    assert(ptr);
    uint32_t val;
    val = *ptr;
    return SWAP32(BE, val);
}

void dpx_set_u32 (uint32_t *ptr, bool BE, uint32_t val) 
{
    assert(ptr);
    *ptr = SWAP32(BE, val);
}


uint16_t dpx_get_u16(const uint16_t *ptr, bool BE) 
{
    assert(ptr);
    uint16_t val;
    val = *ptr;
    return SWAP16(BE, val);
}


void dpx_set_u16(uint16_t *ptr, bool BE, uint16_t val) 
{
    assert(ptr);
    *ptr = SWAP16(BE, val);
}


void dpx_set_r32(float *ptr, bool BE, float val)
{
    assert(ptr);
    *ptr = SWAPR32(BE, val);
}


float dpx_get_r32(const float *ptr, bool BE)
{
    assert(ptr);
    float val;
    val = *ptr;
    return SWAPR32(BE,val);
}    




DpxHdr& DpxHdr::operator=(const DpxHdr& rhs)
{
    if (this != &rhs)
    {
        memcpy ((void*)&m_hdr,(void*) &rhs.m_hdr, sizeof(DPX_header_t));
    }
    return *this;
}


void DpxHdr::init(uint32_t endianness)
{
    m_hdr.file_info.magic_num = endianness;
}


bool DpxHdr::valid() const
{
    return DPX_VALID(&m_hdr);
}


void DpxHdr::set_s2_tcframe(uint32_t frame)
{
    DPX_SET_U32(&m_hdr,file_info.resv.s2.tcFrame, frame);
}


size_t DpxHdr::get_s2_tcframe() const
{
    return DPX_GET_U32(&m_hdr,file_info.resv.s2.tcFrame);
}


uint32_t DpxHdr:: get_s2_filenumber() const
{
    return DPX_GET_U32(&m_hdr,file_info.resv.s2.filenum);
}


void DpxHdr::set_s2_filenumber(uint32_t filenum)
{
    DPX_SET_U32(&m_hdr,file_info.resv.s2.filenum,filenum);
}


void DpxHdr::set_s2_ids(uint8_t rid, uint8_t sid, uint8_t tid)
{
    m_hdr.file_info.resv.s2.rid = rid;
    m_hdr.file_info.resv.s2.sid = sid;
    m_hdr.file_info.resv.s2.tid = tid;
}


void DpxHdr::get_s2_ids(uint8_t *rid, uint8_t *sid, uint8_t *tid) const
{
    assert(rid && sid && tid);

    *rid = m_hdr.file_info.resv.s2.rid;
    *sid = m_hdr.file_info.resv.s2.sid;
    *tid = m_hdr.file_info.resv.s2.tid;
}


void DpxHdr::set_fi_time_stamp()
{
#if defined(AJA_WINDOWS)
    time_t time_now;
    struct tm *t;
     
    time(&time_now);
	t = ::localtime(&time_now);
	 char tstr[32];
    strftime (tstr, 32, "%Y:%m:%d:%T%z", t);

    int len = (int)(std::min)(strlen(tstr)+1, (size_t)24);

    DPX_SET_TEXT(&m_hdr,file_info.create_time,tstr,len);
#endif
}

std::string DpxHdr::get_fi_time_stamp() const
{
    char buf[25];
    buf[0] = '\0';
    DPX_GET_TEXT (&m_hdr,file_info.create_time,buf,24);
    buf[24] = '\0';
    return buf;
}


size_t DpxHdr::get_fi_image_offset() const
{
    return DPX_GET_U32(&m_hdr, file_info.offset);
}


void DpxHdr::set_fi_image_offset(size_t offs)
{
    DPX_SET_U32(&m_hdr, file_info.offset, (uint32_t)offs);
}

size_t DpxHdr::get_fi_file_size() const
{
    return DPX_GET_U32(&m_hdr, file_info.file_size);
}


void DpxHdr::set_fi_file_size (size_t size)
{
    DPX_SET_U32(&m_hdr, file_info.file_size, (uint32_t)size);
}

std::string DpxHdr::get_fi_version() const
{
    char buf[9];
    buf[0] = '\0';
    DPX_GET_TEXT(&m_hdr,file_info.vers,buf,8);
    buf[8] = '\0';
    return buf;
}

void DpxHdr::set_fi_version(const std::string& str)
{
    int len = (int)(std::min)(str.size()+1, (size_t)8);
    DPX_SET_TEXT(&m_hdr,file_info.vers,str.c_str(), len);
}

std::string DpxHdr::get_fi_file_name() const
{
    char buf[101];
    buf[0] = '\0';
    DPX_GET_TEXT(&m_hdr,file_info.file_name,buf,100);
    buf[100] = '\0';
    return buf;
}

void DpxHdr::set_fi_file_name(const std::string& str)
{
    int len = (int)(std::min)(str.size()+1, (size_t)100);
    DPX_SET_TEXT(&m_hdr,file_info.file_name,str.c_str(), len);
}

std::string DpxHdr::get_fi_creator() const
{
    char buf[101];
    buf[0] = '\0';
    DPX_GET_TEXT(&m_hdr,file_info.creator,buf,100);
    buf[100] = '\0';
    return buf;
}

void DpxHdr::set_fi_creator(const std::string& str)
{
    int len = (int)(std::min) (str.size()+1, (size_t)100);
    DPX_SET_TEXT(&m_hdr,file_info.creator,str.c_str(), len);
}

std::string DpxHdr::get_fi_create_time () const
{
    char buf[25];
    buf[0] = '\0';
    DPX_GET_TEXT(&m_hdr,file_info.create_time,buf, 24);
    buf[24] = '\0';
    return buf;
}

void DpxHdr::set_fi_create_time (const std::string& str)
{
    int len = (int)(std::min) (str.size()+1, (size_t)24);
    DPX_SET_TEXT (&m_hdr,file_info.create_time,str.c_str(), len);
}


std::string DpxHdr::get_fi_project () const
{
    char buf[201];
    buf[0] =  '\0';
    DPX_GET_TEXT(&m_hdr,file_info.project,buf,200);
    buf[200] =  '\0';
    return buf;
}

void DpxHdr::set_fi_project (const std::string& str)
{
    int len = (int)(std::min) (str.size()+1, (size_t)200);
    DPX_SET_TEXT(&m_hdr,file_info.project,str.c_str(),len);
}

std::string DpxHdr::get_fi_copyright() const
{
    char buf[201];
    buf[0]  = '\0';
    DPX_GET_TEXT(&m_hdr,file_info.copyright,buf,200);
    buf[200]  = '\0';
    return buf;
}

void DpxHdr::set_fi_copyright(const std::string& str)
{
    int len = (int)(std::min) (str.size()+1, (size_t)200);
    DPX_SET_TEXT(&m_hdr,file_info.copyright,str.c_str(),len);
}


// Image Info access methods
void DpxHdr::set_ii_orientation(uint16_t orientation)
{
    DPX_SET_U16(&m_hdr,image_info.orientation, orientation);
}

uint16_t DpxHdr::get_ii_orientation() const
{
    return DPX_GET_U16(&m_hdr,image_info.orientation);
}

void DpxHdr::set_ii_element_number(uint16_t element_number)
{
    DPX_SET_U16(&m_hdr,image_info.element_number, element_number);
}

uint16_t DpxHdr::get_ii_element_number() const
{
    return DPX_GET_U16(&m_hdr,image_info.element_number);
}

void DpxHdr::set_ii_pixels (uint32_t pixels)
{
    DPX_SET_U32(&m_hdr,image_info.pixels_per_line, pixels);
}

size_t DpxHdr::get_ii_pixels() const
{
    return (size_t)DPX_GET_U32(&m_hdr,image_info.pixels_per_line);
}

void DpxHdr::set_ii_lines(uint32_t lines)
{
    DPX_SET_U32(&m_hdr,image_info.lines_per_image_ele, lines);
}
size_t DpxHdr::get_ii_lines(void) const
{
    return (size_t)DPX_GET_U32(&m_hdr,image_info.lines_per_image_ele);
}


///////////////////////////////////////
// Image Element access methods
///////////////////////////////////////
void DpxHdr::set_ie_description(const std::string& str, int i)
{
    int len = (int)(std::min) (str.size()+1, (size_t)32);
    DPX_SET_TEXT (&m_hdr, image_info.image_element[i].description, str.c_str(), len);
}

std::string DpxHdr::get_ie_description(int i) const
{
    char buf[33];
    buf[0] = '\0';
    DPX_GET_TEXT (&m_hdr, image_info.image_element[i].description,buf, 32);
    buf[32] = '\0';
    return buf;
}


void DpxHdr::set_ie_data_sign (uint32_t sign, int i)
{
    DPX_SET_U32(&m_hdr, image_info.image_element[i].data_sign, sign);
}


size_t DpxHdr::get_ie_data_sign(int i)
{
    return DPX_GET_U32(&m_hdr, image_info.image_element[i].data_sign);
}


void DpxHdr::set_ie_ref_low_data(uint32_t data, int i)
{
    DPX_SET_U32(&m_hdr, image_info.image_element[i].ref_low_data, data);
}


size_t DpxHdr::get_ie_ref_low_data(int i) const
{
    return DPX_GET_U32(&m_hdr, image_info.image_element[i].ref_low_data);
}


void DpxHdr::set_ie_ref_high_data(uint32_t data, int i)
{
    DPX_SET_U32(&m_hdr, image_info.image_element[i].ref_high_data, data);
}


size_t DpxHdr::get_ie_ref_high_data(int i) const
{
    return DPX_GET_U32(&m_hdr, image_info.image_element[i].ref_high_data);
}


void DpxHdr::set_ie_ref_low_quantity(float data, int i)
{
    DPX_SET_R32(&m_hdr, image_info.image_element[i].ref_low_quantity, data);
}


float DpxHdr::get_ie_ref_low_quantity(int i) const
{
    return DPX_GET_R32(&m_hdr, image_info.image_element[i].ref_low_quantity);
}


void DpxHdr::set_ie_ref_high_quantity(float data, int i)
{
    DPX_SET_R32(&m_hdr, image_info.image_element[i].ref_high_quantity, data);
}


float DpxHdr::get_ie_ref_high_quantity(int i) const
{
    return DPX_GET_R32(&m_hdr, image_info.image_element[i].ref_high_quantity);
}


void DpxHdr::set_ie_descriptor(uint8_t desc, int i)
{
    DPX_SET_U8(&m_hdr, image_info.image_element[i].descriptor, desc);
}


uint8_t DpxHdr::get_ie_descriptor(int i) const
{
    return DPX_GET_U8(&m_hdr, image_info.image_element[i].descriptor);
}


void DpxHdr::set_ie_transfer(uint8_t trans, int i)
{
    DPX_SET_U8(&m_hdr, image_info.image_element[i].transfer, trans);
}


uint8_t DpxHdr::get_ie_transfer(int i) const
{
    return DPX_GET_U8(&m_hdr, image_info.image_element[i].transfer);
}


void DpxHdr::set_ie_colorimetric(uint8_t c, int i)
{
    DPX_SET_U8(&m_hdr, image_info.image_element[i].colorimetric, c);
}


uint8_t DpxHdr::get_ie_colorimetric(int i) const
{
    return DPX_GET_U8(&m_hdr, image_info.image_element[i].colorimetric);
}


void DpxHdr::set_ie_bit_size(uint8_t bits, int i)
{
    DPX_SET_U8(&m_hdr, image_info.image_element[i].bit_size, bits);
}


uint8_t DpxHdr::get_ie_bit_size(int i) const
{
    return DPX_GET_U8(&m_hdr, image_info.image_element[i].bit_size);
}


void DpxHdr::set_ie_packing(uint16_t pack, int i)
{
    DPX_SET_U16(&m_hdr, image_info.image_element[i].packing, pack);
}


uint16_t DpxHdr::get_ie_packing (int i) const
{
    return DPX_GET_U16(&m_hdr, image_info.image_element[i].packing);
}


void DpxHdr::set_ie_encoding(uint16_t enc, int i)
{
    DPX_SET_U16(&m_hdr, image_info.image_element[i].encoding, enc);
}


uint16_t DpxHdr::get_ie_encoding(int i) const
{
    return DPX_GET_U16(&m_hdr, image_info.image_element[i].encoding);
}


void DpxHdr::set_ie_data_offset(uint32_t offs, int i)
{
    DPX_SET_U32(&m_hdr, image_info.image_element[i].data_offset, offs);
}


uint32_t DpxHdr::get_ie_data_offset (int i) const
{
    return DPX_GET_U32(&m_hdr, image_info.image_element[i].data_offset);
}


void DpxHdr::set_ie_eol_padding(uint32_t padding, int i)
{
    DPX_SET_U32(&m_hdr, image_info.image_element[i].eol_padding, padding);
}


uint32_t DpxHdr::get_ie_eol_padding(int i) const
{
    return DPX_GET_U32(&m_hdr, image_info.image_element[i].eol_padding);
}


void DpxHdr::set_ie_eo_image_padding(uint32_t padding, int i)
{
    DPX_SET_U32(&m_hdr, image_info.image_element[i].eo_image_padding, padding);
}


uint32_t DpxHdr::get_ie_eo_image_padding(int i) const
{
    return DPX_GET_U32(&m_hdr, image_info.image_element[i].eo_image_padding);
}


std::string DpxHdr::get_is_filename() const
{
    char buf[101];
    buf[0] = '\0';
    DPX_GET_TEXT(&m_hdr,image_source.file_name,buf,100);
    buf[100] = '\0';
    return buf;
}


void DpxHdr::set_is_filename(const std::string& str)
{
    int len = (int)(std::min) (str.size()+1, (size_t)100);
    DPX_SET_TEXT(&m_hdr,image_source.file_name,str.c_str(),len);
}


std::string DpxHdr::get_is_creation_time() const
{
    char buf[25];
    buf[0] = '\0';
    DPX_GET_TEXT(&m_hdr,image_source.creation_time,buf,24);
    buf[24] = '\0';
    return buf;
}


void DpxHdr::set_is_creation_time(const std::string& str)
{
    int len = (int)(std::min) (str.size()+1, (size_t)24);
    DPX_SET_TEXT(&m_hdr,image_source.creation_time,str.c_str(),len);
}


std::string DpxHdr::get_is_input_device() const
{
    char buf[33];
    buf[0] = '\0';
    DPX_GET_TEXT(&m_hdr,image_source.input_dev,buf,32);
    buf[32] = '\0';
    return buf;
}


void DpxHdr::set_is_input_device(const std::string& str)
{
    int len = (int)(std::min)(str.size()+1, (size_t)32);
    DPX_SET_TEXT(&m_hdr,image_source.input_dev,str.c_str(),len);
}


std::string DpxHdr::get_is_input_serial() const
{
    char buf[33];
    buf[0] = '\0';
    DPX_GET_TEXT(&m_hdr,image_source.input_serial,buf,32);
    buf[32] = '\0';
    return buf;
}


void DpxHdr::set_is_input_serial(const std::string& str)
{
    int len = (int)(std::min) (str.size()+1, (size_t)32);
    DPX_SET_TEXT(&m_hdr,image_source.input_serial,str.c_str(),len);
}


std::string DpxHdr::get_film_mfg_id() const
{
    char buf[3];
    buf[0] = '\0';
    DPX_GET_TEXT (&m_hdr,film_header.film_mfg_id,buf,2);
    buf[2] = '\0';
    return buf;
}


void DpxHdr::set_film_mfg_id(const std::string& str)
{
    int len = (int)(std::min)(str.size()+1, (size_t)2);
    DPX_SET_TEXT (&m_hdr,film_header.film_mfg_id,str.c_str(),len);
}


std::string DpxHdr::get_film_type() const
{
    char buf[3];
    buf[0] = '\0';
    DPX_GET_TEXT (&m_hdr,film_header.film_type,buf,2);
    buf[2] = '\0';
    return buf;
}


void DpxHdr::set_film_type(const std::string& str)
{
    int len = (int)(std::min)(str.size()+1, (size_t)2);
    DPX_SET_TEXT (&m_hdr,film_header.film_type,str.c_str(),len);
}


std::string DpxHdr::get_film_offset() const
{
    char buf[3];
    buf[0] = '\0';
    DPX_GET_TEXT (&m_hdr,film_header.offset,buf,2);
    buf[2] = '\0';
    return buf;

}


void DpxHdr::set_film_offset(const std::string& str)
{
    int len = (int)(std::min)(str.size()+1, (size_t)2);
    DPX_SET_TEXT (&m_hdr,film_header.offset,str.c_str(),len);
}


std::string DpxHdr::get_film_prefix() const
{
    char buf[7];
    buf[0] = '\0';
    DPX_GET_TEXT (&m_hdr,film_header.prefix,buf,6);
    buf[6] = '\0';
    return buf;
}


void DpxHdr::set_film_prefix(const std::string& str)
{
    int len = (int)(std::min)(str.size()+1, (size_t)6);
    DPX_SET_TEXT (&m_hdr,film_header.prefix,str.c_str(),len);

}


std::string DpxHdr::get_film_count() const
{
    char buf[5];
    buf[0] = '\0';
    DPX_GET_TEXT (&m_hdr,film_header.count,buf,4);
    buf[4] = '\0';
    return buf;
}


void DpxHdr::set_film_count(const std::string& str)
{
    int len = (int)(std::min)(str.size()+1, (size_t)4);
    DPX_SET_TEXT (&m_hdr,film_header.count,str.c_str(),len);
}


std::string DpxHdr::get_film_format() const
{
    char buf[33];
    buf[0] = '\0';
    DPX_GET_TEXT (&m_hdr,film_header.format,buf,32);
    buf[32] = '\0';
    return buf;
}


void DpxHdr::set_film_format(const std::string& str)
{
    int len = (int)(std::min)(str.size()+1, (size_t)32);
    DPX_SET_TEXT (&m_hdr,film_header.format,str.c_str(),len);
}


size_t DpxHdr::get_film_frame_position() const
{
    return (size_t)DPX_GET_U32(&m_hdr,film_header.frame_position);
}


void DpxHdr::set_film_frame_position(size_t pos)
{
    DPX_SET_U32(&m_hdr,film_header.frame_position,(uint32_t)pos);
}


size_t DpxHdr::get_film_sequence_len() const
{
    return (size_t)DPX_GET_U32(&m_hdr,film_header.sequence_len);
}


void DpxHdr::set_film_sequence_len(size_t len)
{
    DPX_SET_U32(&m_hdr,film_header.sequence_len,(uint32_t)len);
}


size_t DpxHdr::get_film_held_count () const
{
    return(size_t)DPX_GET_U32(&m_hdr,film_header.held_count);
}


void DpxHdr::set_film_held_count(size_t count)
{
    DPX_SET_U32(&m_hdr,film_header.held_count,(uint32_t)count);
}


float  DpxHdr::get_film_frame_rate () const
{
    return DPX_GET_R32(&m_hdr,film_header.frame_rate);
}


void DpxHdr::set_film_frame_rate (float len)
{
    DPX_SET_R32(&m_hdr, film_header.frame_rate, len);
}


float  DpxHdr::get_film_shutter_angle() const
{
    return DPX_GET_R32(&m_hdr,film_header.shutter_angle);
}


void DpxHdr::set_film_shutter_angle(float angle)
{
    DPX_SET_R32(&m_hdr,film_header.shutter_angle,angle);
}


std::string DpxHdr::get_film_frame_id() const
{
    char buf[33];
    buf[0] = '\0';
    DPX_GET_TEXT(&m_hdr,film_header.frame_id,buf,32);
    buf[32] = '\0';
    return buf;
}


void DpxHdr::set_film_frame_id(const std::string& str)
{
    int len = (int)(std::min) (str.size()+1, (size_t)32);
    DPX_SET_TEXT(&m_hdr,film_header.slate_info,str.c_str(),len);
}


std::string DpxHdr::get_film_slate_info() const
{
    char buf[201];
    buf[0] = '\0';
    DPX_GET_TEXT(&m_hdr,film_header.slate_info,buf,200);
    buf[200] = '\0';
    return buf;
}


void DpxHdr::set_film_slate_info(const std::string& str)
{
    int len = (int)(std::min) (str.size()+1, (size_t)200);
    DPX_SET_TEXT(&m_hdr,film_header.slate_info,str.c_str(),len);
}


size_t DpxHdr::get_tv_timecode () const
{
    return DPX_GET_U32(&m_hdr,tv_header.tim_code);
}


void DpxHdr::set_tv_timecode (size_t tc)
{
    DPX_SET_U32(&m_hdr,tv_header.tim_code,(uint32_t)tc);
}


size_t DpxHdr::get_tv_userbits() const
{
    return (size_t)DPX_GET_U32(&m_hdr,tv_header.tim_code);
}


void DpxHdr::set_tv_userbits(size_t ub)
{
    DPX_SET_U32(&m_hdr,tv_header.userBits,(uint32_t)ub);
}


float  DpxHdr::get_tv_frame_rate () const
{
    return DPX_GET_R32(&m_hdr,tv_header.frame_rate);
}


void DpxHdr::set_tv_frame_rate (float fr)
{
    DPX_SET_R32(&m_hdr,tv_header.frame_rate,fr);
}

uint8_t DpxHdr::get_tv_interlace() const
{
	return (DPX_GET_U8(&m_hdr,tv_header.interlace));
}

void DpxHdr::set_tv_interlace(uint8_t interlace)
{
	DPX_SET_U8(&m_hdr,tv_header.interlace,interlace);
}


DpxHdr::~DpxHdr()
{
}

