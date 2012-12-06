#!/bin/bash

who=INSTALL
who="$who Makefile Makefile.in aclocal.m4 autom4te.cache/ config.log config.status configure missing"
who="$who include/stamp-h1 install-sh"
who="$who libcmfs/.deps/ libcmfs/Makefile libcmfs/Makefile.in libcmfs/*.o libcmfs/libcmfs.a libcmfs/cmfs_err.c libcmfs/cmfs_err.h"
who="$who mkfs.cmfs/.deps/ mkfs.cmfs/Makefile mkfs.cmfs/Makefile.in mkfs.cmfs/*.o mkfs.cmfs/mkfs.cmfs"
who="$who misc/.deps misc/Makefile misc/Makefile.in misc/member_offset misc/member_offset.o"
who="$who dumpcmfs/*.o dumpcmfs/Makefile dumpcmfs/Makefile.in dumpcmfs/.deps/"
who="$who libtools-internal/libtools-internal.a libtools-internal/*.o libtools-internal/Makefile libtools-internal/Makefile.in libtools-internal/.deps"


rm -rf $who
