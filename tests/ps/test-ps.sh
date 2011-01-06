#! /bin/sh
# test-ps.sh
# Copyright (C) 2006-2010 Akira TAGOH
#
# Authors:
#   Akira TAGOH  <akira@tagoh.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.

set -e

function run_test() {
    echo `basename $1`:
    ./tests/run.sh $args ./src/hgs/hgs -d QUIET -l unittest $1
}

rootdir=`dirname $0`/../..
pushd $rootdir

if [ "x$1" = "x-d" ]; then
    shift
    args="-d"
fi
if [ $# -ne 0 ]; then
    run_test $1
else
    for i in ./tests/ps/test-*.ps; do
	run_test $i
    done
fi

popd
