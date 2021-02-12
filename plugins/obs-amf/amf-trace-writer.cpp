#include "amf-trace-writer.hpp"
#include <iostream>
#include <iomanip>
#include "common.hpp"

void obs_amf_trace_writer::Write(const wchar_t *scope, const wchar_t *message)
{
	PLOG_INFO("[%ls] %ls", scope, message);
}
