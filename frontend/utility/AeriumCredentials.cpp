#include "AeriumCredentials.hpp"

#ifdef __APPLE__
#include <Security/Security.h>

namespace {
CFMutableDictionaryRef CredentialQuery()
{
	auto query =
		CFDictionaryCreateMutable(nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
	CFDictionarySetValue(query, kSecAttrService, CFSTR("tv.aerium.desktop.development"));
	CFDictionarySetValue(query, kSecAttrAccount, CFSTR("session"));
	return query;
}
} // namespace

AeriumCredentials::Result AeriumCredentials::Read()
{
	auto query = CredentialQuery();
	CFDictionarySetValue(query, kSecReturnData, kCFBooleanTrue);
	CFTypeRef value = nullptr;
	const OSStatus status = SecItemCopyMatching(query, &value);
	CFRelease(query);
	QByteArray token;
	if (status == errSecSuccess && value && CFGetTypeID(value) == CFDataGetTypeID()) {
		auto data = static_cast<CFDataRef>(value);
		token = QByteArray(reinterpret_cast<const char *>(CFDataGetBytePtr(data)), CFDataGetLength(data));
	}
	if (value) {
		CFRelease(value);
	}
	return {status == errSecSuccess || status == errSecItemNotFound, token};
}

AeriumCredentials::Result AeriumCredentials::Write(const QByteArray &token)
{
	auto query = CredentialQuery();
	auto data = CFDataCreate(nullptr, reinterpret_cast<const UInt8 *>(token.constData()), token.size());
	auto changes =
		CFDictionaryCreateMutable(nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	CFDictionarySetValue(changes, kSecValueData, data);
	OSStatus status = SecItemUpdate(query, changes);
	if (status == errSecItemNotFound) {
		CFDictionarySetValue(query, kSecValueData, data);
		CFDictionarySetValue(query, kSecAttrLabel, CFSTR("Aerium development session"));
		CFDictionarySetValue(query, kSecAttrAccessible, kSecAttrAccessibleAfterFirstUnlockThisDeviceOnly);
		status = SecItemAdd(query, nullptr);
	}
	CFRelease(changes);
	CFRelease(data);
	CFRelease(query);
	return {status == errSecSuccess, {}};
}

AeriumCredentials::Result AeriumCredentials::Remove()
{
	auto query = CredentialQuery();
	const OSStatus status = SecItemDelete(query);
	CFRelease(query);
	return {status == errSecSuccess || status == errSecItemNotFound, {}};
}
#else
AeriumCredentials::Result AeriumCredentials::Read()
{
	return {true, {}};
}

AeriumCredentials::Result AeriumCredentials::Write(const QByteArray &)
{
	return {false, {}};
}

AeriumCredentials::Result AeriumCredentials::Remove()
{
	return {true, {}};
}
#endif
