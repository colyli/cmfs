bin_PROGRAMS = debugfs.cmfs
debugfs_cmfs_SOURCES = main.c commands.c
debugfs_cmfs_CFLAGS = `pkg-config --libs --cflags gtk+-2.0` -DVERSION=\"$(VERSION)\" -Wall -Werror
debugfs_cmfs_LDADD = ../libcmfs/libcmfs.a
debugfs_cmfs_LDFLAGS = -lcom_err -luuid -laio
