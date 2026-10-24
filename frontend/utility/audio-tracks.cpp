#include "audio-tracks.hpp"

#include <string_view>

namespace {

std::string MakeTrackConfigKey(size_t trackIdx, std::string_view suffix)
{
	constexpr std::string_view prefix = "Track";
	const std::string number = std::to_string(trackIdx + 1);

	std::string key;
	key.reserve(prefix.size() + number.size() + suffix.size());
	key.append(prefix).append(number).append(suffix);

	return key;
}

} // namespace

std::string GetTrackBitrateConfigKey(size_t trackIdx)
{
	return MakeTrackConfigKey(trackIdx, "Bitrate");
}

std::string GetTrackNameConfigKey(size_t trackIdx)
{
	return MakeTrackConfigKey(trackIdx, "Name");
}
