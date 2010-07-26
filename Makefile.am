NULL =
AUTOMAKE_OPTIONS = dist-bzip2
SUBDIRS = hieroglyph src lib tests

ACLOCAL_AMFLAGS =		\
	-I m4macros		\
	$(NULL)

CONFIGURE_DEPENDENCIES =	\
	requires		\
	$(NULL)

EXTRA_DIST =			\
	$(pkgconfig_DATA)	\
	$(noinst_DATA)		\
	requires		\
	$(NULL)

DISTCLEANFILES =		\
	$(pkgconfig_DATA)	\
	$(noinst_DATA)		\
	$(NULL)

#
#
#
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = hieroglyph.pc
$(pkgconfig_DATA): $(top_builddir)/config.status
#
noinst_DATA = hieroglyph-uninstalled.pc
$(noinst_DATA): $(top_builddir)/config.status
