NULL =

INCLUDES =						\
	$(HG_CFLAGS)					\
	$(NULL)

LIBS =							\
	@LDFLAGS@					\
	-export-dynamic					\
	$(HG_LIBS)					\
	$(NULL)
LDADDS =						\
	$(top_builddir)/hieroglyph/libhieroglyph.la	\
	$(NULL)

plugindir = $(HIEROGLYPH_PLUGINDIR)
plugin_LTLIBRARIES =					\
	libext-test.la					\
	$(NULL)

libext_test_la_SOURCES =				\
	test-main.c					\
	$(NULL)
libext_test_la_CFLAGS =					\
	-DHG_LOG_DOMAIN="\"Test-Extension\""		\
	$(CFLAGS)					\
	$(NULL)
libext_test_la_LDFLAGS =				\
	-avoid-version					\
	-module						\
	$(LDFLAGS)					\
	$(NULL)
libext_test_la_LIBADD =					\
	$(LDADDS)					\
	$(NULL)

hgpslibdir = $(HIEROGLYPH_LIBDIR)
hgpslib_DATA =						\
	hg_unittest.ps					\
	$(NULL)

EXTRA_DIST =						\
	$(hgpslib_DATA)					\
	$(NULL)
