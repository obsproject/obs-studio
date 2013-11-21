EXTRA_DIST = CHANGES LICENSE README.rst win32
SUBDIRS = doc src test

# "make distcheck" builds the dvi target, so use it to check that the
# documentation is built correctly.
dvi:
	$(MAKE) SPHINXOPTS_EXTRA=-W html

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = jansson.pc

if GCC
# These flags are gcc specific
export AM_CFLAGS = -Wall -Wextra -Wdeclaration-after-statement
endif
