/*
Copyright (c) 2015-2016, Intel Corporation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "SegImage.h"

SegImage::SegImage()
	: m_width(0), m_height(0), m_pitch(0), m_data(nullptr), m_number(0), m_timestamp(0L) {}

SegImage::SegImage(int width, int height, int pitch, void* data, long long timestamp, int number)
	: m_width(width), m_height(height), m_pitch(pitch), m_data(new char[height*pitch]), m_timestamp(timestamp), m_number(number)
{
    memcpy_s(m_data, height*pitch, data, height*pitch);
}

SegImage::SegImage(SegImage const& src)
    : m_width(src.m_width), m_height(src.m_height), m_pitch(src.m_pitch), m_data(new char[src.m_height*src.m_pitch]),
	m_timestamp(src.m_timestamp), m_number(src.m_number)
{
	memcpy_s(m_data, m_pitch*m_height, src.m_data, src.m_height*src.m_pitch);
}

SegImage::~SegImage()
{
    delete[] m_data;
}

SegImage& SegImage::operator=(const SegImage & src)
{
    if (this != &src)
    {
        m_width = src.m_width;
        m_height = src.m_height;
        m_pitch = src.m_pitch;
        if (m_data)
        {
            delete[] m_data;
        }
        m_data = new char[m_height*m_pitch];
		m_timestamp = src.m_timestamp;
		m_number = src.m_number;
        memcpy_s(m_data, m_pitch*m_height, src.m_data, m_height*m_pitch);
    }
    return *this;
}

int SegImage::Width()
{
    return m_width;
}

int SegImage::Height()
{
    return m_height;
}

int SegImage::Pitch()
{
    return m_pitch;
}

long long SegImage::TimeStamp()
{
	return m_timestamp;
}

int SegImage::Number()
{
	return m_number;
}

SegPixel SegImage::Get(int i, int j)
{
    SegPixel result;
    result.blue = (((char*)m_data) + j*m_pitch + i * 4)[0];
    result.green = (((char*)m_data) + j*m_pitch + i * 4)[1];
    result.red = (((char*)m_data) + j*m_pitch + i * 4)[2];
    return result;
}
