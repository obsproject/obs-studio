# SPDX-License-Identifier: MIT
#
# Copyright (C) 2004 - 2021 AJA Video Systems, Inc.
#

#
# This file controls the SDK versioning
#
# Check in this file to update the major, minor, and point version numbers
#
# The build number will be set by the TeamCity automated builder
#

SDKVER_MAJ = 16
SDKVER_MIN = 2
SDKVER_PNT = 0

ifeq ($(TC_BUILD_COUNTER),)
  SDKVER_BLD = 0
  OLD_SDK_VERSION_FORMAT = 1 
  export OLD_SDK_VERSION_FORMAT
  ifeq ($(AJA_DEBUG),1)
    RELEASEVER = $(SDKVER_MAJ).$(SDKVER_MIN).$(SDKVER_PNT)D
	SDKVER_STR = "d"
  else
    RELEASEVER = $(SDKVER_MAJ).$(SDKVER_MIN).$(SDKVER_PNT)
    ifeq ($(AJA_BETA),1)
		SDKVER_STR = "b"
    else
		SDKVER_STR = ""
    endif
  endif
else
  SDKVER_BLD = $(TC_BUILD_COUNTER)
  ifeq ($(AJA_DEBUG),1)
    RELEASEVER = $(SDKVER_MAJ).$(SDKVER_MIN).$(SDKVER_PNT).$(SDKVER_BLD)D
	SDKVER_STR = "d"
  else
    RELEASEVER = $(SDKVER_MAJ).$(SDKVER_MIN).$(SDKVER_PNT).$(SDKVER_BLD)
    ifeq ($(AJA_BETA),1)
		SDKVER_STR = "b"
    else
		SDKVER_STR = ""
    endif
  endif
endif

export SDKVER_MAJ
export SDKVER_MIN
export SDKVER_PNT
export SDKVER_BLD
export SDKVER_STR

