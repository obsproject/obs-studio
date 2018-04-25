cmake_minimum_required(VERSION 3.9)
project(libobs-tests)

include(ExternalProject)

get_property(_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

ExternalProject_Add(
	cmocka_external
	GIT_REPOSITORY https://github.com/computerquip-streamlabs/cmocka.git
	GIT_TAG cmocka-static-fix
	INSTALL_COMMAND ""
	BUILD_COMMAND cmake --build <BINARY_DIR> --target cmocka_static --config $<CONFIG>
	CMAKE_ARGS -DWITH_STATIC_LIB:BOOL=true
	BUILD_BYPRODUCTS "<BINARY_DIR>/src/${CMAKE_STATIC_LIBRARY_PREFIX}cmocka_static${CMAKE_STATIC_LIBRARY_SUFFIX}"
)

add_library(cmocka STATIC IMPORTED)
ExternalProject_Get_Property(cmocka_external binary_dir)

if (_IS_MULTI_CONFIG)
	set_property(TARGET cmocka PROPERTY IMPORTED_LOCATION_DEBUG
		"${binary_dir}/src/Debug/${CMAKE_STATIC_LIBRARY_PREFIX}cmocka_static${CMAKE_STATIC_LIBRARY_SUFFIX}")
	set_property(TARGET cmocka PROPERTY IMPORTED_LOCATION_RELEASE
		"${binary_dir}/src/Release/${CMAKE_STATIC_LIBRARY_PREFIX}cmocka_static${CMAKE_STATIC_LIBRARY_SUFFIX}")
	set_property(TARGET cmocka PROPERTY IMPORTED_LOCATION_RELWITHDEBINFO
		"${binary_dir}/src/RelWithDebInfo/${CMAKE_STATIC_LIBRARY_PREFIX}cmocka_static${CMAKE_STATIC_LIBRARY_SUFFIX}")
	set_property(TARGET cmocka PROPERTY IMPORTED_LOCATION_MINRELSIZE
		"${binary_dir}/src/MinRelSize/${CMAKE_STATIC_LIBRARY_PREFIX}cmocka_static${CMAKE_STATIC_LIBRARY_SUFFIX}")
else ()
	set_property(TARGET cmocka PROPERTY IMPORTED_LOCATION
		"${binary_dir}/src/${CMAKE_STATIC_LIBRARY_PREFIX}cmocka_static${CMAKE_STATIC_LIBRARY_SUFFIX}")
endif ()

ExternalProject_Get_Property(cmocka_external source_dir)

set(CMOCKA_INCLUDE_DIRS "${source_dir}/include")

add_dependencies(cmocka cmocka_external)

add_executable(libobs_tests main.c)
target_link_libraries(libobs_tests cmocka libobs)

target_include_directories(libobs_tests
	PRIVATE ${CMOCKA_INCLUDE_DIRS}
	PRIVATE ${PROJECT_SOURCE_DIR}
)

# Note! - It's important we include and not add_subdirectory here
# as we append to the sources list of libobs_tests dynamically.
# The end result is all tests are placed within a single binary
# without the need for static libraries.
include(${CMAKE_CURRENT_LIST_DIR}/suites/CMakeLists.txt)

install_obs_core(libobs_tests)