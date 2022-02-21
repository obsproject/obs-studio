EXTRA_DIST = run check-exports

check_PROGRAMS = \
	test_array \
	test_copy \
	test_dump \
	test_dump_callback \
	test_equal \
	test_load \
	test_loadb \
	test_load_callback \
	test_memory_funcs \
	test_number \
	test_object \
	test_pack \
	test_simple \
	test_unpack

test_array_SOURCES = test_array.c util.h
test_copy_SOURCES = test_copy.c util.h
test_dump_SOURCES = test_dump.c util.h
test_dump_callback_SOURCES = test_dump_callback.c util.h
test_load_SOURCES = test_load.c util.h
test_loadb_SOURCES = test_loadb.c util.h
test_memory_funcs_SOURCES = test_memory_funcs.c util.h
test_number_SOURCES = test_number.c util.h
test_object_SOURCES = test_object.c util.h
test_pack_SOURCES = test_pack.c util.h
test_simple_SOURCES = test_simple.c util.h
test_unpack_SOURCES = test_unpack.c util.h

AM_CPPFLAGS = -I$(top_builddir)/src -I$(top_srcdir)/src
LDFLAGS = -static  # for speed and Valgrind
LDADD = $(top_builddir)/src/libjansson.la
