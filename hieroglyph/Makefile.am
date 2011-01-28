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
	-DHG_COMPILATION					\
	$(NULL)
LIBS =								\
	@LDFLAGS@						\
	$(GLIB_LIBS)						\
	$(GMODULE_LIBS)						\
	-lm							\
	$(NULL)
EXTRA_DIST =							\
	hgversion.h.in						\
	$(NULL)
MAINTAINERCLEANFILES =						\
	$(NULL)
CLEANFILES =							\
	$(hg_proto_files)					\
	hglineedit.c						\
	hgscanner.c						\
	$(NULL)
BUILT_SOURCES =							\
	$(hg_built_headers)					\
	$(hg_built_sources)					\
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
	hgmatrix.h						\
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
	hgtypebit-private.h					\
	$(NULL)
hg_built_public_headers =					\
	hgversion.h						\
	$(NULL)
hg_built_private_headers =					\
	$(hg_proto_files)					\
	$(NULL)
hg_built_headers =						\
	$(hg_built_public_headers)				\
	$(hg_built_private_headers)				\
	$(NULL)
#
hg_built_sources =						\
	$(NULL)
hg_lex_sources =						\
	hglineedit.l						\
	hgscanner.l						\
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
	hgmatrix.c						\
	hgmem.c							\
	hgname.c						\
	hgpath.c						\
	hgobject.c						\
	hgoperator.c						\
	hgplugin.c						\
	hgquark.c						\
	hgsnapshot.c						\
	hgstack.c						\
	hgstring.c						\
	hgvm.c							\
	$(NULL)
#
hg_proto_files =						\
	$(hg_lex_sources:.l=.proto.h)				\
	$(hg_sources:.c=.proto.h)				\
	$(NULL)
#
stamp_files =							\
	$(NULL)
#
LEX_OUTPUT_ROOT=lex.$(basename $<)

##
# Local Rules
%.proto.h: %.c
	$(AM_V_GEN)[[ ! -f $(<:.c=.l) ]] && $(top_srcdir)/build_tools/genfuncdecls.sh $< > gfp || $(top_srcdir)/build_tools/genfuncdecls.sh $(<:.c=.l) > gfp;	\
	(cmp -s gfp $@ || cp gfp $@)	\
	&& rm -f gfp

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
	$(hg_lex_sources)					\
	$(NULL)
libhieroglyph_la_CFLAGS =					\
	-DG_LOG_DOMAIN="\"Hieroglyph\""				\
	$(NULL)
libhieroglyph_la_LDFLAGS =					\
	$(LDFLAGS)						\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
	$(NULL)
