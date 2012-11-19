bin_PROGRAMS = mkfs.cmfs
mkfs_cmfs_SOURCES = mkfs.c check.c
mkfs_cmfs_CFLAGS = -DVERSION=$(VERSION)
mkfs_cmfs_LDADD = ../libcmfs/libcmfs.a
mkfs_cmfs_LDFLAGS = -lcom_err -luuid
