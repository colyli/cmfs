/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * cmfs.h
 * (copied and modified from ocfs2.h)
 *
 * Filesystem object routines for the CMFS userspace library.
 *
 * ocfs2.h Authors: Joel Becker
 * cmfs modification, Copyright (C) 2012, Coly Li <i@coly.li>
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
 */


#ifndef _FILESYS_H
#define _FILESYS_H

#include <stdint.h>
#include <cmfs-kernel/cmfs_fs.h>
#include <cmfs/cmfs.h>


/* falgs for cmfs_filesys structure */
#define CMFS_FLAG_RO			0x00
#define CMFS_FLAG_RW			0x01
#define CMFS_FLAG_DIRTY			0x04
#define CMFS_FLAG_BUFFERED			0x10


typedef struct _cmfs_filesys cmfs_filesys;
typedef struct _io_channel io_channel;
typedef struct _cmfs_cached_inode cmfs_cached_inode;
typedef struct _cmfs_dinode cmfs_dinode;
typedef struct _cmfs_bitmap cmfs_bitmap;

struct _cmfs_filesys {
	char *fs_devname;
	uint32_t fs_flags;
	io_channel *fs_io;
	struct cmfs_dinode *fs_super;
	struct cmfs_dinode *fs_orig_super;
	unsigned int fs_blocksize;
	unsigned int fs_clustersize;
	uint32_t fs_clusters;
	uint64_t fs_blocks;
	uint32_t fs_umask;
	uint64_t fs_root_blkno;
	uint64_t fs_sysdir_blkno;
	uint64_t fs_first_cg_blkno;
	char uuid_str[CMFS_VOL_UUID_LEN * 2 + 1];

	/* Allocators */
	cmfs_cached_inode *fs_cluster_alloc;
	cmfs_cached_inode **fs_inode_allocs;
	cmfs_cached_inode *fs_system_inode_alloc;
//	cmfs_cached_inode **fs_eb_allocs;
//	cmfs_cached_inode *fs_system_eb_alloc;

	/* Reserved for the use of the calling application. */
	void *fs_private;
};

struct _cmfs_cached_inode {
	struct _cmfs_filesys *ci_fs;
	uint64_t ci_blkno;
	struct cmfs_dinode *ci_inode;
	cmfs_bitmap *ci_chains;
};

struct cmfs_cluster_group_sizes {
	uint16_t cgs_cpg;
	uint16_t cgs_tail_group_bits;
	uint32_t cgs_cluster_groups;
};


static inline void cmfs_calc_cluster_groups(
					uint64_t clusters,
					uint64_t blocksize,
					struct cmfs_cluster_group_sizes *cgs)
{
	uint32_t max_bits = 8 * cmfs_group_bitmap_size(blocksize, 0, 0);

	cgs->cgs_cpg = max_bits;
	if (max_bits > clusters)
		cgs->cgs_cpg = clusters;

	cgs->cgs_cluster_groups = (clusters + cgs->cgs_cpg - 1) /
				  cgs->cgs_cpg;
	cgs->cgs_tail_group_bits = clusters % cgs->cgs_cpg;
	if (cgs->cgs_tail_group_bits == 0)
		cgs->cgs_tail_group_bits = cgs->cgs_cpg;
}

struct io_vec_unit {
	uint64_t ivu_blkno;
	char *ivu_buf;
	uint32_t ivu_buflen;
};

struct cmfs_io_stats {
	uint64_t is_bytes_read;
	uint64_t is_bytes_written;
	uint32_t is_cache_hits;
	uint32_t is_cache_misses;
	uint32_t is_cache_inserts;
	uint32_t is_cache_removes;
};

void io_get_stats(io_channel *channel, struct cmfs_io_stats *stats);


errcode_t io_set_blksize(io_channel *channel, int blksize);


#endif
