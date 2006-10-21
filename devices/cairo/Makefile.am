NULL =

INCLUDES =						\
	$(HG_CFLAGS)					\
	$(CAIRO_CFLAGS)					\
	$(NULL)
LIBS =							\
	@LDFLAGS@					\
	-export-dynamic					\
	$(HG_LIBS)					\
	$(CAIRO_LIBS)					\
	$(NULL)
LDADDS =						\
	$(top_builddir)/hieroglyph/libhieroglyph.la	\
	$(NULL)

lib_LTLIBRARIES =					\
	libhieroglyph-cairo.la				\
	$(NULL)

libhieroglyph_cairo_la_SOURCES =			\
	hgdevice-cairo.c				\
	hgdevice-cairo.h				\
	$(NULL)
libhieroglyph_cairo_la_CFLAGS =				\
	-DHG_LOG_DOMAIN="\"Hieroglyph-Cairo\""		\
	$(CFLAGS)					\
	$(NULL)
libhieroglyph_cairo_la_LIBADD =				\
	$(LDADDS)					\
	$(NULL)

devicedir = $(HIEROGLYPH_DEVICEDIR)
device_LTLIBRARIES =					\
	libx11-device.la				\
	$(NULL)

libx11_device_la_SOURCES =				\
	cairo-xlib-main.c				\
	$(NULL)
libx11_device_la_CFLAGS =				\
	-DHG_LOG_DOMAIN="\"X11-Device\""		\
	$(CFLAGS)					\
	$(NULL)
libx11_device_la_LDFLAGS =				\
	-avoid-version					\
	-module						\
	$(LDFLAGS)					\
	$(NULL)
libx11_device_la_LIBADD =				\
	libhieroglyph-cairo.la				\
	$(LDADDS)					\
	$(NULL)
