#include <vector>
#include <util/dstr.h>
#include "../../plugins/obs-webrtc/whip-utils.h"

#include <iostream>

int main()
{
	const struct {
		const char *header;
		const char *line;
		const char *expected;
	} cases[] = {
		{"etag", "ETag: \"session\"", "\"session\""},
		{"etag", "ETag:\"session\"", "\"session\""},
		{"etag", "eTAG:\t\"session\" \r\n", "\"session\""},
		{"etag", "ETag-Other: \"wrong\"", ""},
		{"location", "Location:/session/123", "/session/123"},
		{"link", "Link:\t<stun:example.invalid>; rel=\"ice-server\"",
		 "<stun:example.invalid>; rel=\"ice-server\""},
		{"etag", "ETag:", ""},
		{"etag", "HTTP/1.1 201 Created", ""},
	};
	int failures = 0;
	for (const auto &test : cases) {
		if (value_for_header(test.header, test.line) != test.expected) {
			std::cerr << "Header case failed: " << test.line << '\n';
			failures++;
		}
	}
	std::cout << "Header failures: " << failures << '\n';
	return failures ? 1 : 0;
}
