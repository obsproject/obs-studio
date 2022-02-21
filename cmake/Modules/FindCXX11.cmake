# - Finds if the compiler has C++11 support
# This module can be used to detect compiler flags for using C++11, and checks
# a small subset of the language.
#
# The following variables are set:
#   CXX11_FLAGS - flags to add to the CXX compiler for C++11 support
#   CXX11_FOUND - true if the compiler supports C++11
#

if(CXX11_FLAGS)
    set(CXX11_FOUND TRUE)
    return()
endif()

include(CheckCXXSourceCompiles)

if(MSVC)
    set(CXX11_FLAG_CANDIDATES
	    " "
        )
else()
    set(CXX11_FLAG_CANDIDATES
        #gcc
        "-std=gnu++11"
        "-std=gnu++0x"
        #Gnu and Intel Linux
        "-std=c++11"
        "-std=c++0x"
        #Microsoft Visual Studio, and everything that automatically accepts C++11
        " "
        #Intel windows
        "/Qstd=c++11"
        "/Qstd=c++0x"
        )
endif()

set(CXX11_TEST_SOURCE
"
int main()
{
    int n[] = {4,7,6,1,2};
	int r;
	auto f = [&](int j) { r = j; };
	
    for (auto i : n)
        f(i);
    return 0;
}
")

foreach(FLAG ${CXX11_FLAG_CANDIDATES})
    set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
    set(CMAKE_REQUIRED_FLAGS "${FLAG}")
    unset(CXX11_FLAG_DETECTED CACHE)
    message(STATUS "Try C++11 flag = [${FLAG}]")
    check_cxx_source_compiles("${CXX11_TEST_SOURCE}" CXX11_FLAG_DETECTED)
    set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
    if(CXX11_FLAG_DETECTED)
        set(CXX11_FLAGS_INTERNAL "${FLAG}")
        break()
    endif(CXX11_FLAG_DETECTED)
endforeach(FLAG ${CXX11_FLAG_CANDIDATES})

set(CXX11_FLAGS "${CXX11_FLAGS_INTERNAL}" CACHE STRING "C++11 Flags")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CXX11 DEFAULT_MSG CXX11_FLAGS)
mark_as_advanced(CXX11_FLAGS)
