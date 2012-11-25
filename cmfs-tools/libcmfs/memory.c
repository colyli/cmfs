/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * memory.c
 *
 * Memory routines for the CMFS userspace library.
 * (Copied and modified from ocfs2-tools/libocfs2/memory.c)
 *
 * Copyright (C) 2004 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License, version 2,  as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 *
 * Portions of this code from e2fsprogs/lib/ext2fs/ext2fs.h
 *  Copyright (C) 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001,
 *  	2002 by Theodore Ts'o.
 */


#define _XOPEN_SOURCE 600
#define _LARGEFILE64_SOURCE

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <cmfs/cmfs.h>
#include "cmfs_err.h"


errcode_t cmfs_malloc(unsigned long size, void *ptr)
{
	void **pp = (void **)ptr;

	*pp = malloc(size);
	if (!(*pp))
		return CMFS_ET_NO_MEMORY;
	return 0;
}

errcode_t cmfs_malloc0(unsigned long size, void *ptr)
{
	errcode_t ret;
	void **pp = (void **)ptr;

	ret = cmfs_malloc(size, ptr);
	if (ret)
		return ret;

	memset(*pp, 0, size);
	return 0;
}

errcode_t cmfs_free(void *ptr)
{
	void **pp = (void **)ptr;

	free(*pp);
	*pp = NULL;
	return 0;
}

errcode_t cmfs_malloc_blocks(io_channel *channel,
			     int num_blocks,
			     void *ptr)
{
	errcode_t ret;
	int blksize;
	size_t bytes;
	void **pp = (void **)ptr;

	blksize = io_get_blksize(channel);
	if (((unsigned long long)num_blocks * blksize) > SIZE_MAX)
		return CMFS_ET_NO_MEMORY;

	bytes = num_blocks * blksize;
	ret = posix_memalign(pp, blksize, bytes); 
	if (!ret)
		return 0;
	if (errno == ENOMEM)
		return CMFS_ET_NO_MEMORY;
	abort();
}

errcode_t cmfs_malloc_block(io_channel *channel, void *ptr)
{
	return cmfs_malloc_blocks(channel, 1, ptr);
}
