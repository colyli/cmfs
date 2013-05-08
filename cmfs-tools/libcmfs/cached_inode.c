/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * cached_inode.c
 *
 * Cache inode structure for the CMFS userspace library.
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

#include <cmfs/cmfs.h>
#include "cmfs_err.h"

errcode_t cmfs_read_cached_inode(cmfs_filesys *fs,
				 uint64_t blkno,
				 cmfs_cached_inode **ret_ci)
{
	errcode_t ret;
	char *blk;
	cmfs_cached_inode *cinode;

	if ((blkno < CMFS_SUPER_BLOCK_BLKNO) ||
	    (blkno > fs->fs_blocks))
		return CMFS_ET_BAD_BLKNO;

	ret = cmfs_malloc0(sizeof(cmfs_cached_inode), &cinode);
	if (ret)
		return ret;

	cinode->ci_fs = fs;
	cinode->ci_blkno = blkno;

	ret = cmfs_malloc_block(fs->fs_io, &blk);
	if (ret)
		goto cleanup;

	cinode->ci_inode = (struct cmfs_dinode *)blk;
	ret = cmfs_read_inode(fs, blkno, blk);
	if (ret)
		goto cleanup;

	*ret_ci = cinode;

	return 0;

cleanup:
	cmfs_free_cached_inode(fs, cinode);
	return ret;

}

errcode_t cmfs_free_cached_inode(cmfs_filesys *fs,
				 cmfs_cached_inode *cinode)
{
	if (!cinode)
		return CMFS_ET_INVALID_ARGUMENT;

	if (cinode->ci_chains)
		cmfs_bitmap_free(cinode->ci_chains);

	if (cinode->ci_inode)
		cmfs_free(&cinode->ci_inode);

	cmfs_free(&cinode);

	return 0;
}
