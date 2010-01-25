##
# Global definitions
NULL =
INCLUDES =					\
	$(HG_CFLAGS)				\
	$(NULL)
LIBS =						\
	@LDFLAGS@				\
	$(HG_LIBS)				\
	$(NULL)
EXTRA_DIST =					\
	hgversion.h.in				\
	hgversion.h.in.in			\
	$(NULL)
MAINTAINERCLEANFILES =				\
	$(NULL)
CLEANFILES =					\
	$(NULL)

##
# Local definitions
mkprefix = hg

$(mkprefix)_public_headers =			\
	$(NULL)
$(mkprefix)_private_headers =			\
	$(NULL)
$(mkprefix)_built_public_headers =		\
	hgversion.h				\
	$(NULL)
$(mkprefix)_built_private_headers =		\
	$(NULL)
$(mkprefix)_built_headers =			\
	$($(mkprefix)_built_public_headers)	\
	$($(mkprefix)_built_private_headers)	\
	$(NULL)
#
$(mkprefix)_built_sources =			\
	$(NULL)
$(mkprefix)_sources =				\
	$($(mkprefix)_built_sources)		\
	$(NULL)
#
stamp_files =					\
	$(NULL)

##
# Local Rules
$(libhieroglyph_la_OBJECTS): $($(mkprefix)_built_headers)
hgversion.h.in: hgversion.h.in.in $(top_srcdir)/update-version
	$(top_srcdir)/update-version

##
# Target platform
lib_LTLIBRARIES =				\
	libhieroglyph.la			\
	$(NULL)
#
$(mkprefix)includedir = $(includedir)/hieroglyph
$(mkprefix)include_HEADERS =			\
	$($(mkprefix)_public_headers)		\
	$($(mkprefix)_built_public_headers)	\
	$(NULL)
#
noinst_HEADERS =				\
	$($(mkprefix)_private_headers)		\
	$($(mkprefix)_built_private_headers)	\
	$(NULL)
#
libhieroglyph_la_SOURCES =			\
	$($(mkprefix)_sources)					\
	$(NULL)
libhieroglyph_la_CFLAGS =					\
	-DG_LOG_DOMAIN="\"Hieroglyph\""				\
	$(NULL)
libhieroglyph_la_LDFLAGS =					\
	$(LDFLAGS)						\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
	$(NULL)
