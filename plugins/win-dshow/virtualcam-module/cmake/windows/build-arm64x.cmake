# build-arm64x.cmake
#
# Post-build driver that produces an ARM64X hybrid variant of the virtual-camera
# DirectShow filter from the already-built native ARM64 object files.
#
# Why ARM64X: the DirectShow filter DLL is loaded IN-PROCESS by the application
# that consumes the camera. On Windows on ARM the native-ARM64 and emulated-x64
# processes share ONE 64-bit COM registration view (HKCR\CLSID\{guid}\InprocServer32),
# which has a single slot. A single-arch DLL can therefore serve only one of the two.
# An ARM64X image contains BOTH an arm64 slice (for native ARM64 consumers) and an
# arm64ec slice (for emulated x64 consumers), so one registered DLL serves both.
#
# Approach: recompile the same translation units with /arm64EC, then merge the
# native arm64 objects and the arm64ec objects with `link /MACHINE:ARM64X`.
#
# Invoked via cmake -P with these -D arguments:
#   NATIVE_OBJ_DIR  - directory holding native ARM64 .obj/.res AND the *.tlog dir
#   WORK_DIR        - working dir the native compile ran in (module build dir)
#   CL              - full path to the ARM64-targeting cl.exe (Hostx64/arm64)
#   LINK            - full path to link.exe (Hostx64/arm64)
#   DEF             - path to the arm64x .def (exports)
#   OUT_DLL         - final output DLL path
#   EC_OBJ_DIR      - scratch dir for arm64ec .obj output
#   SYSTEM_LIBS     - space-separated list of system import libs to link

cmake_minimum_required(VERSION 3.28)

# Locate the native CL command tlog. Its parent dir name carries an MSBuild hash
# (obs-virt.XXXXXXXX.tlog), so glob rather than hard-code it.
file(GLOB _tlog_candidates "${NATIVE_OBJ_DIR}/*.tlog/CL.command.*.tlog")
if(NOT _tlog_candidates)
  message(FATAL_ERROR "ARM64X: no CL.command tlog under ${NATIVE_OBJ_DIR}/*.tlog/")
endif()
list(GET _tlog_candidates 0 TLOG)
message(STATUS "ARM64X: using compile tlog ${TLOG}")

file(MAKE_DIRECTORY "${EC_OBJ_DIR}")

# --- Parse the UTF-16 MSBuild CL tlog into (source, args) command pairs -------
# Each command is two logical lines:
#   ^<absolute source path>
#   <cl argument string ending in the source path, contains /c /I /D ...>
file(STRINGS "${TLOG}" _tlog_lines ENCODING UTF-16LE)

set(_pending_src "")
set(_ec_objs "")
set(_compile_failed FALSE)

foreach(_line IN LISTS _tlog_lines)
  # Strip a leading BOM if present on the first entry
  string(REGEX REPLACE "^﻿" "" _line "${_line}")
  if(_line MATCHES "^\\^(.+)$")
    set(_pending_src "${CMAKE_MATCH_1}")
  elseif(NOT _pending_src STREQUAL "")
    # _line is the argument string for _pending_src
    get_filename_component(_base "${_pending_src}" NAME_WE)
    set(_obj "${EC_OBJ_DIR}/${_base}.obj")

    # Recompile as ARM64EC. We keep the recorded args verbatim (they carry the
    # exact /I include dirs, /D defines incl. the quoted CMAKE_INTDIR, /std, /EH,
    # /MT and the source path) and only append overrides:
    #   /arm64EC  -> emit ARM64EC code
    #   /WX-      -> do not fail the hybrid build on warnings
    #   /Fo<obj>  -> redirect object output into our scratch dir
    # The args must run with WORKING_DIRECTORY = NATIVE_OBJ_DIR's parent (the
    # module build dir) so relative /I and generated headers resolve as in the
    # real build; the caller sets that via the wrapper's cd.
    separate_arguments(_args NATIVE_COMMAND "${_line}")
    execute_process(
      COMMAND "${CL}" /arm64EC ${_args} /WX- "/Fo${_obj}"
      WORKING_DIRECTORY "${WORK_DIR}"
      RESULT_VARIABLE _rc
      OUTPUT_VARIABLE _out
      ERROR_VARIABLE _err
    )
    if(NOT _rc EQUAL 0)
      message(WARNING "ARM64X: EC compile failed for ${_base}:\n${_out}\n${_err}")
      set(_compile_failed TRUE)
    else()
      list(APPEND _ec_objs "${_obj}")
    endif()
    set(_pending_src "")
  endif()
endforeach()

if(_compile_failed)
  message(FATAL_ERROR "ARM64X: one or more ARM64EC compilations failed")
endif()

list(LENGTH _ec_objs _ec_count)
if(_ec_count EQUAL 0)
  message(FATAL_ERROR "ARM64X: no ARM64EC objects were produced (tlog parse failed?)")
endif()
message(STATUS "ARM64X: compiled ${_ec_count} ARM64EC object(s)")

# --- Collect native objects (+ resource) --------------------------------------
file(GLOB _native_objs "${NATIVE_OBJ_DIR}/*.obj")
file(GLOB _native_res "${NATIVE_OBJ_DIR}/*.res")
list(LENGTH _native_objs _nat_count)
if(_nat_count EQUAL 0)
  message(FATAL_ERROR "ARM64X: no native ARM64 objects in ${NATIVE_OBJ_DIR}")
endif()
message(STATUS "ARM64X: linking ${_nat_count} native + ${_ec_count} EC objects")

# --- Merge-link into an ARM64X DLL --------------------------------------------
# A single link invocation with /MACHINE:ARM64X and both object sets lets the
# linker partition native vs EC code. CRT is pulled automatically (sources use
# /MT) from the vcvars-provided lib paths for both slices.
#
# CRITICAL: an ARM64X image carries TWO export tables — one per view. /DEF only
# populates the ARM64EC (emulated-x64) view; without /defArm64Native the NATIVE
# ARM64 view exports nothing, so `dumpbin -exports` shows 0 functions and
# regsvr32 cannot find DllRegisterServer/DllGetClassObject. The same .def (the
# six Dll* COM entry points) applies to both views, so pass it to both switches.
separate_arguments(_libs NATIVE_COMMAND "${SYSTEM_LIBS}")

execute_process(
  COMMAND "${LINK}"
    /nologo /DLL /MACHINE:ARM64X
    "/DEF:${DEF}"
    "/defArm64Native:${DEF}"
    "/OUT:${OUT_DLL}"
    ${_native_objs}
    ${_ec_objs}
    ${_native_res}
    ${_libs}
  RESULT_VARIABLE _link_rc
  OUTPUT_VARIABLE _link_out
  ERROR_VARIABLE _link_err
)
if(NOT _link_rc EQUAL 0)
  message(FATAL_ERROR "ARM64X: link failed:\n${_link_out}\n${_link_err}")
endif()

if(NOT EXISTS "${OUT_DLL}")
  message(FATAL_ERROR "ARM64X: link reported success but ${OUT_DLL} is missing")
endif()
message(STATUS "ARM64X: produced ${OUT_DLL}")
