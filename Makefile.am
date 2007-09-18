NULL =
SUBDIRS = hieroglyph tests #src devices plugins lib tests examples po

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = hieroglyph.pc
$(pkgconfig_DATA): config.status

EXTRA_DIST =			\
	hieroglyph.pc.in	\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in	\
	requires		\
	update-version		\
	$(NULL)
