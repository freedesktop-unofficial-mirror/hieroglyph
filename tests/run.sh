#! /bin/sh

export HIEROGLYPH_DEVICE_PATH=`dirname $0`/../devices/cairo/.libs
export HIEROGLYPH_FILTER_PATH=`dirname $0`/../filters/ps/.libs
export HIEROGLYPH_LIB_PATH=`dirname $0`/../lib:.
export LD_LIBRARY_PATH="`dirname $0`/../hieroglyph/.libs:`dirname $0`/../libretto/.libs:$LD_LIBRARY_PATH"
$@
