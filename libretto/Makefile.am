NULL =

INCLUDES =							\
	-DG_LOG_DOMAIN="\"Libretto\""				\
	-DHIEROGLYPH_LIBDIR="\"$(HIEROGLYPH_LIBDIR)\""		\
	-D_GNU_SOURCE						\
	-D__USE_POSIX						\
	$(HG_CFLAGS)						\
	$(NULL)
LIBS =								\
	@LDFLAGS@						\
	-export-dynamic						\
	-lm							\
	$(HG_LIBS)						\
	$(NULL)
LDADDS =							\
	$(top_builddir)/hieroglyph/libhieroglyph.la		\
	$(NULL)


lrincludedir = $(includedir)/libretto
lrinclude_HEADERS =						\
	lbgraphics.h						\
	lbtypes.h						\
	operator.h						\
	scanner.h						\
	vm.h							\
	$(NULL)

lib_LTLIBRARIES =						\
	libretto.la						\
	$(NULL)


libretto_la_SOURCES =						\
	$(lrinclude_HEADERS)					\
	lbgraphics.c						\
	operator.c						\
	scanner.c						\
	vm.c							\
	$(NULL)
libretto_la_LDFLAGS =						\
	$(LDFLAGS)						\
	$(NULL)
libretto_la_LIBADD =						\
	$(LDADDS)						\
	$(NULL)
libretto_la_DEPENDENCIES =					\
	$(top_builddir)/hieroglyph/libhieroglyph.la		\
	$(NULL)

EXTRA_DIST =							\
	$(NULL)
