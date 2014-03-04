#pragma once

#define WIN32_MEAN_AND_LEAN
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <propsys.h>
#include <functiondiscoverykeys_devpkey.h>

#include <vector>
#include <string>

struct AudioDeviceInfo {
	std::string name;
	std::string id;
};

std::string GetDeviceName(IMMDevice *device);
void GetWASAPIAudioDevices(std::vector<AudioDeviceInfo> &devices, bool input);
