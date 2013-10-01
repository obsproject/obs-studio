SRCPATH=.
CC=gcc
CCDEP=gcc
CFLAGS=-std=gnu99 -I$(SRCPATH)
CXXFLAGS=-std=c++11 -I$(SRCPATH)
#CPPFLAGS=-Wall -msse2 -O3 -ffast-math -fomit-frame-pointer
CPPFLAGS=-Wall -msse2 -g
LD=gcc -o 
LDFLAGS=-shared
OBJ=o
SOEXT=dll
EXT=.exe
