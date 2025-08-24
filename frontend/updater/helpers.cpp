#include "helpers.hpp"

void FreeProvider(HCRYPTPROV prov)
{
	CryptReleaseContext(prov, 0);
}

bool file_exists(const wchar_t *path)
{
	WIN32_FIND_DATAW wfd;
	HANDLE h = FindFirstFileW(path, &wfd);
	if (h == INVALID_HANDLE_VALUE)
		return false;
	FindClose(h);
	return true;
}
