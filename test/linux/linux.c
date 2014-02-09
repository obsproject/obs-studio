#include <obs.h>
#include "linux.h"

const char *inputs[] = {
    "xshm_input"
};

uint32_t module_version(uint32_t in_version)
{
    return LIBOBS_API_VER;
}

bool enum_inputs(size_t idx, const char **name)
{
    if (idx >= (sizeof(inputs)/sizeof(const char*)))
        return false;

    *name = inputs[idx];
    return true;
}

bool enum_filters(size_t idx, const char **name)
{
    return false;
}