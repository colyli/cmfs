.PHONY: cmfs_err.c
cmfs_err.c: cmfs_err.et
	compile_et cmfs_err.et

noinst_LIBRARIES = libcmfs.a
libcmfs_a_SOURCES = cmfs_err.c dirblock.c getsectsize.c getsize.c kernel-rbtree.c unix_io.c bitops.c ismounted.c openfs.c closefs.c freefs.c memory.c inode.c blockcheck.c extents.c chain.c feature_string.c lookup.c dir_iterate.c cached_inode.c fileio.c
libcmfs_a_CFLAGS = -Wall -Werror

