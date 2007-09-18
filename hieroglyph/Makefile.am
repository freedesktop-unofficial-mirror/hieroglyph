NULL =

INCLUDES =							\
	-DG_LOG_DOMAIN="\"Hieroglyph\""				\
	$(HG_CFLAGS)						\
	$(NULL)
LIBS =								\
	@LDFLAGS@						\
	$(HG_LIBS)						\
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
	hgarray.h						\
	hgdict.h						\
	hgfile.h						\
	hgmacros.h						\
	hgobject.h						\
	hgoperator.h						\
	hgstack.h						\
	hgstring.h						\
	hgtypes.h						\
	utils.h							\
	vm.h							\
	$(NULL)

noinst_HEADERS =						\
	hgdictprime.h						\
	hgoperator-private.h					\
	$(NULL)

#
# Libraries
#
lib_LTLIBRARIES =						\
	libhieroglyph.la					\
	$(NULL)

libhieroglyph_la_SOURCES =					\
	hgarray.c						\
	hgdict.c						\
	hgfile.c						\
	hgobject.c						\
	hgoperator.c						\
	hgstack.c						\
	hgstring.c						\
	utils.c							\
	vm.c							\
	$(NULL)

libhieroglyph_la_LDFLAGS =					\
	$(LDFLAGS)						\
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
	version.h.in.in						\
	$(NULL)
