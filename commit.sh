#! /bin/sh
DO=""
TOPDIR=`dirname $0`
VERSION_H=$TOPDIR/hieroglyph/hgversion.h.in

function DO() {
	_cmd="$1"
	shift
	echo $_cmd "$@"
	if [ "x$DO" = "x" ]; then
		if [ "x$_cmd" = "xrm" ]; then
			$DO $_cmd "$@" > /dev/null 2>&1 || exit
		else
			$DO $_cmd "$@" || exit
		fi
	fi
}

which git > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Cannot find git command. stopped."
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


if [ "x`git status | grep -E '(new|modified)'`" = "x" ]; then
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
    FILES="$FILES ChangeLog"
else
    GIT_OPTS="-a"
fi
if [ "x$LOG" = "x" ]; then
    LOG="`git diff | awk 'BEGIN{P=0;F=0}$0 ~ /^+++ b\/ChangeLog/{F=1}F == 1 && $0 ~ /^@@/{P=1}P == 1 && $0 ~ /^+/{print $0}P == 1 && /^index/{P=0;F=0}' | sed -e 's/^+//' | grep -v -E \"^[0-9]+\-[0-9]+\-[0-9]+ \"`"
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

LANG=C DO git commit $GIT_OPTS -m "$LOG" $FILES

DO rm $VERSION_H
LANG=C DO git checkout $VERSION_H
