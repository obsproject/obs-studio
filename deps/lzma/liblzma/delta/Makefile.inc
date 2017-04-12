##
## Author: Lasse Collin
##
## This file has been put into the public domain.
## You can do whatever you want with this file.
##

liblzma_la_SOURCES += \
	delta/delta_common.c \
	delta/delta_common.h \
	delta/delta_private.h

if COND_ENCODER_DELTA
liblzma_la_SOURCES += \
	delta/delta_encoder.c \
	delta/delta_encoder.h
endif

if COND_DECODER_DELTA
liblzma_la_SOURCES += \
	delta/delta_decoder.c \
	delta/delta_decoder.h
endif
