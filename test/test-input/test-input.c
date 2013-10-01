#include "test-input-exports.h"

const char *inputs[] = {"random"};
const char *filters[] = {"test"};

bool enum_inputs(size_t idx, const char **name)
{
	if (idx >= (sizeof(inputs)/sizeof(const char*)))
		return false;

	*name = inputs[idx];
	return true;
}

bool enum_filters(size_t idx, const char **name)
{
	if (idx >= (sizeof(filters)/sizeof(const char*)))
		return false;

	*name = filters[idx];
	return true;
}
