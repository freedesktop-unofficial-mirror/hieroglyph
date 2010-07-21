##
# Global definitions
NULL =
INCLUDES =							\
	$(GLIB_CFLAGS)						\
	-I$(top_srcdir)						\
	$(NULL)
LIBS =								\
	@LDFLAGS@						\
	$(GLIB_LIBS)						\
	$(NULL)
DEPS =								\
	$(top_builddir)/hieroglyph/libhieroglyph.la		\
	$(NULL)
##
# Local definitions

##
# Local Rules

##
# Target platform
bin_PROGRAMS =							\
	hgs							\
	$(NULL)
#
hgs_SOURCES =							\
	hgs.c							\
	$(NULL)
hgs_CFLAGS =							\
	-DG_LOG_DOMAIN="\"hgs\""				\
	$(NULL)
hgs_LDADD =							\
	$(top_builddir)/hieroglyph/libhieroglyph.la		\
	$(NULL)
