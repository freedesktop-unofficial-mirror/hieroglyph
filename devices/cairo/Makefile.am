#
# Global definitions
NULL =
INCLUDES =						\
	$(GLIB_CFLAGS)					\
	$(CAIRO_CFLAGS)					\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/hieroglyph			\
	-DHG_COMPILATION				\
	$(NULL)
LIBS =							\
	@LDFLAGS@					\
	-export-dynamic					\
	$(GLIB_LIBS)					\
	$(CAIRO_LIBS)					\
	$(NULL)
LDADDS =						\
	$(top_builddir)/hieroglyph/libhieroglyph.la	\
	$(NULL)
EXTRA_DIST =						\
	$(NULL)

##
# Local definitions

##
# Local Rules

##
# Target platform
devicedir = $(HIEROGLYPH_DEVICEDIR)
device_LTLIBRARIES =					\
	libhgdev-cairo.la				\
	$(NULL)
#
libhgdev_cairo_la_SOURCES =				\
	cairo-main.c					\
	$(NULL)
libhgdev_cairo_la_CFLAGS =				\
	-DG_LOG_DOMAIN="\"Cairo-Device\""		\
	$(CFLAGS)					\
	$(NULL)
libhgdev_cairo_la_LDFLAGS =				\
	-avoid-version					\
	-module						\
	$(LDFLAGS)					\
	$(NULL)
libhgdev_cairo_la_LIBADD =				\
	$(LDADDS)					\
	-lgthread-2.0					\
	$(NULL)
