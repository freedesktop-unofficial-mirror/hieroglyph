NULL =

INCLUDES =							\
	$(HG_CFLAGS)						\
	$(NULL)
LIBS =								\
	@LDFLAGS@						\
	$(HG_LIBS)						\
	$(NULL)
DEPS =								\
	$(top_builddir)/hieroglyph/libhieroglyph.la		\
	$(NULL)

noinst_PROGRAMS =						\
	hgspy-bin						\
	$(NULL)
noinst_LTLIBRARIES =						\
	libhgspy.la						\
	libhgspy-helper.la					\
	$(NULL)

hgspy_bin_SOURCES =						\
	hgspy.c							\
	$(NULL)
hgspy_bin_CFLAGS =						\
	$(GTK2_CFLAGS)						\
	$(HG_MODULE_CFLAGS)					\
	$(GTHREAD_CFLAGS)					\
	$(NULL)
hgspy_bin_LDADD =						\
	$(GTK2_LIBS)						\
	$(HG_MODULE_LIBS)					\
	$(GTHREAD_LIBS)						\
	libhgspy.la						\
	$(NULL)

libhgspy_la_SOURCES =						\
	visualizer.c						\
	visualizer.h						\
	$(NULL)
libhgspy_la_CFLAGS =						\
	$(GTK2_CFLAGS)						\
	$(NULL)
libhgspy_la_LIBADD =						\
	$(GTK2_LIBS)						\
	$(NULL)
libhgspy.la: $(libhgspy_la_OBJECTS) $(libhgspy_la_DEPENDENCIES)
	$(LINK) -rpath $(libdir) $(libhgspy_la_LDFLAGS) $(libhgspy_la_OBJECTS) $(libhgspy_la_LIBADD) $(LIBS)

libhgspy_helper_la_SOURCES =					\
	hgspy_helper.c						\
	hgspy_helper.h						\
	$(NULL)
libhgspy_helper_la_CFLAGS =					\
	$(GTK2_CFLAGS)						\
	$(HG_MODULE_CFLAGS)					\
	$(NULL)
libhgspy_helper_la_LDFLAGS =					\
	-avoid-version						\
	-module							\
	$(LDFLAGS)						\
	$(GTK2_LIBS)						\
	$(HG_MODULE_LIBS)					\
	libhgspy.la						\
	$(NULL)
libhgspy-helper.la: $(libhgspy_helper_la_OBJECTS) $(libhgspy_helper_la_DEPENDENCIES)
	$(LINK) -rpath $(libdir) $(libhgspy_helper_la_LDFLAGS) $(libhgspy_helper_la_OBJECTS) $(libhgspy_helper_la_LIBADD) $(LIBS)

EXTRA_DIST =							\
	hgspy							\
	$(NULL)
