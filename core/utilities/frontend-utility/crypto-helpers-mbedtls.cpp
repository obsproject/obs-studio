#include "crypto-helpers.hpp"

#include <mbedtls/md.h>
#include <mbedtls/pk.h>

bool VerifySignature(const uint8_t *pubKey, const size_t pubKeyLen, const uint8_t *buf, const size_t len,
		     const uint8_t *sig, const size_t sigLen)
{
	bool result = false;
	int ret = 1;
	unsigned char hash[64];
	mbedtls_pk_context pk;

	mbedtls_pk_init(&pk);

	// Parse PEM key
	if ((ret = mbedtls_pk_parse_public_key(&pk, pubKey, pubKeyLen + 1)) != 0) {
		goto exit;
	}
	// Hash input buffer
	if ((ret = mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA512), buf, len, hash)) != 0) {
		goto exit;
	}
	// Verify signautre
	if ((ret = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA512, hash, 64, sig, sigLen)) != 0) {
		goto exit;
	}

	result = true;

exit:
	mbedtls_pk_free(&pk);

	return result;
}
