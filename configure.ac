dnl Process this file with autoconf to produce a configure script.
AC_INIT([hieroglyph], 0.0.1, [https://bugs.freedesktop.org/enter_bug.cgi?product=Hieroglyph])

. ./requires

# Support silent build rules, requires at least automake-1.11. Enable
# by either passing --enable-silent-rules to configure or passing V=0
# to make
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AM_INIT_AUTOMAKE($PACKAGE_NAME, $PACKAGE_VERSION)
AM_MAINTAINER_MODE
AM_CONFIG_HEADER([config.h])

AC_PROG_LIBTOOL
AC_PROG_INTLTOOL
AC_PROG_LEX

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX

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

dnl check for TLS
AC_MSG_CHECKING(for _Thread_local)
_save_cflags=$CFLAGS
CFLAGS=$(sed -e 's/\-Werror//g')
AC_TRY_COMPILE([],[
int a(void);
int b(void);
a();
return 0;
}

_Thread_local int i = 0;
int a(void) {return i;}
int b(void){
],_Thread_local=yes,_Thread_local=no)
CFLAGS=$_save_cflags
AC_MSG_RESULT($_Thread_local)

AC_MSG_CHECKING(for thread_local)
_save_cflags=$CFLAGS
CFLAGS=$(sed -e 's/\-Werror//g')
AC_TRY_COMPILE([],[
int a(void);
int b(void);
a();
return 0;
}

thread_local int i = 0;
int a(void) {return i;}
int b(void){
],thread_local=yes,thread_local=no)
CFLAGS=$_save_cflags
AC_MSG_RESULT($thread_local)

AC_MSG_CHECKING(for __thread)
_save_cflags=$CFLAGS
CFLAGS=$(sed -e 's/\-Werror//g')
AC_TRY_COMPILE([],[
int a(void);
int b(void);
a();
return 0;
}

__thread int i = 0;
int a(void) {return i;}
int b(void){
],__thread=yes,__thread=no)
CFLAGS=$_save_cflags
AC_MSG_RESULT($__thread)

dnl check for flavours of varargs macros
AC_MSG_CHECKING(for ISO C99 varargs macros in C)
_save_cflags=$CFLAGS
CFLAGS=$(sed -e 's/\-Werror//g')
AC_TRY_COMPILE([],[
int a(int p1, int p2, int p3);
#define call_a(...) a(1,__VA_ARGS__)
call_a(2,3);
],have_iso_c_varargs=yes,have_iso_c_varargs=no)
CFLAGS=$_save_cflags
AC_MSG_RESULT($have_iso_c_varargs)

AC_MSG_CHECKING(for ISO C99 varargs macros in C++)
if test "$CXX" = ""; then
	have_iso_cxx_varargs=no
else
	AC_LANG_CPLUSPLUS
	AC_TRY_COMPILE([],[
int a(int p1, int p2, int p3);
#define call_a(...) a(1,__VA_ARGS__)
call_a(2,3);
],have_iso_cxx_varargs=yes,have_iso_cxx_varargs=no)
	AC_LANG_C
fi
AC_MSG_RESULT($have_iso_cxx_varargs)

AC_MSG_CHECKING(for GNUC varargs macros)
_save_cflags=$CFLAGS
CFLAGS=$(sed -e 's/\-Werror//g')
AC_TRY_COMPILE([],[
int a(int p1, int p2, int p3);
#define call_a(params...) a(1,params)
call_a(2,3);
],have_gnuc_varargs=yes,have_gnuc_varargs=no)
CFLAGS=$_save_cflags
AC_MSG_RESULT($have_gnuc_varargs)

dnl inline
AC_CACHE_CHECK([for __inline],hg_cv_has__inline,[
	AC_COMPILE_IFELSE([
	__inline int foo(void);
	__inline int foo() {return 0;}
	int main() {return foo();}
	],
	hg_cv_has__inline=yes,
	hg_cv_has__inline=no,)
])
case x$hg_cv_has__inline in
xyes)
	AC_DEFINE(HG_HAVE___INLINE,1,[Have __inline keyword])
esac
AC_CACHE_CHECK([for __inline__],hg_cv_has__inline__,[
	AC_COMPILE_IFELSE([
	__inline__ int foo(void);
	__inline__ int foo() {return 0;}
	int main() {return foo();}
	],
	hg_cv_has__inline__=yes,
	hg_cv_has__inline__=no,)
])
case x$hg_cv_has__inline__ in
xyes)
	AC_DEFINE(HG_HAVE___INLINE__,1,[Have __inline__ keyword])
esac
AC_CACHE_CHECK([for inline],hg_cv_hasinline,[
	AC_COMPILE_IFELSE([
	#undef inline
	inline int foo(void);
	inline int foo() {return 0;}
	int main() {return foo();}
	],
	hg_cv_hasinline=yes,
	hg_cv_hasinline=no,)
])
case x$hg_cv_hasinline in
xyes)
	AC_DEFINE(HG_HAVE_INLINE,1,[Have inline keyword])
esac
AC_MSG_CHECKING(if inline functions in headers work)
AC_LINK_IFELSE([
#if defined (HG_HAVE_INLINE) && defined (__GNUC__) && defined (__STRICT_ANSI__)
#undef inline
#define inline __inline__
#elif !defined (HG_HAVE_INLINE)
#undef inline
#if defined (HG_HAVE___INLINE__)
#define inline __inline__
#elif defined (HG_HAVE___INLINE)
#define inline __inline
#endif
#endif

int hg_test_func2(int);

static inline int
hg_test_func1(void) {
  return hg_test_func2(1);
}
int main(void) {int i = 1; return --i;}],[hg_can_inline=yes],[hg_can_inline=no])
AC_MSG_RESULT($hg_can_inline)

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
PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES(GMODULE, gmodule-2.0,
	[use_plugins="yes"],
	[use_plugins="no"])
PKG_CHECK_MODULES(CHECK, check >= $CHECK_REQUIRED,
	[use_check="yes"
	 plugins="${plugins}unittest "],
	[use_check="no"])
PKG_CHECK_MODULES(LIBEDIT, libedit,
	[use_libedit="yes"],
	AC_CHECK_LIB(edit, readline, [
	LIBEDIT_LIBS="-ledit"
	use_libedit="yes"],
	[use_libedit="no"]))

AC_SUBST(GMOUDLE_CFLAGS)
AC_SUBST(GMODULE_LIBS)
AC_SUBST(CHECK_CFLAGS)
AC_SUBST(CHECK_LIBS)
AC_SUBST(LIBEDIT_CFLAGS)
AC_SUBST(LIBEDIT_LIBS)

AM_CONDITIONAL(ENABLE_PLUGINS, test x$use_plugins != xno)
AM_CONDITIONAL(ENABLE_UNIT_TEST, test x$use_check != xno)
AM_CONDITIONAL(ENABLE_LIBEDIT, test x$use_libedit != xno)

if test x$use_libedit != xno; then
	AC_DEFINE(USE_LIBEDIT,,[enable libedit])
	plugins="${plugins}libedit "
fi

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
AC_ARG_ENABLE(profile,
	AC_HELP_STRING([--enable-profile], [build with the profiler option enabled.])
	,,
	enable_profile=no)
AC_ARG_WITH(cairo,
	AC_HELP_STRING([--without-cairo], [build without the cairo device.])
	,,
	with_cairo=yes)

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
dnl options - cairo
dnl ======================================================================
if test "x$with_cairo" = xyes; then
	PKG_CHECK_MODULES(CAIRO, cairo >= $CAIRO_REQUIRED)
	AC_SUBST(CAIRO_CFLAGS)
	AC_SUBST(CAIRO_LIBS)
	devices="${devices}cairo "
fi
AM_CONDITIONAL(ENABLE_CAIRO, test x$with_cairo != xno)

dnl ======================================================================
dnl output
dnl ======================================================================
AC_CONFIG_COMMANDS([hieroglyph/hgconfig.h],
[
	outfile=hieroglyph/hgconfig.h-tmp
	cat > $outfile <<\_______EOF
/*
 * hgconfig.h
 *
 * This is a genereated file.  Please modify 'configure.ac'
 */
#ifndef __HIEROGLYPH_HGCONFIG_H__
#define __HIEROGLYPH_HGCONFIG_H__

#include <hieroglyph/hgmacros.h>

_______EOF

	cat >> $outfile <<_______EOF

HG_BEGIN_DECLS

$thread_var_t

#ifdef __cplusplus
#define HG_HAVE_INLINE		1
#else /* !__cplusplus */
$hg_inline
#endif /* !__cplusplus */

#ifdef __cplusplus
#define HG_CAN_INLINE		1
_______EOF

	if test x$hg_can_inline = xyes; then
		cat >>$outfile <<_______EOF
#else /* !__cplusplus */
#define HG_CAN_INLINE		1
_______EOF
	fi

	cat >> $outfile <<_______EOF
#endif

_______EOF

	if test x$hg_have_iso_c_varargs = xyes; then
		cat >>$outfile <<_______EOF
#ifndef __cplusplus
#  define HG_HAVE_ISO_VARARGS	1
#endif
_______EOF
	fi
	if test x$hg_have_iso_cxx_varargs = xyes; then
		cat >>$outfile <<_______EOF
#ifdef __cplusplus
#  define HG_HAVE_ISO_VARARGS	1
#endif
_______EOF
	fi
	if test x$hg_have_gnuc_varargs = xyes; then
		cat >>$outfile <<_______EOF

/* gcc-2.95.x supports both gnu style and ISO varargs, but if -ansi
 * is passed ISO vararg support is turned off, and there is no work
 * around to turn it on, so we unconditionally turn it off.
 */
#if __GNUC__ == 2 && __GNUC_MINOR__ == 95
#  undef HG_HAVE_ISO_VARARGS
#endif

#define HG_HAVE_GNUC_VARARGS	1
_______EOF
	fi

	cat >>$outfile <<_______EOF

HG_END_DECLS

#endif /* __HIEROGLYPH_HGCONFIG_H__ */
_______EOF

	if cmp -s $outfile hieroglyph/hgconfig.h; then
		AC_MSG_NOTICE([hieroglyph/hgconfig.h is unchanged])
		rm -f $outfile
	else
		mv $outfile hieroglyph/hgconfig.h
	fi
],[
if test x$_Thread_local = xyes; then
	thread_var_t='#define HG_THREAD_VAR	_Thread_local'
elif test x$thread_local = xyes; then
	thread_var_t='#define HG_THREAD_VAR	thread_local'
elif test x$__thread = xyes; then
	thread_var_t='#define HG_THREAD_VAR	__thread'
else
	thread_var_t='#define HG_THREAD_VAR'
fi
hg_have_gnuc_varargs=$have_gnuc_varargs
hg_have_iso_c_varargs=$have_iso_c_varargs
hg_have_iso_cxx_varargs=$have_iso_cxx_varargs
if test x$hg_cv_hasinline = xyes; then
	hg_inline='#define HG_HAVE_INLINE		1'
fi
if test x$hg_cv_has__inline = xyes; then
	hg_inline="\$hg_inline
#define HG_HAVE___INLINE	1"
fi
if test x$hg_cv_has__inline__ = xyes; then
	hg_inline="\$hg_inline
#define HG_HAVE___INLINE__	1"
fi
hg_can_inline=$hg_can_inline
])
AC_CONFIG_FILES([
	Makefile
	build_tools/Makefile
	devices/Makefile
	devices/cairo/Makefile
	hieroglyph/Makefile
	hieroglyph/hgversion.h
	hieroglyph.pc
	hieroglyph-uninstalled.pc
	lib/Makefile
	plugins/Makefile
	plugins/libedit/Makefile
	plugins/unittest/Makefile
	po/Makefile.in
	src/Makefile
	src/hgs/Makefile
	tests/Makefile
	tests/ps/Makefile
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
echo " Binary version:		$HIEROGLYPH_BINARY_VERSION"
echo " Enable the debugging build:	$enable_debug"
echo " Enable the profile:		$enable_profile"
echo " Unit testing:			$use_check"
echo " Enable plugins:		$use_plugins"
echo ""
echo "========== Path Information =========="
echo " Library path:			$HIEROGLYPH_LIBDIR"
echo " Device path:			$HIEROGLYPH_DEVICEDIR"
echo " Plugin path:			$HIEROGLYPH_PLUGINDIR"
echo ""
echo "========== Device Information =========="
echo " Devices:			$devices"
echo ""
echo "========== Plugin Information =========="
echo " Plugins:			$plugins"
echo ""
