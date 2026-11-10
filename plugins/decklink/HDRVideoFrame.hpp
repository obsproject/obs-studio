#pragma once

#include "platform.hpp"
#include "obs.hpp"
#include <atomic>

class HDRVideoFrame : public IDeckLinkVideoFrame, public IDeckLinkVideoFrameMetadataExtensions {
public:
	HDRVideoFrame(IDeckLinkMutableVideoFrame *frame);
	virtual ~HDRVideoFrame() {}

	// IUnknown interface
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv);
	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual ULONG STDMETHODCALLTYPE Release(void);

	// IDeckLinkVideoFrame interface
	virtual long STDMETHODCALLTYPE GetWidth(void) { return m_videoFrame->GetWidth(); }
	virtual long STDMETHODCALLTYPE GetHeight(void) { return m_videoFrame->GetHeight(); }
	virtual long STDMETHODCALLTYPE GetRowBytes(void) { return m_videoFrame->GetRowBytes(); }
	virtual BMDPixelFormat STDMETHODCALLTYPE GetPixelFormat(void) { return m_videoFrame->GetPixelFormat(); }
	virtual BMDFrameFlags STDMETHODCALLTYPE GetFlags(void)
	{
		return m_videoFrame->GetFlags() | bmdFrameContainsHDRMetadata;
	}

	virtual HRESULT STDMETHODCALLTYPE GetTimecode(BMDTimecodeFormat format, IDeckLinkTimecode **timecode)
	{
		return m_videoFrame->GetTimecode(format, timecode);
	}
	virtual HRESULT STDMETHODCALLTYPE GetAncillaryData(IDeckLinkVideoFrameAncillary **ancillary)
	{
		return m_videoFrame->GetAncillaryData(ancillary);
	}

	// IDeckLinkVideoFrameMetadataExtensions interface
	virtual HRESULT STDMETHODCALLTYPE GetInt(BMDDeckLinkFrameMetadataID metadataID, int64_t *value);
	virtual HRESULT STDMETHODCALLTYPE GetFloat(BMDDeckLinkFrameMetadataID metadataID, double *value);
	virtual HRESULT STDMETHODCALLTYPE GetFlag(BMDDeckLinkFrameMetadataID metadataID, decklink_bool_t *value);
	virtual HRESULT STDMETHODCALLTYPE GetString(BMDDeckLinkFrameMetadataID metadataID, decklink_string_t *value);
	virtual HRESULT STDMETHODCALLTYPE GetBytes(BMDDeckLinkFrameMetadataID metadataID, void *buffer,
						   uint32_t *bufferSize);

private:
	ComPtr<IDeckLinkMutableVideoFrame> m_videoFrame;
	std::atomic<ULONG> m_refCount;
};
