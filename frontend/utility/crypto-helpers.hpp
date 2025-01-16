#pragma once

#include <cstdint>
#include <stdlib.h>

bool VerifySignature(const uint8_t *pubKey, const size_t pubKeyLen, const uint8_t *buf, const size_t len,
		     const uint8_t *sig, const size_t sigLen);
