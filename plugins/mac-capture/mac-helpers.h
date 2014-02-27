#pragma once

static inline bool mac_success(OSStatus stat, const char *action)
{
	if (stat != noErr) {
		blog(LOG_WARNING, "%s failed: %d", action, (int)stat);
		return false;
	}

	return true;
}

static inline bool cf_to_cstr(CFStringRef ref, char *buf, size_t size)
{
	return (bool)CFStringGetCString(ref, buf, size, kCFStringEncodingUTF8);
}
