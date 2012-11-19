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

#include <cmfs/cmfs.h>


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

	/*
	 * If CMFS_FLAG_IMAGE_FILE is specified, it needs to be
	 * handled differently
	 */
	if (flags & CMFS_FLAG_IMAGE_FILE) {
		ret = cmfs_image_load_bitmap(fs);
		if (ret)
			goto out;
		if (!superblock)
			superblock = fs->ost->ost_superblocks[0];
	} else {
		if (io_is_device_readonly(fs->fs_io))
			fs->fs_flags |= CMFS_FLAG_HARD_RO;
	}


	if (!superblock)
		superblock = CMFS_SUPER_BLOCK_NO;
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

	ret = CMFS_ET_UNSUPP_FEATURE;
	if (CMFS_RAW_SB(fs->fs_super)->s_feature_incompat &
	    ~CMFS_LIB_FEATURE_INCOMPAT_SUPP)
		goto out;

	ret = CMFS_ET_RO_UNSUPP_FEATURE;
	if((flags & CMFS_FLAG_RW) &&
	   (CMFS_RAW_SB(fs->fs_super)->s_feature_ro_compat &
	    ~CMFS_LIB_FEATURE_RO_COMPAT_SUPP))
		goto out;

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
	if ((1 << CMFS_RAW_SB(fs->super)->s_blocksize_bits) !=
	    CMFS_MAX_BLOCKSIZE)
		goto out;

	fs->fs_clustersize =
		1 << CMFS_RAW_SB(fs->super)->s_clustersize_bits;

	/* XXX: read system dir */
	fs->fs_root_blkno =
		CMFS_RAW_SB(fs->super)->s_root_blkno;
	fs->fs_sysdir_blkno =
		CMFS_RAW_SB(fs->super)->s_system_dir_blkno;
	fs->fs_clusters = fs->fs_super->i_clusters;
	fs->fs_blocks = cmfs_clusters_to_blocks(fs, fs->fs_clusters);
	fs->fs_first_cg_blkno =
		CMFS_RAW_SB(fs->fs_super)->s_first_cluster_group;

	raw_uuid = CMFS_RAW_SB(fs->super)->s_uuid;
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



































