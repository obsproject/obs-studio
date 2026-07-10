#include <string>

std::string base64_encode(unsigned char const*, unsigned int len);
std::string base64_decode(std::string const& s);

static inline std::string base64_encode(const char *str, unsigned int len)
{
	return base64_encode((unsigned const char *)str, len);
}

static inline std::string base64_encode(const std::string &str)
{
	return base64_encode(str.c_str(), (unsigned int)str.size());
}
