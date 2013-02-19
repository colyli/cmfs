/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * extents.c
 *
 * Iterate over the extents in an inode.  Part of the CMFS userspace
 * library.
 *
 * (Copied and modified from ocfs2-tools/libocfs2/extents.c)
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
 * Ideas taken from e2fsprogs/lib/ext2fs/block.c
 *   Copyright (C) 1993, 1994, 1995, 1996 Theodore Ts'o.
 */

#define _XOPEN_SOURCE 600  /* Triggers XOPEN2K in features.h */
#define _LARGEFILE64_SOURCE

#include <string.h>
#include <inttypes.h>

#include <cmfs/cmfs.h>
#include <cmfs-kernel/cmfs_fs.h>
#include <cmfs/byteorder.h>


static void cmfs_swap_extent_list_primary(struct cmfs_extent_list *el)
{
	el->l_tree_depth	= bswap_16(el->l_tree_depth);
	el->l_count		= bswap_16(el->l_count);
	el->l_next_free_rec	= bswap_16(el->l_next_free_rec);
}

static void cmfs_swap_extent_list_secondary(cmfs_filesys *fs,
					    void *obj,
					    struct cmfs_extent_list *el)
{
	struct cmfs_extent_rec *rec;
	int i;

	for(i = 0; i < el->l_next_free_rec; i++)
	{
		rec = &el->l_recs[i];
		if(cmfs_swap_barrier(fs, obj, rec,
				     sizeof(struct cmfs_extent_rec)))
			break;
		rec->e_cpos = bswap_64(rec->e_cpos);
		rec->e_blkno = bswap_64(rec->e_blkno);
		if (el->l_tree_depth)
			rec->e_int_blocks = bswap_64(rec->e_int_blocks);
		else
			rec->e_leaf_blocks = bswap_32(rec->e_leaf_blocks);
	}
}

void cmfs_swap_extent_list_from_cpu(cmfs_filesys *fs,
				    void *obj,
				    struct cmfs_extent_list *el)
{
	if (cpu_is_little_endian)
		return;

	cmfs_swap_extent_list_secondary(fs, obj, el);
	cmfs_swap_extent_list_primary(el);
}

void cmfs_swap_extent_list_to_cpu(cmfs_filesys *fs,
				  void *obj,
				  struct cmfs_extent_list *el)
{
	if (cpu_is_little_endian)
		return;

	cmfs_swap_extent_list_primary(el);
	cmfs_swap_extent_list_secondary(fs, obj, el);
}

errcode_t cmfs_read_extent_block(cmfs_filesys *fs,
				 uint64_t blkno,
				 char *eb_buf)
{
	return -1;
}

errcode_t cmfs_block_iterate_inode(cmfs_filesys *fs,
				   struct cmfs_dinode *inode,
				   int flags,
				   int (*func)(cmfs_filesys *fs,
					       uint64_t blkno,
					       uint64_t bcount,
					       uint16_t ext_flags,
					       void *priv_data),
				   void *priv_data)
{
	return -1;
}
































































