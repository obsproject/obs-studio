#pragma once

#include "platform.hpp"

class OBSVideoFrame : public IDeckLinkMutableVideoFrame {
private:
	BMDFrameFlags flags;
	BMDPixelFormat pixelFormat = bmdFormat8BitYUV;

	long width;
	long height;
	long rowBytes;

	unsigned char *data;

public:
	OBSVideoFrame(long width, long height);

	HRESULT STDMETHODCALLTYPE SetFlags(BMDFrameFlags newFlags) override;

	HRESULT STDMETHODCALLTYPE SetTimecode(
		BMDTimecodeFormat format, IDeckLinkTimecode *timecode) override;

	HRESULT STDMETHODCALLTYPE SetTimecodeFromComponents(
		BMDTimecodeFormat format, uint8_t hours, uint8_t minutes,
		uint8_t seconds, uint8_t frames,
		BMDTimecodeFlags flags) override;

	HRESULT
	STDMETHODCALLTYPE
	SetAncillaryData(IDeckLinkVideoFrameAncillary *ancillary) override;

	HRESULT STDMETHODCALLTYPE
	SetTimecodeUserBits(BMDTimecodeFormat format,
			    BMDTimecodeUserBits userBits) override;

	long STDMETHODCALLTYPE GetWidth() override;

	long STDMETHODCALLTYPE GetHeight() override;

	long STDMETHODCALLTYPE GetRowBytes() override;

	BMDPixelFormat STDMETHODCALLTYPE GetPixelFormat() override;

	BMDFrameFlags STDMETHODCALLTYPE GetFlags() override;

	HRESULT STDMETHODCALLTYPE GetBytes(void **buffer) override;

	//Dummy implementations of remaining virtual methods
	virtual HRESULT STDMETHODCALLTYPE
	GetTimecode(/* in */ BMDTimecodeFormat format,
		    /* out */ IDeckLinkTimecode **timecode)
	{
		return E_NOINTERFACE;
	};
	virtual HRESULT STDMETHODCALLTYPE
	GetAncillaryData(/* out */ IDeckLinkVideoFrameAncillary **ancillary)
	{
		return E_NOINTERFACE;
	};

	// IUnknown interface (dummy implementation)
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,
							 LPVOID *ppv)
	{
		return E_NOINTERFACE;
	}
	virtual ULONG STDMETHODCALLTYPE AddRef() { return 1; }
	virtual ULONG STDMETHODCALLTYPE Release() { return 1; }
};
