#include "OBSVideoFrame.h"

OBSVideoFrame::OBSVideoFrame(long width, long height,
			     BMDPixelFormat pixelFormat)
{
	int bpp = 2;
	this->width = width;
	this->height = height;
	this->rowBytes = width * bpp;
	this->data = new unsigned char[width * height * bpp + 1];
	this->pixelFormat = pixelFormat;
}

OBSVideoFrame::~OBSVideoFrame()
{
	delete this->data;
}

HRESULT OBSVideoFrame::SetFlags(BMDFrameFlags newFlags)
{
	flags = newFlags;
	return S_OK;
}

HRESULT OBSVideoFrame::SetTimecode(BMDTimecodeFormat format,
				   IDeckLinkTimecode *timecode)
{
	UNUSED_PARAMETER(format);
	UNUSED_PARAMETER(timecode);
	return 0;
}

HRESULT
OBSVideoFrame::SetTimecodeFromComponents(BMDTimecodeFormat format,
					 uint8_t hours, uint8_t minutes,
					 uint8_t seconds, uint8_t frames,
					 BMDTimecodeFlags flags)
{
	UNUSED_PARAMETER(format);
	UNUSED_PARAMETER(hours);
	UNUSED_PARAMETER(minutes);
	UNUSED_PARAMETER(seconds);
	UNUSED_PARAMETER(frames);
	UNUSED_PARAMETER(flags);
	return 0;
}

HRESULT OBSVideoFrame::SetAncillaryData(IDeckLinkVideoFrameAncillary *ancillary)
{
	UNUSED_PARAMETER(ancillary);
	return 0;
}

HRESULT OBSVideoFrame::SetTimecodeUserBits(BMDTimecodeFormat format,
					   BMDTimecodeUserBits userBits)
{
	UNUSED_PARAMETER(format);
	UNUSED_PARAMETER(userBits);
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

#define CompareREFIID(iid1, iid2) (memcmp(&iid1, &iid2, sizeof(REFIID)) == 0)

HDRVideoFrame::HDRVideoFrame(IDeckLinkMutableVideoFrame *frame)
	: m_videoFrame(frame),
	  m_refCount(1)
{
}

HRESULT HDRVideoFrame::QueryInterface(REFIID iid, LPVOID *ppv)
{
	if (ppv == nullptr)
		return E_INVALIDARG;

	CFUUIDBytes unknown = CFUUIDGetUUIDBytes(IUnknownUUID);
	if (CompareREFIID(iid, unknown))
		*ppv = static_cast<IDeckLinkVideoFrame *>(this);
	else if (CompareREFIID(iid, IID_IDeckLinkVideoFrame))
		*ppv = static_cast<IDeckLinkVideoFrame *>(this);
	else if (CompareREFIID(iid, IID_IDeckLinkVideoFrameMetadataExtensions))
		*ppv = static_cast<IDeckLinkVideoFrameMetadataExtensions *>(
			this);
	else {
		*ppv = nullptr;
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}

ULONG HDRVideoFrame::AddRef(void)
{
	return ++m_refCount;
}

ULONG HDRVideoFrame::Release(void)
{
	ULONG newRefValue = --m_refCount;

	if (newRefValue == 0)
		delete this;

	return newRefValue;
}

HRESULT HDRVideoFrame::GetInt(BMDDeckLinkFrameMetadataID metadataID,
			      int64_t *value)
{
	HRESULT result = S_OK;

	switch (metadataID) {
	case bmdDeckLinkFrameMetadataHDRElectroOpticalTransferFunc:
		*value = 2;
		break;

	case bmdDeckLinkFrameMetadataColorspace:
		// Colorspace is fixed for this sample
		*value = bmdColorspaceRec2020;
		break;

	default:
		result = E_INVALIDARG;
	}

	return result;
}

HRESULT HDRVideoFrame::GetFloat(BMDDeckLinkFrameMetadataID metadataID,
				double *value)
{
	HRESULT result = S_OK;

	switch (metadataID) {
	case bmdDeckLinkFrameMetadataHDRDisplayPrimariesRedX:
		*value = 0.708;
		break;

	case bmdDeckLinkFrameMetadataHDRDisplayPrimariesRedY:
		*value = 0.292;
		break;

	case bmdDeckLinkFrameMetadataHDRDisplayPrimariesGreenX:
		*value = 0.17;
		break;

	case bmdDeckLinkFrameMetadataHDRDisplayPrimariesGreenY:
		*value = 0.797;
		break;

	case bmdDeckLinkFrameMetadataHDRDisplayPrimariesBlueX:
		*value = 0.131;
		break;

	case bmdDeckLinkFrameMetadataHDRDisplayPrimariesBlueY:
		*value = 0.046;
		break;

	case bmdDeckLinkFrameMetadataHDRWhitePointX:
		*value = 0.3127;
		break;

	case bmdDeckLinkFrameMetadataHDRWhitePointY:
		*value = 0.329;
		break;

	case bmdDeckLinkFrameMetadataHDRMaxDisplayMasteringLuminance:
		*value = obs_get_video_hdr_nominal_peak_level();
		break;

	case bmdDeckLinkFrameMetadataHDRMinDisplayMasteringLuminance:
		*value = 0.00001;
		break;

	case bmdDeckLinkFrameMetadataHDRMaximumContentLightLevel:
		*value = obs_get_video_hdr_nominal_peak_level();
		break;

	case bmdDeckLinkFrameMetadataHDRMaximumFrameAverageLightLevel:
		*value = obs_get_video_hdr_nominal_peak_level();
		break;

	default:
		result = E_INVALIDARG;
	}

	return result;
}

HRESULT HDRVideoFrame::GetFlag(BMDDeckLinkFrameMetadataID, decklink_bool_t *)
{
	// Not expecting GetFlag
	return E_INVALIDARG;
}

HRESULT HDRVideoFrame::GetString(BMDDeckLinkFrameMetadataID,
				 decklink_string_t *)
{
	// Not expecting GetString
	return E_INVALIDARG;
}

HRESULT HDRVideoFrame::GetBytes(BMDDeckLinkFrameMetadataID, void *,
				uint32_t *bufferSize)
{
	*bufferSize = 0;
	// Not expecting GetBytes
	return E_INVALIDARG;
}
