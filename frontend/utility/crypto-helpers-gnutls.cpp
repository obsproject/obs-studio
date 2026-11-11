#include "crypto-helpers.hpp"

#include <climits>
#include <gnutls/abstract.h>

bool VerifySignature(const uint8_t *pubKey, const size_t pubKeyLen, const uint8_t *buf, const size_t len,
		     const uint8_t *sig, const size_t sigLen)
{
	gnutls_pubkey_t key = nullptr;
	if (pubKeyLen > UINT_MAX || len > UINT_MAX || sigLen > UINT_MAX || gnutls_pubkey_init(&key) < 0) {
		return false;
	}

	const gnutls_datum_t keyData = {const_cast<unsigned char *>(pubKey), static_cast<unsigned int>(pubKeyLen)};
	const gnutls_datum_t data = {const_cast<unsigned char *>(buf), static_cast<unsigned int>(len)};
	const gnutls_datum_t signature = {const_cast<unsigned char *>(sig), static_cast<unsigned int>(sigLen)};

	const bool result = gnutls_pubkey_import(key, &keyData, GNUTLS_X509_FMT_PEM) >= 0 &&
			    gnutls_pubkey_verify_data2(key, GNUTLS_SIGN_RSA_SHA512, 0, &data, &signature) >= 0;
	gnutls_pubkey_deinit(key);
	return result;
}
