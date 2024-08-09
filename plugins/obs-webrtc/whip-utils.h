#pragma once

#include <obs.h>

#include <string>
#include <random>
#include <sstream>

#define do_log(level, format, ...)                              \
	blog(level, "[obs-webrtc] [whip_output: '%s'] " format, \
	     obs_output_get_name(output), ##__VA_ARGS__)

static uint32_t generate_random_u32()
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<uint32_t> dist(1, (UINT32_MAX - 1));
	return dist(gen);
}

static std::string trim_string(const std::string &source)
{
	std::string ret(source);
	ret.erase(0, ret.find_first_not_of(" \n\r\t"));
	ret.erase(ret.find_last_not_of(" \n\r\t") + 1);
	return ret;
}

static std::string value_for_header(const std::string &header,
				    const std::string &val)
{
	if (val.size() <= header.size() ||
	    astrcmpi_n(header.c_str(), val.c_str(), header.size()) != 0) {
		return "";
	}

	auto delimiter = val.find_first_of(" ");
	if (delimiter == std::string::npos) {
		return "";
	}

	return val.substr(delimiter + 1);
}

static size_t curl_writefunction(char *data, size_t size, size_t nmemb,
				 void *priv_data)
{
	auto read_buffer = static_cast<std::string *>(priv_data);

	size_t real_size = size * nmemb;

	read_buffer->append(data, real_size);
	return real_size;
}

static size_t curl_header_function(char *data, size_t size, size_t nmemb,
				   void *priv_data)
{
	auto header_buffer = static_cast<std::vector<std::string> *>(priv_data);
	header_buffer->push_back(trim_string(std::string(data, size * nmemb)));
	return size * nmemb;
}

static inline std::string generate_user_agent()
{
#ifdef _WIN64
#define OS_NAME "Windows x86_64"
#elif __APPLE__
#define OS_NAME "Mac OS X"
#elif __OpenBSD__
#define OS_NAME "OpenBSD"
#elif __FreeBSD__
#define OS_NAME "FreeBSD"
#elif __linux__ && __LP64__
#define OS_NAME "Linux x86_64"
#else
#define OS_NAME "Linux"
#endif

	// Build the user-agent string
	std::stringstream ua;
	// User agent header prefix
	ua << "User-Agent: Mozilla/5.0 ";
	// OBS version info
	ua << "(OBS-Studio/" << obs_get_version_string() << "; ";
	// Operating system version info
	ua << OS_NAME << "; " << obs_get_locale() << ")";

	return ua.str();
}

static const int avpriv_mpeg4audio_sample_rates[16] = {
	96000, 88200, 64000, 48000, 44100, 32000, 24000,
	22050, 16000, 12000, 11025, 8000,  7350};

static const uint8_t ff_mpeg4audio_channels[8] = {0, 1, 2, 3, 4, 5, 6, 8};

#define MKBETAG(a, b, c, d) \
	((d) | ((c) << 8) | ((b) << 16) | ((unsigned)(a) << 24))

struct GetBitContext {
	uint8_t *data;
	int bit_length;
	int bit_index;
};
struct PutBitContext {
	uint8_t *buf;
	int size;
	int index;
	uint32_t bit_buf;
	uint32_t bit_left;
};

static inline int show_bits(GetBitContext *gb, int bits)
{
	uint8_t *buf = gb->data + (gb->bit_index >> 3);
	unsigned int val = (((uint32_t)((const uint8_t *)(buf))[0] << 24) |
			    ((uint32_t)((const uint8_t *)(buf))[1] << 16) |
			    ((uint32_t)((const uint8_t *)(buf))[2] << 8) |
			    (uint32_t)((const uint8_t *)(buf))[3]);
	val <<= (gb->bit_index & 7);

	val = (((uint32_t)(val)) >> (32 - (bits)));

	return val;
}

static inline int get_bits(GetBitContext *gb, int bits)
{
	uint8_t *buf = gb->data + (gb->bit_index >> 3);
	unsigned int val = (((uint32_t)((const uint8_t *)(buf))[0] << 24) |
			    ((uint32_t)((const uint8_t *)(buf))[1] << 16) |
			    ((uint32_t)((const uint8_t *)(buf))[2] << 8) |
			    (uint32_t)((const uint8_t *)(buf))[3]);
	val <<= (gb->bit_index & 7);

	val = (((uint32_t)(val)) >> (32 - (bits)));

	gb->bit_index += bits;
	return val;
}

static inline int get_object_type(GetBitContext *gb)
{
	int object_type = get_bits(gb, 5);
	if (object_type == 31 /*AOT_ESCAPE*/)
		object_type = 32 + get_bits(gb, 6);
	return object_type;
}

static inline int get_sample_rate(GetBitContext *gb, int *index)
{
	*index = get_bits(gb, 4);
	return *index == 0x0f ? get_bits(gb, 24)
			      : avpriv_mpeg4audio_sample_rates[*index];
}

#ifndef AV_WB32
#define AV_WB32(p, val)                          \
	do {                                     \
		uint32_t d = (val);              \
		((uint8_t *)(p))[3] = (d);       \
		((uint8_t *)(p))[2] = (d) >> 8;  \
		((uint8_t *)(p))[1] = (d) >> 16; \
		((uint8_t *)(p))[0] = (d) >> 24; \
	} while (0)
#endif

#define AV_RB16(x) \
	((((const uint8_t *)(x))[0] << 8) | ((const uint8_t *)(x))[1])

static void put_bits(PutBitContext *pb, uint32_t n, uint64_t value)
{
	if (n <= 32) {
		uint32_t va = (uint32_t)value;
		if (n < pb->bit_left) {
			pb->bit_buf = (pb->bit_buf << n) | va;
			pb->bit_left -= n;
		} else {
			pb->bit_buf <<= pb->bit_left;
			pb->bit_buf |= va >> (n - pb->bit_left);

			AV_WB32(pb->buf + pb->index, pb->bit_buf);
			pb->index += 4;

			pb->bit_left += 32 - n;
			pb->bit_buf = va;
		}
	} else if (n > 32) {
		uint32_t lo = value & 0xffffffff;
		uint32_t hi = value >> 32;

		put_bits(pb, n - 32, hi);
		put_bits(pb, 32, lo);
	}
}
