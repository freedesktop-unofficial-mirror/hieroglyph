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

lib_LTLIBRARIES =					\
	libhieroglyph-x11.la				\
	$(NULL)

libhieroglyph_x11_la_SOURCES =				\
	hgdevice-x11.c					\
	hgdevice-x11.h					\
	$(NULL)
libhieroglyph_x11_la_LIBADD =				\
	$(LDADDS)					\
	$(NULL)

devicedir = $(HIEROGLYPH_DEVICEDIR)
device_LTLIBRARIES =					\
	libx11-embedded-device.la			\
	$(NULL)

libx11_embedded_device_la_SOURCES =			\
	x11-embedded-main.c				\
	$(NULL)
libx11_embedded_device_la_CFLAGS =			\
	-DHG_LOG_DOMAIN="\"X11-Embedded-Device\""	\
	$(CFLAGS)					\
	$(NULL)
libx11_embedded_device_la_LDFLAGS =			\
	-avoid-version					\
	-module						\
	$(LDFLAGS)					\
	$(NULL)
libx11_embedded_device_la_LIBADD =			\
	libhieroglyph-x11.la				\
	$(LDADDS)					\
	$(NULL)
