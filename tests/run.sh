#! /bin/sh
# run.sh
# Copyright (C) 2010 Akira TAGOH
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

if [ "x$HIEROGLYPH_LIB_PATH" != "x" ]; then
	export HIEROGLYPH_LIB_PATH=$HIEROGLYPH_LIB_PATH:`dirname $0`/../lib:.
else
	export HIEROGLYPH_LIB_PATH=`dirname $0`/../lib:.
fi
export LD_LIBRARY_PATH="`dirname $0`/../hieroglyph/.libs:$LD_LIBRARY_PATH"
$@