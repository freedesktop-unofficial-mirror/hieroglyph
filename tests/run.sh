#! /bin/sh

export HIEROGLYPH_DEVICE_PATH=
for i in `dirname $0`/../devices/*; do
	if test -d $i && test -d $i/.libs; then
		export HIEROGLYPH_DEVICE_PATH=$HIEROGLYPH_DEVICE_PATH:$i/.libs
	fi
done
export HIEROGLYPH_PLUGIN_PATH=
for i in `dirname $0`/../plugins/*; do
	if test -d $i && test -d $i/.libs; then
		export HIEROGLYPH_PLUGIN_PATH=$HIEROGLYPH_PLUGIN_PATH:$i/.libs
	fi
done
if [ "x$HIEROGLYPH_LIB_PATH" != "x" ]; then
	export HIEROGLYPH_LIB_PATH=$HIEROGLYPH_LIB_PATH:`dirname $0`/../lib:.
else
	export HIEROGLYPH_LIB_PATH=`dirname $0`/../lib:.
fi
export LD_LIBRARY_PATH="`dirname $0`/../hieroglyph/.libs:`dirname $0`/../libretto/.libs:$LD_LIBRARY_PATH"
$@
