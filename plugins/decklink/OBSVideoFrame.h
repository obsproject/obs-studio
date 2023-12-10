#pragma once

#include "platform.hpp"
#include "obs.hpp"
#include <atomic>

class OBSVideoFrame : public IDeckLinkMutableVideoFrame {
private:
	BMDFrameFlags flags = bmdFrameFlagDefault;
	BMDPixelFormat pixelFormat = bmdFormat8BitYUV;

	long width;
	long height;
	long rowBytes;

	unsigned char *data;

public:
	OBSVideoFrame(long width, long height, BMDPixelFormat pixelFormat);
	~OBSVideoFrame();

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
		    /* out */ IDeckLinkTimecode **timecode) override
	{
		UNUSED_PARAMETER(format);
		UNUSED_PARAMETER(timecode);
		return E_NOINTERFACE;
	};
	virtual HRESULT STDMETHODCALLTYPE GetAncillaryData(
		/* out */ IDeckLinkVideoFrameAncillary **ancillary) override
	{
		UNUSED_PARAMETER(ancillary);
		return E_NOINTERFACE;
	};

	// IUnknown interface (dummy implementation)
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,
							 LPVOID *ppv) override
	{
		UNUSED_PARAMETER(iid);
		UNUSED_PARAMETER(ppv);
		return E_NOINTERFACE;
	}
	virtual ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
	virtual ULONG STDMETHODCALLTYPE Release() override { return 1; }
};

class HDRVideoFrame : public IDeckLinkVideoFrame,
		      public IDeckLinkVideoFrameMetadataExtensions {
public:
	HDRVideoFrame(IDeckLinkMutableVideoFrame *frame);
	virtual ~HDRVideoFrame() {}

	// IUnknown interface
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,
							 LPVOID *ppv);
	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual ULONG STDMETHODCALLTYPE Release(void);

	// IDeckLinkVideoFrame interface
	virtual long STDMETHODCALLTYPE GetWidth(void)
	{
		return m_videoFrame->GetWidth();
	}
	virtual long STDMETHODCALLTYPE GetHeight(void)
	{
		return m_videoFrame->GetHeight();
	}
	virtual long STDMETHODCALLTYPE GetRowBytes(void)
	{
		return m_videoFrame->GetRowBytes();
	}
	virtual BMDPixelFormat STDMETHODCALLTYPE GetPixelFormat(void)
	{
		return m_videoFrame->GetPixelFormat();
	}
	virtual BMDFrameFlags STDMETHODCALLTYPE GetFlags(void)
	{
		return m_videoFrame->GetFlags() | bmdFrameContainsHDRMetadata;
	}
	virtual HRESULT STDMETHODCALLTYPE GetBytes(void **buffer)
	{
		return m_videoFrame->GetBytes(buffer);
	}
	virtual HRESULT STDMETHODCALLTYPE
	GetTimecode(BMDTimecodeFormat format, IDeckLinkTimecode **timecode)
	{
		return m_videoFrame->GetTimecode(format, timecode);
	}
	virtual HRESULT STDMETHODCALLTYPE
	GetAncillaryData(IDeckLinkVideoFrameAncillary **ancillary)
	{
		return m_videoFrame->GetAncillaryData(ancillary);
	}

	// IDeckLinkVideoFrameMetadataExtensions interface
	virtual HRESULT STDMETHODCALLTYPE
	GetInt(BMDDeckLinkFrameMetadataID metadataID, int64_t *value);
	virtual HRESULT STDMETHODCALLTYPE
	GetFloat(BMDDeckLinkFrameMetadataID metadataID, double *value);
	virtual HRESULT STDMETHODCALLTYPE
	GetFlag(BMDDeckLinkFrameMetadataID metadataID, decklink_bool_t *value);
	virtual HRESULT STDMETHODCALLTYPE
	GetString(BMDDeckLinkFrameMetadataID metadataID,
		  decklink_string_t *value);
	virtual HRESULT STDMETHODCALLTYPE
	GetBytes(BMDDeckLinkFrameMetadataID metadataID, void *buffer,
		 uint32_t *bufferSize);

private:
	ComPtr<IDeckLinkMutableVideoFrame> m_videoFrame;
	std::atomic<ULONG> m_refCount;
};
