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
	libext-debug.la					\
	$(NULL)

libext_debug_la_SOURCES =				\
	debug-main.c					\
	$(NULL)
libext_debug_la_CFLAGS =				\
	-DHG_LOG_DOMAIN="\"Debug-Extension\""		\
	$(CFLAGS)					\
	$(NULL)
libext_debug_la_LDFLAGS =				\
	-avoid-version					\
	-module						\
	$(LDFLAGS)					\
	$(NULL)
libext_debug_la_LIBADD =				\
	$(LDADDS)					\
	$(NULL)
