#pragma once

#include "platform.hpp"

class DeckLinkVideoBufferAccess {
public:
	DeckLinkVideoBufferAccess(IUnknown *frame, BMDBufferAccessFlags flags) : flags_(flags)
	{
		if (SUCCEEDED(frame->QueryInterface(IID_IDeckLinkVideoBuffer, (void **)&buffer_)) &&
		    SUCCEEDED(buffer_->StartAccess(flags))) {
			started_ = true;
		}
	}

	~DeckLinkVideoBufferAccess()
	{
		if (started_) {
			buffer_->EndAccess(flags_);
		}
	}

	DeckLinkVideoBufferAccess(const DeckLinkVideoBufferAccess &) = delete;
	DeckLinkVideoBufferAccess &operator=(const DeckLinkVideoBufferAccess &) = delete;
	DeckLinkVideoBufferAccess(DeckLinkVideoBufferAccess &&) = delete;
	DeckLinkVideoBufferAccess &operator=(DeckLinkVideoBufferAccess &&) = delete;

	void *GetBytes() const
	{
		void *bytes = nullptr;
		if (started_) {
			buffer_->GetBytes(&bytes);
		}
		return bytes;
	}

private:
	ComPtr<IDeckLinkVideoBuffer> buffer_;
	BMDBufferAccessFlags flags_;
	bool started_ = false;
};
