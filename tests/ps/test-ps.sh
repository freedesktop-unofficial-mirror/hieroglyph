#! /bin/sh

set -e

rootdir=`dirname $0`/../..
export HIEROGLYPH_LIB_PATH=$rootdir/plugins/test
for i in `dirname $0`/test-*.ps; do
    echo $i
    $rootdir/tests/run.sh $rootdir/src/hgs -l test $i
done
