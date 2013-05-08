/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * extent_map.c
 *
 * In-memory extent map for the OCFS2 userspace library.
 * (Copied and modified from ocfs2-toos/libocfs2/extent_map.c)
 *
 * Copyright (C) 2004 Oracle.  All rights reserved.
 * CMFS modification by Coly Li <i@coly.li>
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
 */

#define _XOPEN_SOURCE 600 /* Triggers magic in features.h */
#define _LARGEFILE64_SOURCE

#include <cmfs/cmfs.h>

errcode_t cmfs_extent_map_get_blocks(cmfs_cached_inode *cinode,
				     uint64_t v_blkno,
				     int count,
				     uint64_t *p_blkno,
				     uint64_t *ret_count,
				     uint16_t *extent_flags)
{
	return -1;
}

