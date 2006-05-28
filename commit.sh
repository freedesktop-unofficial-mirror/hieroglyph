#! /bin/sh
DO=""
TOPDIR=`dirname $0`
VERSIONIN=$TOPDIR/hieroglyph/version.h.in.in
VERSION_H=${VERSIONIN//.in.in}.in

which svn > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Cannot find svn command. stopped."
    exit
fi
which awk > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Cannot find awk command. stopped."
    exit
fi
which sed > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Cannot find sed command. stopped."
    exit
fi


if [ "x`svn stat`" = "x" ]; then
    echo "No changes has been made. any changes has never committed."
    exit
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
if [ "x$LOG" = "x" ]; then
    while [ 1 ]; do
	echo -n "Are you sure that you want to commit file(s) without commit log? [y/N]: "
	read DOCOMMIT
	if [ "x$DOCOMMIT" = "x" -o "x$DOCOMMIT" = "xn" -o "x$DOCOMMIT" = "xN" ]; then
	    echo "Abort."
	    exit
	elif [ "x$DOCOMMIT" = "xy" -o "x$DOCOMMIT" = "xY" ]; then
	    break
	else
	    echo "Answer must be y or n."
	fi
    done
fi

if [ "x$DO" = "x" ]; then
    $TOPDIR/update-version
fi

LANG=C $DO svn ci -m "$LOG" $FILES
