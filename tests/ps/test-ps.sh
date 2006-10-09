#! /bin/sh

set -e

rootdir=`dirname $0`/../..
pushd $rootdir
export HIEROGLYPH_LIB_PATH=./plugins/test
for i in ./tests/ps/test-*.ps; do
    echo `basename $i`:
    ./tests/run.sh ./src/hgs -d QUIET -l test $i
done

popd
