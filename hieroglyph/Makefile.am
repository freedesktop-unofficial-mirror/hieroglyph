NULL =

INCLUDES =							\
	-DG_LOG_DOMAIN="\"Hieroglyph\""				\
	-DHIEROGLYPH_DEVICEDIR="\"$(HIEROGLYPH_DEVICEDIR)\""	\
	-DHIEROGLYPH_PLUGINDIR="\"$(HIEROGLYPH_PLUGINDIR)\""	\
	-DHIEROGLYPH_CMAPDIR="\"$(HIEROGLYPH_CMAPDIR)\""	\
	-DHIEROGLYPH_LIBDIR="\"$(HIEROGLYPH_LIBDIR)\""		\
	$(HG_CFLAGS)						\
	$(HG_MODULE_CFLAGS)					\
	$(NULL)
LIBS =								\
	@LDFLAGS@						\
	-export-dynamic						\
	$(HG_LIBS)						\
	$(HG_MODULE_LIBS)					\
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
	hgallocator-bfit.h					\
	hgarray.h						\
	hgbtree.h						\
	hgdebug.h						\
	hgdevice.h						\
	hgdict.h						\
	hgfile.h						\
	hggraphics.h						\
	hglineedit.h						\
	hgmacros.h						\
	hgmatrix.h						\
	hgmem.h							\
	hgpage.h						\
	hgpath.h						\
	hgplugins.h						\
	hgrender.h						\
	hgstack.h						\
	hgstring.h						\
	hgtypes.h						\
	hgvaluenode.h						\
	operator.h						\
	scanner.h						\
	vm.h							\
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
	hgallocator-bfit.c					\
	hgarray.c						\
	hgbtree.c						\
	hgdebug.c						\
	hgdevice.c						\
	hgdict.c						\
	hgfile.c						\
	hggraphics.c						\
	hglineedit.c						\
	hgmatrix.c						\
	hgmem.c							\
	hgpage.c						\
	hgpath.c						\
	hgplugins.c						\
	hgrender.c						\
	hgstack.c						\
	hgstring.c						\
	hgvaluenode.c						\
	operator.c						\
	scanner.c						\
	vm.c							\
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
