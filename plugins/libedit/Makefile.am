##
# Global definitions
NULL =
INCLUDES =							\
	$(GLIB_CFLAGS)						\
	$(LIBEDIT_CFLAGS)					\
	-I$(top_srcdir)						\
	$(NULL)
LIBS =								\
	@LDFLAGS@						\
	-export-dynamic						\
	$(GLIB_LIBS)						\
	$(LIBEDIT_LIBS)						\
	$(NULL)
LDADDS =							\
	$(top_builddir)/hieroglyph/libhieroglyph.la		\
	$(NULL)
EXTRA_DIST =							\
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
plugindir = $(HIEROGLYPH_PLUGINDIR)
plugin_LTLIBRARIES =						\
	libext-libedit.la					\
	$(NULL)
#
libext_libedit_la_SOURCES =					\
	libedit-main.c						\
	$(NULL)
libext_libedit_la_CFLAGS =					\
	-DG_LOG_DOMAIN="\"Libedit-Extension\""			\
	$(CFLAGS)						\
	$(NULL)
libext_libedit_la_LDFLAGS =					\
	-avoid-version						\
	-module							\
	$(LDFLAGS)						\
	$(NULL)
libext_libedit_la_LIBADD =					\
	$(LDADDS)						\
	$(NULL)
