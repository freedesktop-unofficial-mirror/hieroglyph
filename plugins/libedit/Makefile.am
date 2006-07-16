NULL =

INCLUDES =						\
	$(HG_CFLAGS)					\
	$(NULL)

LIBS =							\
	@LDFLAGS@					\
	-export-dynamic					\
	$(HG_LIBS)					\
	$(LIBEDIT_LIBS)					\
	$(NULL)
LDADDS =						\
	$(top_builddir)/hieroglyph/libhieroglyph.la	\
	$(NULL)

plugindir = $(HIEROGLYPH_PLUGINDIR)
plugin_LTLIBRARIES =					\
	libext-libedit.la				\
	$(NULL)

libext_libedit_la_SOURCES =				\
	libedit-main.c					\
	$(NULL)
libext_libedit_la_CFLAGS =				\
	-DG_LOG_DOMAIN="\"Libedit-Extension\""		\
	$(CFLAGS)					\
	$(NULL)
libext_libedit_la_LDFLAGS =				\
	-avoid-version					\
	-module						\
	$(LDFLAGS)					\
	$(NULL)
libext_libedit_la_LIBADD =				\
	$(LDADDS)					\
	$(NULL)
