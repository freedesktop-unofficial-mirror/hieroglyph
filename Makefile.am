NULL =
SUBDIRS = hieroglyph libretto src devices lib tests examples

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = hg-libretto.pc hieroglyph.pc
$(pkgconfig_DATA): config.status

EXTRA_DIST =			\
	hg-libretto.pc.in	\
	hieroglyph.pc.in	\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in	\
	requires		\
	$(NULL)
