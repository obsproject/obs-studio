#pragma once

#include <obs.hpp>

static const char *kProgramPropsFilename = "ajaOutputProps.json";
static const char *kPreviewPropsFilename = "ajaPreviewOutputProps.json";

OBSData load_settings(const char *filename);
void output_toggle();
void preview_output_toggle();
