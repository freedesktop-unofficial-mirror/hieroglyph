#! /bin/sh

rootdir=`dirname $0`/../..
export HIEROGLYPH_LIB_PATH=$rootdir/plugins/test
for i in `dirname $0`/test-*.ps; do
    $rootdir/tests/run.sh $rootdir/examples/pse -l test $i
done