#include <windows.h>
#include <sapi.h>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <obs.h>
#include <media-io/audio-resampler.h>
#include <util/circlebuf.h>
#include <util/windows/WinHandle.hpp>

#include <fstream>

class CircleBuf {
	circlebuf buf = {};
public:
	inline ~CircleBuf() {circlebuf_free(&buf);}
	inline operator circlebuf*() {return &buf;}
	inline circlebuf *operator->() {return &buf;}
};

class Resampler {
	audio_resampler_t *resampler = nullptr;

public:
	inline void Reset(const WAVEFORMATEX *wfex)
	{
		const struct audio_output_info *aoi =
			audio_output_get_info(obs_get_audio());

		struct resample_info src;
		src.samples_per_sec = aoi->samples_per_sec;
		src.format = aoi->format;
		src.speakers = aoi->speakers;

		struct resample_info dst;
		dst.samples_per_sec = uint32_t(wfex->nSamplesPerSec);
		dst.format = AUDIO_FORMAT_16BIT;
		dst.speakers = (enum speaker_layout)wfex->nChannels;

		if (resampler)
			audio_resampler_destroy(resampler);
		resampler = audio_resampler_create(&dst, &src);
	}

	inline ~Resampler() {audio_resampler_destroy(resampler);}
	inline operator audio_resampler_t*() {return resampler;}
};

class CaptionStream : public ISpAudio {
	volatile long refs = 1;
	SPAUDIOBUFFERINFO buf_info = {};
	ULONG notify_size = 0;
	SPAUDIOSTATE state;
	WinHandle event;
	ULONG vol = 0;

	std::condition_variable cv;
	std::mutex m;
	std::vector<int16_t> temp_buf;
	WAVEFORMATEX format = {};
	Resampler resampler;

	CircleBuf buf;
	ULONG wait_size = 0;
	DWORD samplerate = 0;
	ULARGE_INTEGER pos = {};
	ULONGLONG write_pos = 0;

public:
	CaptionStream(DWORD samplerate);

	void Stop();
	void PushAudio(const struct audio_data *audio_data, bool muted);

	// IUnknown methods
	STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override;
	STDMETHODIMP_(ULONG) AddRef() override;
	STDMETHODIMP_(ULONG) Release() override;

	// ISequentialStream methods
	STDMETHODIMP Read(void *data, ULONG bytes, ULONG *read_bytes) override;
	STDMETHODIMP Write(const void *data, ULONG bytes, ULONG *written_bytes)
		override;

	// IStream methods
	STDMETHODIMP Seek(LARGE_INTEGER move, DWORD origin,
			ULARGE_INTEGER *new_pos) override;
	STDMETHODIMP SetSize(ULARGE_INTEGER new_size) override;
	STDMETHODIMP CopyTo(IStream *stream, ULARGE_INTEGER bytes,
			ULARGE_INTEGER *read_bytes,
			ULARGE_INTEGER *written_bytes) override;
	STDMETHODIMP Commit(DWORD commit_flags) override;
	STDMETHODIMP Revert(void) override;
	STDMETHODIMP LockRegion(ULARGE_INTEGER offset, ULARGE_INTEGER size,
			DWORD type) override;
	STDMETHODIMP UnlockRegion(ULARGE_INTEGER offset, ULARGE_INTEGER size,
			DWORD type) override;
	STDMETHODIMP Stat(STATSTG *stg, DWORD flags) override;
	STDMETHODIMP Clone(IStream **stream) override;

	// ISpStreamFormat methods
	STDMETHODIMP GetFormat(GUID *guid, WAVEFORMATEX **co_mem_wfex_out)
		override;

	// ISpAudio methods
	STDMETHODIMP SetState(SPAUDIOSTATE state, ULONGLONG reserved) override;
	STDMETHODIMP SetFormat(REFGUID guid_ref, const WAVEFORMATEX *wfex)
		override;
	STDMETHODIMP GetStatus(SPAUDIOSTATUS *status) override;
	STDMETHODIMP SetBufferInfo(const SPAUDIOBUFFERINFO *buf_info) override;
	STDMETHODIMP GetBufferInfo(SPAUDIOBUFFERINFO *buf_info) override;
	STDMETHODIMP GetDefaultFormat(GUID *format,
			WAVEFORMATEX **co_mem_wfex_out) override;
	STDMETHODIMP_(HANDLE) EventHandle(void) override;
	STDMETHODIMP GetVolumeLevel(ULONG *level) override;
	STDMETHODIMP SetVolumeLevel(ULONG level) override;
	STDMETHODIMP GetBufferNotifySize(ULONG *size) override;
	STDMETHODIMP SetBufferNotifySize(ULONG size) override;
};
