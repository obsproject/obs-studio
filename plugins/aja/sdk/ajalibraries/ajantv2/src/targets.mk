# SPDX-License-Identifier: MIT
#
# Copyright (C) 2004 - 2021 AJA Video Systems, Inc.
#

#
# Here's the idea behind this makefile approach
# http://make.paulandlesley.org/multi-arch.html
#

SUFFIXES:

include $(NTV2_ROOT)/configure.mk

MAKETARGET = $(MAKE) -C $@ -f $(CURDIR)/Makefile \
                  SRCDIR=$(CURDIR) $(MAKECMDGOALS)

.PHONY: $(OBJDIR)
$(OBJDIR):
	+@[ -d $@ ] || mkdir -p $@
	+@$(MAKETARGET)

Makefile : ;
%.mk :: ;

% :: $(OBJDIR) ; :

#.PHONY: clean
#clean:
#	rm -rf $(OBJDIR)

