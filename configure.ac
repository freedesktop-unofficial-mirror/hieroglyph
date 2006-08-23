dnl Process this file with autoconf to produce a configure script.
AC_INIT([hieroglyph], 0.0.1, [at@gclab.org])

. ./requires

AM_INIT_AUTOMAKE($PACKAGE_NAME, $PACKAGE_VERSION)
AM_MAINTAINER_MODE
AM_CONFIG_HEADER(config.h)

AC_PROG_LIBTOOL
AC_PROG_INTLTOOL

AC_PROG_CC

AC_SYS_LARGEFILE

dnl ======================================================================
dnl initialize values
dnl ======================================================================
pluginslist="debug test"

dnl ======================================================================
dnl set CFLAGS
dnl ======================================================================
changequote(,)dnl
if test "x$GCC" = "xyes"; then
	case " $CFLAGS " in
	*[\ \	]-Wall[\ \	]*) ;;
	*) CFLAGS="$CFLAGS -Wall"   ;;
	esac

	if test "x$enable_maintainer_mode" = "xyes"; then
		case " $CFLAGS " in
		*[\ \	]-Werror[\ \	]*) ;;
		*) CFLAGS="$CFLAGS -Werror" ;;
		esac
		CFLAGS="`echo $CFLAGS | sed -e 's/-O./-O0/'`"
	fi

	if test "x$enable_ansi" = "xyes"; then
		case " $CFLAGS " in
		*[\ \	]-ansi[\ \	]*) ;;
		*) CFLAGS="$CFLAGS -ansi"   ;;
		esac

		case " $CFLAGS " in
		*[\ \	]-pedantic[\ \	]*)   ;;
		*) CFLAGS="$CFLAGS -pedantic" ;;
		esac
	fi
fi
changequote([,])dnl

dnl ======================================================================
dnl functions testing
dnl ======================================================================
AC_CHECK_FUNC(exp10,,[
	AC_MSG_CHECKING([for exp10 whether _GNU_SOURCE needs to be defined.])
	SAVE_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS -D_GNU_SOURCE"
	AC_COMPILE_IFELSE([
#include <math.h>
void *foo = exp10;
	], [
		AC_MSG_RESULT([-D_GNU_SOURCE])
	] ,[
		AC_MSG_ERROR([FIXME: need exp10 function anyway.])
	])
])

dnl ======================================================================
dnl gettext stuff
dnl ======================================================================
GETTEXT_PACKAGE=$PACKAGE
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext package])

ALL_LINGUAS="ja"
AM_GLIB_GNU_GETTEXT
AM_GLIB_DEFINE_LOCALEDIR(HGSPY_LOCALEDIR)

dnl ======================================================================
dnl check pkg-config stuff
dnl ======================================================================
PKG_CHECK_MODULES(HG, glib-2.0)
PKG_CHECK_MODULES(HG_MODULE, gmodule-2.0)
PKG_CHECK_MODULES(GTK2, gtk+-2.0)
PKG_CHECK_MODULES(GTHREAD, gthread-2.0)
AC_SUBST(HG_CFLAGS)
AC_SUBST(HG_LIBS)
AC_SUBST(HG_MODULE_CFLAGS)
AC_SUBST(HG_MODULE_LIBS)
AC_SUBST(GTK2_CFLAGS)
AC_SUBST(GTK2_LIBS)
AC_SUBST(GTHREAD_CFLAGS)
AC_SUBST(GTHREAD_LIBS)

dnl ======================================================================
dnl check glib-2.0 stuff
dnl ======================================================================
AM_PATH_GLIB_2_0($GLIB_REQUIRED, :,
	AC_MSG_ERROR([
*** GLIB $GLIB_REQUIRED_VERSION or better is required. The latest version of 
*** GLIB is always available from ftp://ftp.gtk.org/.]),
  gobject gmodule gthread)

dnl ======================================================================
dnl check another libraries
dnl ======================================================================
AC_CHECK_LIB(edit, readline, [
	LIBEDIT_LIBS="-ledit"
	AC_DEFINE(USE_LIBEDIT,1,[build with readline])
	pluginslist="$pluginslist libedit"
	],[
	LIBEDIT_LIBS=""
	])
AC_SUBST(LIBEDIT_LIBS)
AM_CONDITIONAL(ENABLE_LIBEDIT, test x$LIBEDIT_LIBS != x)

dnl ======================================================================
dnl options
dnl ======================================================================
AC_ARG_ENABLE(debug,
	AC_HELP_STRING([--enable-debug], [build with the debug code.])
	,,
	enable_debug=no)
AC_ARG_ENABLE(profile,
	AC_HELP_STRING([--enable-profile], [build with the profiler option.])
	,,
	enable_profile=no)
AC_ARG_ENABLE(rebuilds,
	AC_HELP_STRING([--disable-rebuilds], [disable all source autogeneration rules])
	,,
	enable_rebuilds=yes)
AC_ARG_WITH(cairo,
	AC_HELP_STRING([--without-cairo], [build without the cairo support.])
	,,
	with_cairo=yes)
AC_ARG_WITH(x11,
	AC_HELP_STRING([--without-x11], [build without the x11 embedded support.])
	,,
	with_x11=yes)
AC_ARG_WITH(cmap,
	AC_HELP_STRING([--with-cmap=DIR], [build with the CMap support.])
	,,
	with_cmap=no)

dnl ======================================================================
dnl options - debug
dnl ======================================================================
AC_MSG_CHECKING(whether to enable the debuggin build)
if test "x$USE_MAINTAINER_MODE" = xyes; then
	AC_MSG_RESULT(force)
	enable_debug=force
	AC_DEFINE(DEBUG,,[enable the debuggin build])
elif test "x$enable_debug" = xno; then
	AC_MSG_RESULT(no)
else
	AC_MSG_RESULT(yes)
	AC_DEFINE(DEBUG,,[enable the debuggin build])
fi
AM_CONDITIONAL(ENABLE_DEBUG, test x$enable_debug != xno)

dnl ======================================================================
dnl options - profile
dnl ======================================================================
AC_MSG_CHECKING(whether to enable the profiler)
if test "x$enable_profile" = xno; then
	AC_MSG_RESULT(no)
else
	AC_MSG_RESULT(yes)
	changequote(,) dnl
	CFLAGS="`echo $CFLAGS | sed -e 's/[ \t]-O[0-9][ \t]/ /'` -pg"
	changequote([,]) dnl
fi

dnl ======================================================================
dnl options - rebuilds
dnl ======================================================================
REBUILD=\#
if test "x$enable_rebuilds" = xyes; then
	REBUILD=
fi
AC_SUBST(REBUILD)

dnl ======================================================================
dnl options - with cairo
dnl ======================================================================
if test "x$with_cairo" = xyes; then
	PKG_CHECK_MODULES(CAIRO, cairo >= $CAIRO_REQUIRED)
	AC_SUBST(CAIRO_CFLAGS)
	AC_SUBST(CAIRO_LIBS)
	devices="${devices}cairo "
fi
AM_CONDITIONAL(ENABLE_CAIRO, test x$with_cairo != xno)

dnl ======================================================================
dnl options - with x11
dnl ======================================================================
if test "x$with_x11" = xyes; then
   	AC_PATH_XTRA
	if test "x$no_x" = xyes; then
		AC_MSG_ERROR([X development libraries not found])
	fi
	X11_CFLAGS="$X_CFLAGS"
	X11_LIBS="$X_LIBS $X_EXTRA_LIBS"
	AC_SUBST(X11_CFLAGS)
	AC_SUBST(X11_LIBS)
	devices="${devices}x11-embedded "
fi
AM_CONDITIONAL(ENABLE_X11_EMBEDDED, test x$with_x11 != xno)

dnl ======================================================================
dnl options - with CMap
dnl ======================================================================
if test "x$with_cmap" = xyes; then
	HIEROGLYPH_CMAPDIR=${datadir}/ghostscript/CMap
	AC_DEFINE(USE_CMAP,1,[enable the CMap support.])
elif test "x$with_cmap" = xno; then
	HIEROGLYPH_CMAPDIR=
else
	HIEROGLYPH_CMAPDIR=$with_cmap
	AC_DEFINE(USE_CMAP,1,[enable the CMap support.])
	with_cmap=yes
fi
AC_SUBST(HIEROGLYPH_CMAPDIR)

dnl ======================================================================
dnl variables
dnl ======================================================================
changequote(,) dnl
HIEROGLYPH_MAJOR_VERSION=`echo $PACKAGE_VERSION | sed -e 's/\(^[0-9]*\)\.[0-9]*\.[0-9]*$/\1/'`
HIEROGLYPH_MINOR_VERSION=`echo $PACKAGE_VERSION | sed -e 's/^[0-9]*\.\([0-9]*\)\.[0-9]*$/\1/'`
HIEROGLYPH_RELEASE_VERSION=`echo $PACKAGE_VERSION | sed -e 's/^[0-9]*\.[0-9]*\.\([0-9]*\)$/\1/'`
changequote([,]) dnl
HIEROGLYPH_BINARY_VERSION=$HIEROGLYPH_MAJOR_VERSION.0
HIEROGLYPH_DEVICEDIR="${libdir}/hieroglyph/${HIEROGLYPH_BINARY_VERSION}/devices"
HIEROGLYPH_PLUGINDIR="${libdir}/hieroglyph/${HIEROGLYPH_BINARY_VERSION}/plugins"
HIEROGLYPH_LIBDIR="${datadir}/hieroglyph/${HIEROGLYPH_BINARY_VERSION}/lib"
AC_SUBST(HIEROGLYPH_MAJOR_VERSION)
AC_SUBST(HIEROGLYPH_MINOR_VERSION)
AC_SUBST(HIEROGLYPH_RELEASE_VERSION)
AC_SUBST(HIEROGLYPH_BINARY_VERSION)
AC_SUBST(HIEROGLYPH_DEVICEDIR)
AC_SUBST(HIEROGLYPH_PLUGINDIR)
AC_SUBST(HIEROGLYPH_LIBDIR)

dnl ======================================================================
dnl output
dnl ======================================================================
AC_CONFIG_FILES([
Makefile
devices/Makefile
devices/cairo/Makefile
devices/x11-embedded/Makefile
examples/Makefile
hieroglyph/Makefile
hieroglyph/version.h
lib/Makefile
plugins/Makefile
plugins/debug/Makefile
plugins/libedit/Makefile
plugins/test/Makefile
po/Makefile.in
src/Makefile
tests/Makefile
tests/ps/Makefile
hieroglyph.pc
])
AC_OUTPUT

dnl ======================================================================
dnl result
dnl ======================================================================
echo ""
echo "========== Build Information =========="
echo " Enable the debugging build: $enable_debug"
echo " Enable autogeneration:      $enable_rebuilds"
echo " Binary version:             $HIEROGLYPH_BINARY_VERSION"
echo " Device support:             $devices"
echo " Device install path:        $HIEROGLYPH_DEVICEDIR"
echo " Plugin install path:        $HIEROGLYPH_PLUGINDIR"
echo -n " Use CMap:                   $with_cmap "
if test "x$with_cmap" = xyes; then
	echo "($HIEROGLYPH_CMAPDIR)"
else
	echo
fi
echo " Plugins list:               $pluginslist"
echo " CFLAGS:                     $HG_CFLAGS"
echo "   for modules:              $HG_MODULE_CFLAGS"
if test "x$with_cairo" = xyes; then
echo "   for cairo device:         $CAIRO_CFLAGS"
fi
if test "x$with_x11" = xyes; then
echo "   for x11 embedded device:  $X11_CFLAGS"
fi
echo " LIBS:                       $HG_LIBS"
echo "   for modules:              $HG_MODULE_LIBS"
if test "x$with_cairo" = xyes; then
echo "   for cairo device:         $CAIRO_LIBS"
fi
if test "x$with_x11" = xyes; then
echo "   for x11 embedded device:  $X11_LIBS"
fi
echo ""
