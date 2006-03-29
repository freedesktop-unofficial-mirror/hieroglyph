NULL =

INCLUDES =							\
	-DG_LOG_DOMAIN="\"Hieroglyph\""				\
	-DHIEROGLYPH_DEVICEDIR="\"$(HIEROGLYPH_DEVICEDIR)\""	\
	-DHIEROGLYPH_FILTERDIR="\"$(HIEROGLYPH_FILTERDIR)\""	\
	-DHIEROGLYPH_CMAPDIR="\"$(HIEROGLYPH_CMAPDIR)\""	\
	$(HG_CFLAGS)						\
	$(HG_MODULE_CFLAGS)					\
	$(NULL)
LIBS =								\
	@LDFLAGS@						\
	-export-dynamic						\
	$(HG_LIBS)						\
	$(HG_MODULE_LIBS)					\
	$(READLINE_LIBS)					\
	$(NULL)

function_prefix = hieroglyph
type_prefix = HIEROGLYPH

################################################################################
# Generated files
################################################################################
built_headers =							\
	version.h						\
	$(NULL)

################################################################################
# Publisher
################################################################################

#
# Header files
#
hgincludedir = $(includedir)/hieroglyph
hginclude_HEADERS =						\
	$(built_headers)					\
	hgallocator-ffit.h					\
	hgarray.h						\
	hgbtree.h						\
	hgdebug.h						\
	hgdevice.h						\
	hgdict.h						\
	hgfile.h						\
	hglineedit.h						\
	hgmacros.h						\
	hgmatrix.h						\
	hgmem.h							\
	hgpage.h						\
	hgpath.h						\
	hgrender.h						\
	hgstring.h						\
	hgtypes.h						\
	hgvaluenode.h						\
	$(NULL)

noinst_HEADERS =						\
	hgallocator-private.h					\
	$(NULL)

#
# Libraries
#
lib_LTLIBRARIES =						\
	libhieroglyph.la					\
	$(NULL)

libhieroglyph_la_SOURCES =					\
	hgallocator-ffit.c					\
	hgarray.c						\
	hgbtree.c						\
	hgdebug.c						\
	hgdevice.c						\
	hgdict.c						\
	hgfile.c						\
	hglineedit.c						\
	hgmatrix.c						\
	hgmem.c							\
	hgpage.c						\
	hgpath.c						\
	hgrender.c						\
	hgstring.c						\
	hgvaluenode.c						\
	$(NULL)

libhieroglyph_la_LDFLAGS =					\
	$(LDFLAGS)						\
	-lm							\
	$(NULL)

libhieroglyph_la_LIBADD =					\
	$(NULL)

$(libhieroglyph_la_OBJECTS): $(built_headers)
version.h.in: version.h.in.in $(top_srcdir)/update-version
	$(top_srcdir)/update-version

#
# Etc
#
EXTRA_DIST =							\
	version.h.in						\
	$(NULL)
