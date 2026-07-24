#include "crypto-helpers.hpp"

#include <limits>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

bool VerifySignature(const uint8_t *pubKey, const size_t pubKeyLen, const uint8_t *buf, const size_t len,
		     const uint8_t *sig, const size_t sigLen)
{
	bool result = false;
	BIO *bio = nullptr;
	EVP_PKEY *key = nullptr;
	EVP_MD_CTX *context = nullptr;

	if (pubKeyLen > static_cast<size_t>(std::numeric_limits<int>::max())) {
		goto exit;
	}

	bio = BIO_new_mem_buf(pubKey, static_cast<int>(pubKeyLen));
	if (!bio) {
		goto exit;
	}

	key = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
	if (!key) {
		goto exit;
	}

	context = EVP_MD_CTX_new();
	if (!context) {
		goto exit;
	}

	if (EVP_DigestVerifyInit(context, nullptr, EVP_sha512(), nullptr, key) != 1) {
		goto exit;
	}

	if (EVP_DigestVerifyUpdate(context, buf, len) != 1) {
		goto exit;
	}

	result = EVP_DigestVerifyFinal(context, sig, sigLen) == 1;

exit:
	EVP_MD_CTX_free(context);
	EVP_PKEY_free(key);
	BIO_free(bio);

	return result;
}
