#! /bin/sh

topdir=`dirname $0`

LD_LIBRARY_PATH=$topdir/.libs:$LD_LIBRARY_PATH LD_PRELOAD=$topdir/.libs/libhgspy-helper.so $topdir/.libs/hgspy-bin $@
