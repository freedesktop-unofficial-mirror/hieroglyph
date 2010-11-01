##
# Global definitions
NULL =
INCLUDES =							\
	$(GLIB_CFLAGS)						\
	$(GMODULE_CFLAGS)					\
	-I$(top_srcdir)						\
	-DHIEROGLYPH_LIBDIR="\"$(HIEROGLYPH_LIBDIR)\""		\
	-DHIEROGLYPH_PLUGINDIR="\"$(HIEROGLYPH_PLUGINDIR)\""	\
	-DHIEROGLYPH_DEVICEDIR="\"$(HIEROGLYPH_DEVICEDIR)\""	\
	$(NULL)
LIBS =								\
	@LDFLAGS@						\
	$(GLIB_LIBS)						\
	$(GMODULE_LIBS)						\
	-lm							\
	$(NULL)
EXTRA_DIST =							\
	hgversion.h.in						\
	hgversion.h.in.in					\
	$(NULL)
MAINTAINERCLEANFILES =						\
	$(NULL)
CLEANFILES =							\
	hglineedit.c						\
	hgscanner.c						\
	$(NULL)

##
# Local definitions
hg_public_headers =						\
	hgallocator.h						\
	hgarray.h						\
	hgbool.h						\
	hgdevice.h						\
	hgdict.h						\
	hgencoding.h						\
	hgerror.h						\
	hgfile.h						\
	hggstate.h						\
	hgint.h							\
	hglineedit.h						\
	hgmacros.h						\
	hgmark.h						\
	hgmem.h							\
	hgname.h						\
	hgnull.h						\
	hgpath.h						\
	hgobject.h						\
	hgoperator.h						\
	hgplugin.h						\
	hgquark.h						\
	hgreal.h						\
	hgsnapshot.h						\
	hgstack.h						\
	hgstring.h						\
	hgtypes.h						\
	hgvm.h							\
	$(NULL)
hg_private_headers =						\
	hgallocator-private.h					\
	hgdict-private.h					\
	hgmem-private.h						\
	hgstack-private.h					\
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
	hgarray.c						\
	hgdevice.c						\
	hgdict.c						\
	hgencoding.c						\
	hgerror.c						\
	hgfile.c						\
	hggstate.c						\
	hglineedit.l						\
	hgmem.c							\
	hgname.c						\
	hgpath.c						\
	hgobject.c						\
	hgoperator.c						\
	hgplugin.c						\
	hgscanner.l						\
	hgsnapshot.c						\
	hgstack.c						\
	hgstring.c						\
	hgvm.c							\
	$(NULL)
#
stamp_files =							\
	$(NULL)
#
LEX_OUTPUT_ROOT=lex.$(basename $<)

##
# Local Rules
###$(libhieroglyph_la_OBJECTS): $(hg_built_headers)
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
