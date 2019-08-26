#include "win-update-helpers.hpp"

void FreeProvider(HCRYPTPROV prov)
{
	CryptReleaseContext(prov, 0);
}

void FreeHash(HCRYPTHASH hash)
{
	CryptDestroyHash(hash);
}

void FreeKey(HCRYPTKEY key)
{
	CryptDestroyKey(key);
}

std::string vstrprintf(const char *format, va_list args)
{
	if (!format)
		return std::string();

	std::string str;
	int size = (int)vsnprintf(nullptr, 0, format, args) + 1;
	str.resize(size);
	vsnprintf(&str[0], size, format, args);
	return str;
}

std::string strprintf(const char *format, ...)
{
	std::string str;
	va_list args;

	va_start(args, format);
	str = vstrprintf(format, args);
	va_end(args);

	return str;
}
