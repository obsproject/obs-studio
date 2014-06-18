project(libsamplerate)

set(libsamplerate_SOURCES
	samplerate.c
	src_linear.c
	src_sinc.c
	src_zoh.c
	common.h
	fastest_coeffs.h
	float_cast.h
	high_qual_coeffs.h
	mid_qual_coeffs.h
	samplerate.h)

set(IS_MSVC ${MSVC})

include(CheckCSourceCompiles)
include(CheckCSourceRuns)

set(_tmp_math_header "#define _ISOC9X_SOURCE 1
#define _ISOC99_SOURCE 1
#define __USE_ISOC9X 1
#define __USE_ISOC99 1
#include <math.h>")

check_c_source_compiles("${_tmp_math_header}
int main(void)
{
  int res1 = lrint((double)2.0);
  int res2 = lrintf((float)2.0);
  return res1 + res2;
}" HAVE_LRINT)

check_c_source_runs("${_tmp_math_header}
int main(void)
{
  double fval;
  int k, ival;
  fval = 1.0 * 0x7FFFFFFF;
  for(k = 0; k < 100; k++)
  {
    ival = (lrint(fval)) >> 24;
    if(ival != 127)
      return 1;
    fval *= 1.2499999;
  }
  return 0;
}" CPU_CLIPS_POSITIVE)

check_c_source_runs("${_tmp_math_header}
int main (void)
{
  double fval;
  int k, ival;
  fval = -8.0 * 0x10000000;
  for (k = 0; k < 100; k++)
  {
    ival = (lrint(fval)) >> 24;
    if(ival != -128)
      return 1;
    fval *= 1.2499999;
  }
  return 0;
}" CPU_CLIPS_NEGATIVE)

if(CPU_CLIPS_NEGATIVE)
	set(CPU_CLIPS_NEGATIVE 1)
else()
	set(CPU_CLIPS_NEGATIVE 0)
endif()

if(CPU_CLIPS_POSITIVE)
	set(CPU_CLIPS_POSITIVE 1)
else()
	set(CPU_CLIPS_POSITIVE 0)
endif()

if(HAVE_LRINT)
	set(HAVE_LRINT 1)
	set(HAVE_LRINTF 1)
else()
	set(HAVE_LRINT 0)
	set(HAVE_LRINTF 0)
endif()

configure_file(config.h.in "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_library(libsamplerate STATIC
	${libsamplerate_SOURCES})
target_include_directories(glad
	PUBLIC .)
target_link_libraries(libsamplerate)

if(NOT MSVC)
	target_compile_options(libsamplerate
		PRIVATE -DPIC -fPIC -fvisibility=hidden -mmmx -msse -msse2)
endif()
