# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/wasmtime-src"
  "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/wasmtime-build"
  "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/wasmtime-subbuild/wasmtime-populate-prefix"
  "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/wasmtime-subbuild/wasmtime-populate-prefix/tmp"
  "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/wasmtime-subbuild/wasmtime-populate-prefix/src/wasmtime-populate-stamp"
  "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/wasmtime-subbuild/wasmtime-populate-prefix/src"
  "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/wasmtime-subbuild/wasmtime-populate-prefix/src/wasmtime-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/wasmtime-subbuild/wasmtime-populate-prefix/src/wasmtime-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/wasmtime-subbuild/wasmtime-populate-prefix/src/wasmtime-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
