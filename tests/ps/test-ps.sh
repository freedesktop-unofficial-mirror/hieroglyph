#! /bin/sh

set -e

function run_test() {
    echo `basename $1`:
    ./tests/run.sh ./src/hgs -d QUIET -l test $1
}

rootdir=`dirname $0`/../..
pushd $rootdir
export HIEROGLYPH_LIB_PATH=./plugins/test

if [ $# -ne 0 ]; then
    run_test $1
else
    for i in ./tests/ps/test-*.ps; do
	run_test $i
    done
fi

popd
