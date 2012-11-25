/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * openfs.c
 *
 * Open a CMFS filesystem.  Part of the CMFS userspace library.
 * (Copied and modified from ocfs2-tools/libocfs/openfs.c)
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
 * Ideas taken from e2fsprogs/lib/ext2fs/openfs.c
 *   Copyright (C) 1993, 1994, 1995, 1996 Theodore Ts'o.
 */

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <et/com_err.h>

#include <cmfs-kernel/cmfs_fs.h>
#include <cmfs/cmfs.h>
#include "cmfs_err.h"





static errcode_t __cmfs_read_blocks(cmfs_filesys *fs,
				    uint64_t blkno,
				    int count,
				    char *data,
				    int nocache)
{
	errcode_t err;

	if (nocache)
		err = io_read_block_nocache(fs->fs_io, blkno, count, data);
	else
		err = io_read_block(fs->fs_io, blkno, count, data);

	return err;
}

errcode_t cmfs_read_blocks(cmfs_filesys *fs,
			   uint64_t blkno,
			   int count,
			   char *data)
{
	return __cmfs_read_blocks(fs, blkno, count, data, 0);
}

errcode_t cmfs_read_super(cmfs_filesys *fs, uint64_t superblock, char *sb)
{
	errcode_t ret;
	char *blk, *swapblk;
	struct cmfs_dinode *di, *orig_super;
	int orig_blocksize;
	int blocksize = io_get_blksize(fs->fs_io);

	ret = cmfs_malloc_block(fs->fs_io, &blk);
	if (ret)
		return ret;

	ret = cmfs_read_blocks(fs, superblock, 1, blk);
	if (ret)
		goto out_blk;

	di = (struct cmfs_dinode *)blk;

	ret = CMFS_ET_BAD_MAGIC;
	if (memcmp(di->i_signature,
		   CMFS_SUPER_BLOCK_SIGNATURE,
		   strlen(CMFS_SUPER_BLOCK_SIGNATURE)))
		goto out_blk;

	/*
	 * when calling cmfs_validate_meta_ecc() to validate super block,
	 * flags in fs->fs_super hash to be check whether meta ecc is
	 * enabled in the superblock (blk read by cmfs_read_blocks()).
	 * Therefore, before calling cmfs_validate_meta_ecc() to validate
	 * blk, a swapblk copy is needed, which is used to replace
	 * fs->fs_super and fs->fs_blocksize.
	 */
	ret = cmfs_malloc_block(fs->fs_io, &swapblk);
	if (ret)
		goto out_blk;

	memcpy(swapblk, blk, blocksize);
	orig_super = fs->fs_super;
	orig_blocksize = fs->fs_blocksize;
	fs->fs_super = (struct cmfs_dinode *)swapblk;
	fs->fs_blocksize = blocksize;
	cmfs_swap_inode_to_cpu(fs, fs->fs_super);

	ret = cmfs_validate_meta_ecc(fs, blk, &di->i_check);

	fs->fs_super = orig_super;
	fs->fs_blocksize = orig_blocksize;
	cmfs_free(&swapblk);

	if (ret)
		goto out_blk;

	/* meta ecc check OK, now update to fs->fs_super */
	cmfs_swap_inode_to_cpu(fs, di);
	if (!sb)
		fs->fs_super = di;
	else {
		memcpy(sb, blk, fs->fs_blocksize);
		cmfs_free(&blk);
	}

	return 0;
out_blk:
	cmfs_free(&blk);
	return ret;
}

errcode_t cmfs_open(const char *name,
		    int flags,
		    unsigned int superblock,
		    unsigned int block_size,
		    cmfs_filesys **ret_fs)
{
	cmfs_filesys *fs;
	errcode_t ret;
	int i, len;
	char *ptr;
	unsigned char *raw_uuid;

	/* block_size is fixed to 4KB */
	ret = CMFS_ET_UNEXPECTED_BLOCK_SIZE;
	if (block_size != CMFS_MAX_BLOCKSIZE)
		return ret;
	
	ret = cmfs_malloc0(sizeof(cmfs_filesys), &fs);
	if (ret)
		return ret;

	fs->fs_flags = flags;
	fs->fs_umask = 022;

	ret = io_open(name,
		      (flags & (CMFS_FLAG_RO |
				CMFS_FLAG_RW |
				CMFS_FLAG_BUFFERED)),
		      &fs->fs_io);
	if (ret)
		goto out;

	ret = cmfs_malloc(strlen(name) + 1, &fs->fs_devname);
	if (ret)
		goto out;
	strcpy(fs->fs_devname, name);

	/* don't support image file yet */
	if (io_is_device_readonly(fs->fs_io))
			fs->fs_flags |= CMFS_FLAG_HARD_RO;

	if (!superblock)
		superblock = CMFS_SUPER_BLOCK_BLKNO;
	io_set_blksize(fs->fs_io, block_size);

	ret = cmfs_read_super(fs, (uint64_t)superblock, NULL);
	if (ret)
		goto out;

	fs->fs_blocksize = block_size;
	if (superblock == CMFS_SUPER_BLOCK_BLKNO) {
		ret = cmfs_malloc_block(fs->fs_io, &fs->fs_orig_super);
		if (ret)
			goto out;
		memcpy((char *)fs->fs_orig_super,
			(char *)fs->fs_super,
			fs->fs_blocksize);
	}

	ret = CMFS_ET_CORRUPT_SUPERBLOCK;
	if (!CMFS_RAW_SB(fs->fs_super)->s_blocksize_bits)
		goto out;
	if (fs->fs_super->i_blkno != superblock)
		goto out;
	
	if ((CMFS_RAW_SB(fs->fs_super)->s_clustersize_bits <
	     CMFS_MIN_CLUSTERSIZE_BITS) ||
	    (CMFS_RAW_SB(fs->fs_super)->s_clustersize_bits >
	     CMFS_MAX_CLUSTERSIZE_BITS))
		goto out;
	if (!CMFS_RAW_SB(fs->fs_super)->s_root_blkno ||
	    !CMFS_RAW_SB(fs->fs_super)->s_system_dir_blkno)
		goto out;

	ret = CMFS_ET_UNEXPECTED_BLOCK_SIZE;
	if ((1 << CMFS_RAW_SB(fs->fs_super)->s_blocksize_bits) !=
	    CMFS_MAX_BLOCKSIZE)
		goto out;

	fs->fs_clustersize =
		1 << CMFS_RAW_SB(fs->fs_super)->s_clustersize_bits;

	/* XXX: read system dir */
	fs->fs_root_blkno =
		CMFS_RAW_SB(fs->fs_super)->s_root_blkno;
	fs->fs_sysdir_blkno =
		CMFS_RAW_SB(fs->fs_super)->s_system_dir_blkno;
	fs->fs_clusters = fs->fs_super->i_clusters;
	fs->fs_blocks = cmfs_clusters_to_blocks(fs, fs->fs_clusters);
	fs->fs_first_cg_blkno =
		CMFS_RAW_SB(fs->fs_super)->s_first_cluster_group;

	raw_uuid = CMFS_RAW_SB(fs->fs_super)->s_uuid;
	for (i = 0, ptr = fs->uuid_str; i < CMFS_VOL_UUID_LEN; i++) {
		/* print with null */
		len = snprintf(ptr, 3, "%02X", raw_uuid[i]);
		if (len != 2) {
			ret = CMFS_ET_INTERNAL_FAILURE;
			goto out;
		}
		/* only advance past the last char */
		ptr += 2;
	}

	*ret_fs = fs;
	return 0;
out:
	cmfs_freefs(fs);
	return ret;
}


































