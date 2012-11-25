/*
 * getsectsize.c --- get the sector size of a device.
 * (copied and modified from libocfs2/getsectsize.c)
 *
 * Copyright (C) 1995, 1995 Theodore Ts'o
 * Copyright (C) 2003 VMWare, Inc.
 *
 * This file may be redistributed undert the terms of GNU
 * Public License.
 *
 * Modified for OCFS2 by Manish Singh <manish.singh@oracle.com>
 * Modified for CMFS by Coly Li <i@coly.li>
 */

#ifndef _XOPEN_SOURCE
# define _XOPEN_SOURCE 600
#endif
#ifndef _LARGEFILE64_SOURCE
# define _LARGEFILE64_SOURCE
#endif
/* Returns the number of blocks in a partition */
#include <et/com_err.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <unistd.h>

#include "cmfs_err.h"

errcode_t cmfs_get_device_sectsize(const char *file, int *sectsize)
{
	int fd;
	int ret;

	fd = open64(file, O_RDONLY);
	if (fd < 0) {
		if (errno == ENOENT)
			return CMFS_ET_NAMED_DEVICE_NOT_FOUND;
		else
			return CMFS_ET_IO;
	}

	ret = CMFS_ET_CANNOT_DETERMINE_SECTOR_SIZE;
	if (ioctl(fd, BLKSSZGET, sectsize) >= 0)
		ret = 0;

	close(fd);
	return ret;
}
