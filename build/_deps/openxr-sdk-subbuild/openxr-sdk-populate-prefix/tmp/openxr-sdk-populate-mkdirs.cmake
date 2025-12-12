# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/openxr-sdk-src"
  "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/openxr-sdk-build"
  "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/openxr-sdk-subbuild/openxr-sdk-populate-prefix"
  "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/openxr-sdk-subbuild/openxr-sdk-populate-prefix/tmp"
  "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/openxr-sdk-subbuild/openxr-sdk-populate-prefix/src/openxr-sdk-populate-stamp"
  "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/openxr-sdk-subbuild/openxr-sdk-populate-prefix/src"
  "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/openxr-sdk-subbuild/openxr-sdk-populate-prefix/src/openxr-sdk-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/openxr-sdk-subbuild/openxr-sdk-populate-prefix/src/openxr-sdk-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/openxr-sdk-subbuild/openxr-sdk-populate-prefix/src/openxr-sdk-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
