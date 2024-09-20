#include "crypto-helpers.hpp"

#import <Foundation/Foundation.h>
#import <Security/Security.h>
#import <Security/SecKey.h>

bool VerifySignature(const uint8_t *pubKey, const size_t pubKeyLen, const uint8_t *buf, const size_t len,
                     const uint8_t *sig, const size_t sigLen)
{
    NSData *pubKeyData = [NSData dataWithBytes:pubKey length:pubKeyLen];
    CFArrayRef items = nullptr;

    OSStatus res = SecItemImport((CFDataRef) pubKeyData, nullptr, nullptr, nullptr, (SecItemImportExportFlags) 0,
                                 nullptr, nullptr, &items);
    if (res != errSecSuccess)
        return false;

    SecKeyRef pubKeyRef = (SecKeyRef) CFArrayGetValueAtIndex(items, 0);
    NSData *signedData = [NSData dataWithBytes:buf length:len];
    NSData *signature = [NSData dataWithBytes:sig length:sigLen];

    CFErrorRef errRef;
    bool result = SecKeyVerifySignature(pubKeyRef, kSecKeyAlgorithmRSASignatureMessagePKCS1v15SHA512,
                                        (__bridge CFDataRef) signedData, (__bridge CFDataRef) signature, &errRef);

    CFRelease(items);
    return result;
};
