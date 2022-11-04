#include "update-helpers.hpp"

#include <stdarg.h>

std::string vstrprintf(const char *format, va_list args)
{
	va_list args2;

	if (!format)
		return std::string();

	va_copy(args2, args);

	std::string str;
	int size = (int)vsnprintf(nullptr, 0, format, args2) + 1;
	str.resize(size);
	vsnprintf(&str[0], size, format, args);

	va_end(args2);

	return str;
}

std::string strprintf(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	std::string str;
	str = vstrprintf(format, args);
	va_end(args);

	return str;
}
