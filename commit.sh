#! /bin/sh
DO=""
TOPDIR=`dirname $0`
VERSIONIN=$TOPDIR/hieroglyph/version.h.in.in
VERSION_H=${VERSIONIN//.in.in}.in

if [ "x`svn stat`" = "x" ]; then
    echo "No changes has been made. any changes has never committed."
    exit
fi
if [ "x$DO" = "x" ]; then
    $TOPDIR/update-version
fi
LOG=''
if test "$1" = "--message" || test "$1" = "-m"; then
    shift
    LOG="$1"
    shift
fi
FILES=''
while [ $# -ne 0 ]; do
    FILES="$FILES $1"
    shift
done
if [ "x$FILES" != "x" ]; then
    FILES="$FILES ChangeLog $VERSION_H"
fi
if [ "x$LOG" = "x" ]; then
    LOG="`svn diff | awk 'BEGIN{P=0;F=0}$0 ~ /^+++ ChangeLog/{F=1}F == 1 && $0 ~ /^@@/{P=1}P == 1 && $0 ~ /^+/{print $0}P == 1 && /^Index/{P=0;F=0}' | sed -e 's/^+//'`"
fi

LANG=C $DO svn ci -m "\"$LOG\"" $FILES
