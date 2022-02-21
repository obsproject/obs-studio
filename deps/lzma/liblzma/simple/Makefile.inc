##
## Author: Lasse Collin
##
## This file has been put into the public domain.
## You can do whatever you want with this file.
##

liblzma_la_SOURCES += \
	simple/simple_coder.c \
	simple/simple_coder.h \
	simple/simple_private.h

if COND_ENCODER_SIMPLE
liblzma_la_SOURCES += \
	simple/simple_encoder.c \
	simple/simple_encoder.h
endif

if COND_DECODER_SIMPLE
liblzma_la_SOURCES += \
	simple/simple_decoder.c \
	simple/simple_decoder.h
endif

if COND_FILTER_X86
liblzma_la_SOURCES += simple/x86.c
endif

if COND_FILTER_POWERPC
liblzma_la_SOURCES += simple/powerpc.c
endif

if COND_FILTER_IA64
liblzma_la_SOURCES += simple/ia64.c
endif

if COND_FILTER_ARM
liblzma_la_SOURCES += simple/arm.c
endif

if COND_FILTER_ARMTHUMB
liblzma_la_SOURCES += simple/armthumb.c
endif

if COND_FILTER_SPARC
liblzma_la_SOURCES += simple/sparc.c
endif
