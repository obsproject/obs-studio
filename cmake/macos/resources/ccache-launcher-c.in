#!/bin/sh

if [[ "$1" == "${CMAKE_C_COMPILER}" ]] ; then
    shift
fi

export CCACHE_CPP2=true
export CCACHE_DEPEND=true
export CCACHE_DIRECT=true
export CCACHE_FILECLONE=true
export CCACHE_INODECACHE=true
export CCACHE_COMPILERCHECK='content'
export CCACHE_SLOPPINESS='modules,include_file_mtime,include_file_ctime,clang_index_store,system_headers'
exec "${CMAKE_C_COMPILER_LAUNCHER}" "${CMAKE_C_COMPILER}" "$@"
