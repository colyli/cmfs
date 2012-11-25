/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * inode.c
 *
 * Inode operations for the CMFS userspace library.
 * (Copied and modified from ocfs2-tools/libocfs2/inode.c)
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
 * Ideas taken from e2fsprogs/lib/ext2fs/inode.c
 *   Copyright (C) 1993, 1994, 1995, 1996 Theodore Ts'o.
 */

#define _XOPEN_SOURCE 600
#define _LARGEFILE64_SOURCE

#include <string.h>
#include <inttypes.h>
#include <sys/stat.h>



#include <cmfs/byteorder.h>
#include <cmfs-kernel/cmfs_fs.h>
#include <cmfs/cmfs.h>

/*
 * XXX: should check cmfs_dinode in include/cmfs-kenrel/cmfs_fs.h
 * to make sure nothing missing here.
 */
static void cmfs_swap_inode_first(struct cmfs_dinode *di)
{
	/* skip i_signature */
	di->i_generation	= bswap_32(di->i_generation);
	di->i_links_count	= bswap_32(di->i_links_count);
	di->i_uid		= bswap_32(di->i_uid);
	di->i_gid		= bswap_32(di->i_gid);
	di->i_size		= bswap_64(di->i_size);
	di->i_atime		= bswap_64(di->i_atime);
	di->i_ctime		= bswap_64(di->i_ctime);
	di->i_mtime		= bswap_64(di->i_mtime);
	di->i_dtime		= bswap_64(di->i_dtime);
	di->i_flags		= bswap_32(di->i_flags);
	di->i_mode		= bswap_16(di->i_mode);
	di->i_suballoc_slot	= bswap_16(di->i_suballoc_slot);
	di->i_blkno		= bswap_64(di->i_blkno);
	di->i_clusters		= bswap_32(di->i_clusters);
	di->i_fs_generation	= bswap_32(di->i_fs_generation);
	di->i_last_eb_blk	= bswap_64(di->i_last_eb_blk);
	di->i_atime_nsec	= bswap_32(di->i_atime_nsec);
	di->i_ctime_nsec	= bswap_32(di->i_ctime_nsec);
	di->i_mtime_nsec	= bswap_32(di->i_mtime_nsec);
	di->i_attr		= bswap_32(di->i_attr);
	di->i_xattr_loc		= bswap_64(di->i_xattr_loc);
	di->i_refcount_loc	= bswap_64(di->i_refcount_loc);
	di->i_suballoc_loc	= bswap_64(di->i_suballoc_loc);
	/* skip i_check */
	di->i_dyn_features	= bswap_16(di->i_dyn_features);
	di->i_xattr_inline_size	= bswap_16(di->i_xattr_inline_size);
	/* skip i_reserved1 */
}

static void cmfs_swap_inode_second(struct cmfs_dinode *di)
{
	if (S_ISCHR(di->i_mode) || S_ISBLK(di->i_mode))
		di->id1.dev1.i_rdev = bswap_64(di->id1.dev1.i_rdev);
	else if (di->i_flags & CMFS_BITMAP_FL) {
		di->id1.bitmap1.i_used =
			bswap_32(di->id1.bitmap1.i_used);
		di->id1.bitmap1.i_total =
			bswap_32(di->id1.bitmap1.i_total);
	} else if (di->i_flags & CMFS_JOURNAL_FL) {
		di->id1.journal1.ij_flags =
			bswap_32(di->id1.journal1.ij_flags);
		di->id1.journal1.ij_recovery_generation =
			bswap_32(di->id1.journal1.ij_recovery_generation);
	}
}

static void cmfs_swap_inode_third(cmfs_filesys *fs, struct cmfs_dinode *di)
{
	int i;
	/*
	 * we need to be careful to swap the union member that is in use.
	 * first the ones that are explicitly marked with flags
	 */
	if (di->i_flags & CMFS_SUPER_BLOCK_FL) {
		struct cmfs_super_block *sb = &di->id2.i_super;

		sb->s_major_rev_level	= bswap_16(sb->s_major_rev_level);
		sb->s_minor_rev_level	= bswap_16(sb->s_minor_rev_level);
		sb->s_mnt_count		= bswap_16(sb->s_mnt_count);
		sb->s_max_mnt_count	= bswap_16(sb->s_max_mnt_count);
		sb->s_state		= bswap_16(sb->s_state);
		sb->s_errors		= bswap_16(sb->s_errors);
		sb->s_checkinterval	= bswap_32(sb->s_checkinterval);
		sb->s_lastcheck		= bswap_64(sb->s_lastcheck);
		sb->s_creator_os	= bswap_32(sb->s_creator_os);
		sb->s_feature_compat	= bswap_32(sb->s_feature_compat);
		sb->s_feature_incompat	= bswap_32(sb->s_feature_incompat);
		sb->s_feature_ro_compat	= bswap_32(sb->s_feature_ro_compat);
		sb->s_root_blkno	= bswap_64(sb->s_root_blkno);
		sb->s_system_dir_blkno	= bswap_64(sb->s_system_dir_blkno);
		sb->s_clustersize_bits	= bswap_32(sb->s_clustersize_bits);
		sb->s_blocksize_bits	= bswap_32(sb->s_blocksize_bits);
		/* skip s_label */
		/* skip s_uuid */
		sb->s_tunefs_flag	= bswap_16(sb->s_tunefs_flag);
		sb->s_xattr_inline_size	= bswap_16(sb->s_xattr_inline_size);
		sb->s_uuid_hash		= bswap_32(sb->s_uuid_hash);
		sb->s_first_cluster_group = bswap_64(sb->s_first_cluster_group);
	} else if (di->i_flags & CMFS_LOCAL_ALLOC_FL) {
		struct cmfs_local_alloc *la = &di->id2.i_lab;

		la->la_bm_off	= bswap_32(la->la_bm_off);
		la->la_size	= bswap_16(la->la_size);

	} else if (di->i_flags & CMFS_CHAIN_FL) {
		struct cmfs_chain_list *cl = &di->id2.i_chain;

		cl->cl_cpg	= bswap_16(cl->cl_cpg);
		cl->cl_bpc	= bswap_16(cl->cl_bpc);
		cl->cl_count	= bswap_16(cl->cl_count);
		cl->cl_next_free_rec = bswap_16(cl->cl_next_free_rec);

		/* swap list items */
		for (i = 0; i < cl->cl_next_free_rec; i++) {
			struct cmfs_chain_rec *rec = &cl->cl_recs[i];
			if (cmfs_swap_barrier(fs, di, rec,
					     sizeof(struct cmfs_chain_rec)))
				break;
			rec->c_free	= bswap_32(rec->c_free);
			rec->c_total	= bswap_32(rec->c_total);
			rec->c_blkno	= bswap_64(rec->c_blkno);
		}
	} else if (di->i_flags & CMFS_DEALLOC_FL) {
		struct cmfs_truncate_log *tl = &di->id2.i_dealloc;

		tl->tl_count	= bswap_16(tl->tl_count);
		tl->tl_used	= bswap_16(tl->tl_used);
		
		for (i = 0; i < tl->tl_count; i++) {
			struct cmfs_truncate_rec *rec;

			rec = &tl->tl_recs[i];
			if (cmfs_swap_barrier(fs, di, rec,
					     sizeof(struct cmfs_truncate_rec)))
				break;

			rec->t_start	= bswap_32(rec->t_start);
			rec->t_clusters	= bswap_32(rec->t_clusters);
		}
	} else if (di->i_dyn_features & CMFS_INLINE_DATA_FL) {
		struct cmfs_inline_data *id = &di->id2.i_data;

		id->id_count = bswap_16(id->id_count);
	}
}

static inline void cmfs_swap_inline_dir(cmfs_filesys *fs,
					struct cmfs_dinode *di,
					int to_cpu)
{
	void *de_buf = di->id2.i_data.id_data;
	uint64_t bytes = di->id2.i_data.id_count;
	int max_inline = cmfs_max_inline_data(fs->fs_blocksize, di);

	/* In case max_inline is garbage */
	if (max_inline < 0)
		max_inline = 0;

	if (bytes > max_inline)
		bytes = max_inline;

	if (to_cpu)
		cmfs_swap_dir_entries_to_cpu(de_buf, bytes);
	else
		cmfs_swap_dir_entries_from_cpu(de_buf, bytes);
}

static int has_extents(struct cmfs_dinode *di)
{
	/* inodes flagged with other stuffs in id2 */
	if (di->i_flags & (CMFS_SUPER_BLOCK_FL | CMFS_LOCAL_ALLOC_FL |
			   CMFS_CHAIN_FL | CMFS_DEALLOC_FL))
		return 0;

	/* i_flags doesn't indicate when id2 is a fast symlink */
	if (S_ISLNK(di->i_mode) && di->i_size && di->i_clusters == 0)
		return 0;

	/* inline data has no extent */
	if (di->i_dyn_features & CMFS_INLINE_DATA_FL)
		return 0;

	return 1;
}

void cmfs_swap_inode_to_cpu(cmfs_filesys *fs, struct cmfs_dinode *di)
{
	if (cpu_is_little_endian)
		return;

	cmfs_swap_inode_first(di);
	cmfs_swap_inode_second(di);
	cmfs_swap_inode_third(fs, di);
	if ((di->i_dyn_features & CMFS_INLINE_DATA_FL) &&
	    S_ISDIR(di->i_mode))
		cmfs_swap_inline_dir(fs, di, 1);
	if (has_extents(di))
		cmfs_swap_extent_list_to_cpu(fs, di, &di->id2.i_list);
}









































































