#include "stun.h"
#include "agent.h"
#include <stdio.h>
#include <stdlib.h>
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
	/* Exercise role conflict decisions, including a present zero tie-breaker.
	 * No socket is created: sending the 487 response may fail, but the local
	 * role must still follow RFC 8445 section 7.3.1.1. */
	for (unsigned int controlling = 0; controlling < 2; controlling++) {
		for (uint64_t remote = 0; remote < 3; remote++) {
			juice_agent_t *agent = calloc(1, sizeof(*agent));
			if (!agent) {
				return 1;
			}
			agent->mode = controlling ? AGENT_MODE_CONTROLLING : AGENT_MODE_CONTROLLED;
			agent->ice_tiebreaker = 1;
			stun_message_t request = {0};
			request.msg_class = STUN_CLASS_REQUEST;
			request.msg_method = STUN_METHOD_BINDING;
			request.has_ice_controlling = controlling != 0;
			request.has_ice_controlled = controlling == 0;
			request.ice_controlling = controlling ? remote : 0;
			request.ice_controlled = controlling ? 0 : remote;
			agent_stun_entry_t entry = {0};
			entry.type = AGENT_STUN_ENTRY_TYPE_CHECK;
			agent_process_stun_binding(agent, &request, &entry, NULL, NULL);
			agent_mode_t expected = remote <= 1 ? AGENT_MODE_CONTROLLING : AGENT_MODE_CONTROLLED;
			bool valid = agent->mode == expected;
			free(agent);
			if (!valid) {
				fprintf(stderr, "ICE conflict failed: controlling=%u, remote=%llu\n", controlling,
					(unsigned long long)remote);
				return 1;
			}
		}
	}
	puts("STUN vectors, 8 ICE role/integrity cases and 6 role conflicts passed");
	return 0;
}
