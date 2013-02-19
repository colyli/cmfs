/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * lookup.c
 *
 * Directory lookup routines for the CMFS userspace library.
 * (Copied and modified from ocfs2-tools/libocfs2/lookup.c)
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
 *  This code is a port of e2fsprogs/lib/ext2fs/lookup.c
 *  Copyright (C) 1993, 1994, 1994, 1995 Theodore Ts'o.
 */

#define _XOPEN_SOURCE 600 /* Triggers magic in features.h */
#define _LARGEFILE64_SOURCE

#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include "cmfs/cmfs.h"


struct lookup_struct  {
	const char	*name;
	int		len;
	uint64_t	*inode;
	int		found;
};	

errcode_t cmfs_lookup(cmfs_filesys *fs,
		      uint64_t dir,
		      const char *name,
		      int namelen,
		      char *buf,
		      uint64_t *inode)
{
	return -1;
}
