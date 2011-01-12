##
# Global definitions
NULL =
INCLUDES =							\
	$(GLIB_CFLAGS)						\
	-I$(top_srcdir)						\
	-I$(top_srcdir)/hieroglyph				\
	-DHG_COMPILATION					\
	$(NULL)
LIBS =								\
	@LDFLAGS@						\
	-export-dynamic						\
	$(GLIB_LIBS)						\
	$(NULL)
LDADDS =							\
	$(top_builddir)/hieroglyph/libhieroglyph.la		\
	$(NULL)
EXTRA_DIST =							\
	$(hgpslib_DATA)						\
	$(NULL)
MAINTAINERCLEANFILES =						\
	$(NULL)
CLEANFILES =							\
	$(NULL)

##
# Local definitions

##
# Local Rules

##
# Target platform
hgpslibdir = $(HIEROGLYPH_LIBDIR)
hgpslib_DATA =							\
	hg_unittest.ps						\
	$(NULL)
plugindir = $(HIEROGLYPH_PLUGINDIR)
plugin_LTLIBRARIES =						\
	libext-unittest.la					\
	$(NULL)
#
libext_unittest_la_SOURCES =					\
	unittest-main.c						\
	$(NULL)
libext_unittest_la_CFLAGS =					\
	-DG_LOG_DOMAIN="\"Unittest-Extension\""			\
	$(CFLAGS)						\
	$(NULL)
libext_unittest_la_LDFLAGS =					\
	-avoid-version						\
	-module							\
	$(LDFLAGS)						\
	$(NULL)
libext_unittest_la_LIBADD =					\
	$(LDADDS)						\
	$(NULL)
