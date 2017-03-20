##
## Author: Lasse Collin
##
## This file has been put into the public domain.
## You can do whatever you want with this file.
##

EXTRA_DIST += rangecoder/price_tablegen.c

liblzma_la_SOURCES += rangecoder/range_common.h

if COND_ENCODER_LZMA1
liblzma_la_SOURCES += \
	rangecoder/range_encoder.h \
	rangecoder/price.h \
	rangecoder/price_table.c
endif

if COND_DECODER_LZMA1
liblzma_la_SOURCES += rangecoder/range_decoder.h
endif
