.PHONY: cmfs_err.c
cmfs_err.c: cmfs_err.et
	compile_et cmfs_err.et

noinst_LIBRARIES = libcmfs.a
libcmfs_a_SOURCES = cmfs_err.c dirblock.c getsectsize.c getsize.c unix_io.c
