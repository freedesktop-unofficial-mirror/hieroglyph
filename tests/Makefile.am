##
# Global definitions
NULL =
INCLUDES =						\
	$(HG_CFLAGS)					\
	$(CHECK_CFLAGS)					\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/hieroglyph			\
	$(NULL)
DEPS =							\
	$(top_builddir)/hieroglyph/libhieroglyph.la	\
	$(NULL)
LIBS =							\
	@LDFLAGS@					\
	$(HG_LIBS)					\
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
	$(NULL)
endif
#
test_hgallocator_SOURCES =				\
	hgallocator.c					\
	$(common_sources)				\
	$(NULL)
