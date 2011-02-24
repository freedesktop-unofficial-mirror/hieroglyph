##
# Global definitions
NULL =
SUBDIRS = ps

INCLUDES =						\
	$(GLIB_CFLAGS)					\
	$(CHECK_CFLAGS)					\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/hieroglyph			\
	-DHG_COMPILATION				\
	$(NULL)
DEPS =							\
	$(top_builddir)/hieroglyph/libhieroglyph.la	\
	$(NULL)
LIBS =							\
	@LDFLAGS@					\
	$(GLIB_LIBS)					\
	$(CHECK_LIBS)					\
	$(top_builddir)/hieroglyph/libhieroglyph.la	\
	$(NULL)
EXTRA_DIST =						\
	$(NULL)
TESTS =							\
	$(noinst_PROGRAMS)				\
	$(NULL)


##
# Local definitions
common_private_headers =				\
	main.h						\
	$(NULL)
common_sources =					\
	main.c						\
	$(NULL)

##
# Local Rules


##
# Target platform
noinst_HEADERS =					\
	$(common_private_headers)			\
	$(NULL)
noinst_PROGRAMS =					\
	$(NULL)
if ENABLE_UNIT_TEST
noinst_PROGRAMS +=					\
	test-hgallocator				\
	test-hgarray					\
	test-hgbool					\
	test-hgdict					\
	test-hgencoding					\
	test-hgint					\
	test-hgmark					\
	test-hgmem					\
	test-hgname					\
	test-hgnull					\
	test-hgquark					\
	test-hgreal					\
	test-hgstack					\
	test-hgstring					\
	$(NULL)
endif
#
test_hgallocator_SOURCES =				\
	hgallocator.c					\
	$(common_sources)				\
	$(NULL)
#
test_hgarray_SOURCES =					\
	hgarray.c					\
	$(common_sources)				\
	$(NULL)
#
test_hgbool_SOURCES =					\
	hgbool.c					\
	$(common_sources)				\
	$(NULL)
#
test_hgdict_SOURCES =					\
	hgdict.c					\
	$(common_sources)				\
	$(NULL)
#
test_hgencoding_SOURCES =				\
	hgencoding.c					\
	$(common_sources)				\
	$(NULL)
#
test_hgint_SOURCES =					\
	hgint.c						\
	$(common_sources)				\
	$(NULL)
#
test_hgmark_SOURCES =					\
	hgmark.c					\
	$(common_sources)				\
	$(NULL)
#
test_hgmem_SOURCES =					\
	hgmem.c						\
	$(common_sources)				\
	$(NULL)
#
test_hgname_SOURCES =					\
	hgname.c					\
	$(common_sources)				\
	$(NULL)
#
test_hgnull_SOURCES =					\
	hgnull.c					\
	$(common_sources)				\
	$(NULL)
#
test_hgquark_SOURCES =					\
	hgquark.c					\
	$(common_sources)				\
	$(NULL)
#
test_hgreal_SOURCES =					\
	hgreal.c					\
	$(common_sources)				\
	$(NULL)
#
test_hgstack_SOURCES =					\
	hgstack.c					\
	$(common_sources)				\
	$(NULL)
#
test_hgstring_SOURCES =					\
	hgstring.c					\
	$(common_sources)				\
	$(NULL)
