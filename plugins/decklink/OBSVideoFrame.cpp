#include "OBSVideoFrame.h"

OBSVideoFrame::OBSVideoFrame(long width, long height)
{
	this->width = width;
	this->height = height;
	this->rowBytes = width * 2;
	this->data = new unsigned char[width * height * 2 + 1];
}

HRESULT OBSVideoFrame::SetFlags(BMDFrameFlags newFlags)
{
	flags = newFlags;
	return S_OK;
}

HRESULT OBSVideoFrame::SetTimecode(BMDTimecodeFormat format,
				   IDeckLinkTimecode *timecode)
{
	return 0;
}

HRESULT
OBSVideoFrame::SetTimecodeFromComponents(BMDTimecodeFormat format,
					 uint8_t hours, uint8_t minutes,
					 uint8_t seconds, uint8_t frames,
					 BMDTimecodeFlags flags)
{
	return 0;
}

HRESULT OBSVideoFrame::SetAncillaryData(IDeckLinkVideoFrameAncillary *ancillary)
{
	return 0;
}

HRESULT OBSVideoFrame::SetTimecodeUserBits(BMDTimecodeFormat format,
					   BMDTimecodeUserBits userBits)
{
	return 0;
}

long OBSVideoFrame::GetWidth()
{
	return width;
}

long OBSVideoFrame::GetHeight()
{
	return height;
}

long OBSVideoFrame::GetRowBytes()
{
	return rowBytes;
}

BMDPixelFormat OBSVideoFrame::GetPixelFormat()
{
	return pixelFormat;
}

BMDFrameFlags OBSVideoFrame::GetFlags()
{
	return flags;
}

HRESULT OBSVideoFrame::GetBytes(void **buffer)
{
	*buffer = this->data;
	return S_OK;
}
