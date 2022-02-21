#include <obs-module.h>
#include <util/platform.h>
#include <memory>
#include <algorithm>

#include "mf-encoder-descriptor.hpp"

using namespace MF;

template<class T> class ComHeapPtr {

protected:
	T *ptr;

	inline void Kill()
	{
		if (ptr)
			CoTaskMemFree(ptr);
	}

	inline void Replace(T *p)
	{
		if (ptr != p) {
			if (ptr) ptr->Kill();
			ptr = p;
		}
	}

public:
	inline ComHeapPtr() : ptr(nullptr)                 {}
	inline ComHeapPtr(T *p) : ptr(p)                   {}
	inline ComHeapPtr(const ComHeapPtr<T> &c)          = delete;
	inline ComHeapPtr(ComHeapPtr<T> &&c)               = delete;
	inline ~ComHeapPtr()                               { Kill(); }

	inline void Clear()
	{
		if (ptr) {
			Kill();
			ptr = nullptr;
		}
	}

	inline ComPtr<T> &operator=(T *p)
	{
		Replace(p);
		return *this;
	}

	inline T *Detach()
	{
		T *out = ptr;
		ptr = nullptr;
		return out;
	}

	inline T **Assign()                { Clear(); return &ptr; }
	inline void Set(T *p)              { Kill(); ptr = p; }

	inline T *Get() const              { return ptr; }

	inline T **operator&()             { return Assign(); }

	inline    operator T*() const      { return ptr; }
	inline T *operator->() const       { return ptr; }

	inline bool operator==(T *p) const { return ptr == p; }
	inline bool operator!=(T *p) const { return ptr != p; }

	inline bool operator!() const      { return !ptr; }
};

struct EncoderEntry {
	const char *guid;
	const char *name;
	const char *id;
	EncoderType type;

} guidNameMap[] = {
	{
		"{6CA50344-051A-4DED-9779-A43305165E35}",
		"MF.H264.EncoderSWMicrosoft",
		"mf_h264_software",
		EncoderType::H264_SOFTWARE
	},
	{
		"{ADC9BC80-0F41-46C6-AB75-D693D793597D}",
		"MF.H264.EncoderHWAMD",
		"mf_h264_vce",
		EncoderType::H264_VCE,
	},
	{
		"{4BE8D3C0-0515-4A37-AD55-E4BAE19AF471}",
		"MF.H264.EncoderHWIntel",
		"mf_h264_qsv",
		EncoderType::H264_QSV
	},
	{
		"{60F44560-5A20-4857-BFEF-D29773CB8040}",
		"MF.H264.EncoderHWNVIDIA",
		"mf_h264_nvenc",
		EncoderType::H264_NVENC
	}
};

static std::string MBSToString(wchar_t *mbs)
{
	char *cstr;
	os_wcs_to_utf8_ptr(mbs, 0, &cstr);
	std::string str = cstr;
	bfree(cstr);
	return str;
}

static std::unique_ptr<EncoderDescriptor> CreateDescriptor(
		ComPtr<IMFActivate> activate)
{
	UINT32 flags;
	if (FAILED(activate->GetUINT32(MF_TRANSFORM_FLAGS_Attribute, &flags)))
		return nullptr;

	bool isAsync = !(flags & MFT_ENUM_FLAG_SYNCMFT);
	isAsync |= !!(flags & MFT_ENUM_FLAG_ASYNCMFT);
	bool isHardware = !!(flags & MFT_ENUM_FLAG_HARDWARE);

	GUID guid = {0};

	if (FAILED(activate->GetGUID(MFT_TRANSFORM_CLSID_Attribute, &guid)))
		return nullptr;

	ComHeapPtr<WCHAR> guidW;
	StringFromIID(guid, &guidW);
	std::string guidString = MBSToString(guidW);

	auto pred = [guidString](const EncoderEntry &name) {
		return guidString == name.guid;
	};

	EncoderEntry *entry = std::find_if(std::begin(guidNameMap),
			std::end(guidNameMap), pred);

	std::unique_ptr<EncoderDescriptor> descriptor(new EncoderDescriptor(
			activate, entry->name, entry->id, guid, guidString,
			isAsync, isHardware, entry->type));

	return descriptor;
}

std::vector<std::shared_ptr<EncoderDescriptor>> EncoderDescriptor::Enumerate()
{
	HRESULT hr;
	UINT32 count = 0;
	std::vector<std::shared_ptr<EncoderDescriptor>> descriptors;

	ComHeapPtr<IMFActivate *> ppActivate;

	MFT_REGISTER_TYPE_INFO info = { MFMediaType_Video, MFVideoFormat_H264 };

	UINT32 unFlags = 0;

	unFlags |= MFT_ENUM_FLAG_LOCALMFT;
	unFlags |= MFT_ENUM_FLAG_TRANSCODE_ONLY;

	unFlags |= MFT_ENUM_FLAG_SYNCMFT;
	unFlags |= MFT_ENUM_FLAG_ASYNCMFT;
	unFlags |= MFT_ENUM_FLAG_HARDWARE;

	unFlags |= MFT_ENUM_FLAG_SORTANDFILTER;

	hr = MFTEnumEx(MFT_CATEGORY_VIDEO_ENCODER,
		unFlags,
		NULL,
		&info,
		&ppActivate,
		&count);

	if (SUCCEEDED(hr) && count == 0) {
		return descriptors;
	}

	if (SUCCEEDED(hr)) {
		for (decltype(count) i = 0; i < count; i++) {
			auto p = std::move(CreateDescriptor(ppActivate[i]));
			if (p)
				descriptors.emplace_back(std::move(p));
		}
	}

	for (UINT32 i = 0; i < count; i++) {
		ppActivate[i]->Release();
	}

	return descriptors;
}
