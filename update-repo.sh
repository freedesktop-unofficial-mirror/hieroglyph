#! /bin/sh
DO=""
TOPDIR=`dirname $0`
VERSIONIN=$TOPDIR/hieroglyph/hgversion.h.in.in
VERSION_H=`echo $VERSIONIN | sed -e 's/.in.in$/.in/'`

function DO() {
	_param="$@"
	echo $_param
	if [ "x$DO" = "x" ]; then
		if [ "x$(echo $_param|cut -d' ' -f1)" = "xrm" ]; then
			$DO $_param > /dev/null 2>&1
		else
			$DO $_param
		fi
	fi
}

which git > /dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "Unable to find git command. stopped."
	exit
fi

if [ "x$1" != "xNOPULL" ]; then
DO rm $VERSION_H
LANG=C DO git pull
else
  echo no pull
fi
DO rm $VERSION_H
LANG=C DO git checkout $VERSION_H
