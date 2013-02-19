/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * dir_iterate.c
 *
 * Directory block routines for the OCFS2 userspace library.
 * (Copied and modified from ocfs2-tools/libocfs2/dir_iterate.c)
 *
 * Copyright (C) 2004 Oracle.  All rights reserved.
 * CMFS modification, by Coly Li <i@coly.li>
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
 *  This code is a port of e2fsprogs/lib/ext2fs/dir_iterate.c
 *  Copyright (C) 1993, 1994, 1994, 1995, 1996, 1997 Theodore Ts'o.
 */

#define _XOPEN_SOURCE 600 /* Triggers magic in features.h */
#define _LARGEFILE64_SOURCE

#include <inttypes.h>

#include "cmfs/cmfs.h"

errcode_t cmfs_dir_iterate(cmfs_filesys *fs,
			   uint64_t dir,
			   int flags,
			   char *block_buf,
			   int (*func)(struct cmfs_dir_entry *dirent,
				       uint64_t blocknr,
				       int offset,
				       int blocksize,
				       char *buf,
				       void *prov_data),
			   void *priv_data)
{
	return -1;
}

