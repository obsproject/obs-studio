#pragma once

#include <stdlib.h>
#include <cstdint>

bool VerifySignature(const uint8_t *pubKey, const size_t pubKeyLen,
		     const uint8_t *buf, const size_t len, const uint8_t *sig,
		     const size_t sigLen);
