/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * chain.c
 *
 * Iterate over allocation chains.  Part of the OCFS2 userspace library.
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
 */

#define _XOPEN_SOURCE 600  /* Triggers XOPEN2K in features.h */
#define _LARGEFILE64_SOURCE

#include <string.h>

#include <cmfs/byteorder.h>
#include <cmfs/cmfs.h>
#include <cmfs-kernel/cmfs_fs.h>
#include "cmfs_err.h"

static void cmfs_swap_group_desc_header(struct cmfs_group_desc *gd)
{
	gd->bg_size = bswap_16(gd->bg_size);
	gd->bg_bits = bswap_16(gd->bg_bits);
	gd->bg_free_bits_count = bswap_16(gd->bg_free_bits_count);
	gd->bg_chain = bswap_16(gd->bg_chain);
	gd->bg_generation = bswap_32(gd->bg_generation);
	gd->bg_next_group = bswap_64(gd->bg_next_group);
	gd->bg_parent_dinode = bswap_64(gd->bg_parent_dinode);
	gd->bg_blkno = bswap_64(gd->bg_blkno);
}

/* XXX: why only swap header ? */
void cmfs_swap_group_desc_from_cpu(cmfs_filesys *fs,
				   struct cmfs_group_desc *gd)
{
	if (cpu_is_little_endian)
		return;

	cmfs_swap_group_desc_header(gd);
}

void cmfs_swap_group_desc_to_cpu(cmfs_filesys *fs,
				 struct cmfs_group_desc *gd)
{
	if (cpu_is_little_endian)
		return;

	cmfs_swap_group_desc_header(gd);
}

errcode_t cmfs_read_group_desc(cmfs_filesys *fs,
			       uint64_t blkno,
			       char *gd_buf)
{
	errcode_t ret;
	char *blk;
	struct cmfs_group_desc *gd;

	if ((blkno < CMFS_SUPER_BLOCK_BLKNO) ||
	    (blkno > fs->fs_blocks))
		return CMFS_ET_BAD_BLKNO;

	ret = cmfs_malloc_block(fs->fs_io, &blk);
	if (ret)
		return ret;

	ret = cmfs_read_blocks(fs, blkno, 1, blk);
	if (ret)
		goto out;

	gd = (struct cmfs_group_desc *)blk;
	ret = CMFS_ET_BAD_GROUP_DESC_MAGIC;
	if (memcpy(gd->bg_signature, CMFS_GROUP_DESC_SIGNATURE,
		   strlen(CMFS_GROUP_DESC_SIGNATURE)))
		goto out;

	memcpy(gd_buf, blk, fs->fs_blocksize);

	gd = (struct cmfs_group_desc *)gd_buf;
	cmfs_swap_group_desc_to_cpu(fs, gd);

	ret = 0;
out:
	cmfs_free(&blk);
	return ret;
}
