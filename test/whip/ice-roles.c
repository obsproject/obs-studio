#include "stun.h"
#include <stdio.h>
#include <string.h>

int test_stun(void);

int main(void)
{
	if (test_stun()) {
		fprintf(stderr, "Existing STUN parser/integrity vectors failed\n");
		return 1;
	}
	for (unsigned int roles = 0; roles < 4; roles++) {
		for (unsigned int value = 0; value < 2; value++) {
			stun_message_t sent = {0};
			stun_message_t received = {0};
			unsigned char wire[512];
			sent.msg_class = STUN_CLASS_REQUEST;
			sent.msg_method = STUN_METHOD_BINDING;
			sent.has_ice_controlling = (roles & 1) != 0;
			sent.has_ice_controlled = (roles & 2) != 0;
			sent.ice_controlling = sent.has_ice_controlling ? value : 0;
			sent.ice_controlled = sent.has_ice_controlled ? value : 0;
			strcpy(sent.credentials.username, "test:peer");
			int size = _juice_stun_write(wire, sizeof(wire), &sent, "test-only-password");
			if (size <= 0 || _juice_stun_read(wire, (size_t)size, &received) <= 0 ||
			    received.has_ice_controlling != sent.has_ice_controlling ||
			    received.has_ice_controlled != sent.has_ice_controlled ||
			    received.ice_controlling != sent.ice_controlling ||
			    received.ice_controlled != sent.ice_controlled ||
			    !_juice_stun_check_integrity(wire, (size_t)size, &received, "test-only-password") ||
			    _juice_stun_check_integrity(wire, (size_t)size, &received, "wrong-password")) {
				fprintf(stderr, "ICE role round trip failed: roles=%u value=%u\n", roles, value);
				return 1;
			}
		}
	}
	puts("STUN vectors and 8 ICE role/integrity cases passed");
	return 0;
}
