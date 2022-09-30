#pragma once

#include <obs.hpp>

#include <map>

const std::map<int, std::string> &GetAACEncoderBitrateMap();
const char *GetAACEncoderForBitrate(int bitrate);
int FindClosestAvailableAACBitrate(int bitrate);
