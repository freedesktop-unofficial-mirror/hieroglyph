NULL =

INCLUDES =						\
	$(HG_CFLAGS)					\
	$(NULL)
LIBS =							\
	@LDFLAGS@					\
	$(HG_LIBS)					\
	$(NULL)
LDADDS =						\
	$(top_builddir)/hieroglyph/libhieroglyph.la	\
	$(NULL)

noinst_PROGRAMS =					\
	pse						\
	cairo-xlib					\
	$(NULL)

pse_SOURCES =						\
	pse.c						\
	$(NULL)
pse_LDADD =						\
	$(LDADDS)					\
	$(NULL)

cairo_xlib_SOURCES =					\
	cairo-xlib.c					\
	$(NULL)
cairo_xlib_LDADD =					\
	$(LDADDS)					\
	$(NULL)
