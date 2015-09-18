#pragma once

#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#undef WIN32_MEAN_AND_LEAN

#include <mfapi.h>
#include <mfidl.h>

#include <stdint.h>
#include <vector>

#include <util/windows/ComPtr.hpp>

namespace MF {

enum class EncoderType {
	H264_SOFTWARE,
	H264_QSV,
	H264_NVENC,
	H264_VCE,
};

static const char *typeNames[] = {
	"Software",
	"Quicksync",
	"NVENC",
	"AMD VCE",
};

class EncoderDescriptor {
public:
	static std::vector<std::shared_ptr<EncoderDescriptor>> EncoderDescriptor::Enumerate();

public:
	EncoderDescriptor(ComPtr<IMFActivate> activate_,
			const char *name_,
			const char *id_,
			GUID &guid_,
			const std::string &guidString_,
			bool isAsync_,
			bool isHardware_,
			EncoderType type_)
		: activate             (activate_),
		  name                 (name_),
		  id                   (id_),
		  guid                 (guid_),
		  guidString           (guidString_),
		  isAsync              (isAsync_),
		  isHardware           (isHardware_),
		  type                 (type_)
	{}

	EncoderDescriptor(const EncoderDescriptor &) = delete;

public:
	const char *Name() const { return name; }
	const char *Id() const { return id; }
	ComPtr<IMFActivate> &Activator() { return activate; }
	GUID &Guid() { return guid; }
	std::string GuidString() const { return guidString; }
	bool Async() const { return isAsync; }
	bool Hardware() const { return isHardware; }
	EncoderType Type() const { return type; }

private:
	ComPtr<IMFActivate> activate;
	const char *name;
	const char *id;
	GUID guid;
	std::string guidString;
	bool isAsync;
	bool isHardware;
	EncoderType type;
};
};
