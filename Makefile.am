NULL =
SUBDIRS = hieroglyph src devices plugins lib tests examples po

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
