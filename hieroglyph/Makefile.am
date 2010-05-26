##
# Global definitions
NULL =
INCLUDES =							\
	$(HG_CFLAGS)						\
	-I$(top_srcdir)						\
	$(NULL)
LIBS =								\
	@LDFLAGS@						\
	$(HG_LIBS)						\
	-lm							\
	$(NULL)
EXTRA_DIST =							\
	hgversion.h.in						\
	hgversion.h.in.in					\
	$(NULL)
MAINTAINERCLEANFILES =						\
	$(NULL)
CLEANFILES =							\
	$(NULL)

##
# Local definitions
hg_public_headers =						\
	hgallocator.h						\
	hgbool.h						\
	hgencoding.h						\
	hgerror.h						\
	hgint.h							\
	hgmacros.h						\
	hgmark.h						\
	hgmem.h							\
	hgnull.h						\
	hgobject.h						\
	hgtypes.h						\
	hgutils.h						\
	$(NULL)
hg_private_headers =						\
	hgallocator-private.h					\
	hgmem-private.h						\
	$(NULL)
hg_built_public_headers =					\
	hgversion.h						\
	$(NULL)
hg_built_private_headers =					\
	$(NULL)
hg_built_headers =						\
	$(hg_built_public_headers)				\
	$(hg_built_private_headers)				\
	$(NULL)
#
hg_built_sources =						\
	$(NULL)
hg_sources =							\
	$(hg_built_sources)					\
	hgallocator.c						\
	hgbool.c						\
	hgencoding.c						\
	hgerror.c						\
	hgint.c							\
	hgmark.c						\
	hgmem.c							\
	hgnull.c						\
	hgobject.c						\
	hgutils.c						\
	$(NULL)
#
stamp_files =							\
	$(NULL)

##
# Local Rules
$(libhieroglyph_la_OBJECTS): $(hg_built_headers)
hgversion.h.in: hgversion.h.in.in $(top_srcdir)/update-version
	$(top_srcdir)/update-version

##
# Target platform
lib_LTLIBRARIES =						\
	libhieroglyph.la					\
	$(NULL)
#
hgincludedir = $(includedir)/hieroglyph
hginclude_HEADERS =						\
	$(hg_public_headers)					\
	$(hg_built_public_headers)				\
	$(NULL)
#
noinst_HEADERS =						\
	$(hg_private_headers)					\
	$(hg_built_private_headers)				\
	$(NULL)
#
libhieroglyph_la_SOURCES =					\
	$(hg_sources)						\
	$(NULL)
libhieroglyph_la_CFLAGS =					\
	-DG_LOG_DOMAIN="\"Hieroglyph\""				\
	$(NULL)
libhieroglyph_la_LDFLAGS =					\
	$(LDFLAGS)						\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
	$(NULL)
