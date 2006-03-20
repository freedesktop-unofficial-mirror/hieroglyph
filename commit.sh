#! /bin/sh
TOPDIR=`dirname $0`
VERSIONIN=$TOPDIR/hieroglyph/version.h.in.in
VERSION_H=${VERSIONIN//.in.in}.in

$TOPDIR/update-version
FILES=''
while [ $# -ne 0 ]; do
	FILES="$FILES $1"
	shift
done
if [ "x$FILES" != "x" ]; then
	FILES="$FILES ChangeLog $VERSION_H"
fi

LANG=C svn ci -m "`svn diff | awk 'BEGIN{P=0;F=0}$0 ~ /^+++ ChangeLog/{F=1}F == 1 && $0 ~ /^@@/{P=1}P == 1 && $0 ~ /^+/{print $0}P == 1 && /^Index/{P=0;F=0}' | sed -e 's/^+//'`" $FILES
