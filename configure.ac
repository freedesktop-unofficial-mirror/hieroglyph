dnl Process this file with autoconf to produce a configure script.
AC_INIT([hieroglyph], 0.0.1, [https://bugs.freedesktop.org/enter_bug.cgi?product=Hieroglyph])

. ./requires

AM_INIT_AUTOMAKE($PACKAGE_NAME, $PACKAGE_VERSION)
AM_MAINTAINER_MODE
AM_CONFIG_HEADER([config.h])

AC_PROG_LIBTOOL
AC_PROG_INTLTOOL
AC_PROG_LEX
AC_PROG_YACC

AC_PROG_CC

GNOME_COMPILE_WARNINGS

GNOME_COMMON_INIT
GNOME_DEBUG_CHECK
GNOME_MAINTAINER_MODE_DEFINES

AC_SYS_LARGEFILE

CFLAGS="$CFLAGS $WARN_CFLAGS"

dnl ======================================================================
dnl Libraries versioning
dnl ======================================================================
dnl Quote from Updating library version information at libtool.info
dnl and renumbering
dnl
dnl 1. Update the version information only immediately before a public
dnl    release of your software.  More frequent updates are unnecessary,
dnl    and only guarantee that the current interface number gets larger
dnl    faster.
dnl 2. If the library source code has changed at all since the last
dnl    update, then increment REVISION (`C:R:A' becomes `C:r+1:A')
dnl 3. If any interfaces have been added, removed, or changed since the
dnl    last update, increment CURRENT, and set REVISION to 0.
dnl 4. If any interfaces have been added since the last public release,
dnl    then increment AGE.
dnl 5. If any interfaces have been removed since the last public release,
dnl    then set AGE to 0.
dnl
LT_CURRENT=0
LT_REVISION=0
LT_AGE=0

AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

dnl ======================================================================
dnl define variables
dnl ======================================================================
pluginslist=""

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
dnl testing functionalities
dnl ======================================================================
dnl ---alignment---
AC_CHECK_ALIGNOF(char)
AC_CHECK_ALIGNOF(short)
AC_CHECK_ALIGNOF(int)
AC_CHECK_ALIGNOF(long)
AC_CHECK_ALIGNOF(void *)
AC_CHECK_ALIGNOF(long long)

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

AM_GLIB_GNU_GETTEXT

dnl ======================================================================
dnl check pkg-config stuff
dnl ======================================================================
PKG_CHECK_MODULES(CHECK, check >= $CHECK_REQUIRED,
	[use_check="yes"],
	[use_check="no"])

AC_SUBST(CHECK_CFLAGS)
AC_SUBST(CHECK_LIBS)

AM_CONDITIONAL(ENABLE_UNIT_TEST, test x$use_check != xno)

dnl ======================================================================
dnl check another libraries
dnl ======================================================================
AM_PATH_GLIB_2_0($GLIB_REQUIRED, :,
	AC_MSG_ERROR([
*** GLIB $GLIB_REQUIRED or better is required. The latest version of
*** GLIB is always available from ftp://ftp.gtk.org/.]),
	glib)

dnl ======================================================================
dnl options
dnl ======================================================================

dnl ======================================================================
dnl output
dnl ======================================================================
AC_CONFIG_FILES([
	Makefile
	hieroglyph/Makefile
	hieroglyph/hgversion.h
	hieroglyph.pc
	hieroglyph-uninstalled.pc
	po/Makefile.in
	tests/Makefile
])
AC_OUTPUT

dnl ======================================================================
dnl result
dnl ======================================================================
echo ""
echo "========== Build Information =========="
echo " CFLAGS:			$CFLAGS $GLIB_CFLAGS"
echo " LDFLAGS:			$LDFLAGS $GLIB_LIBS"
echo ""
echo " Enable the debugging build:	$enable_debug"
echo " Unit testing:			$use_check"
echo ""
