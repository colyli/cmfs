/*
 * member_offset.c
 *
 * CMFS utility to show member offset from data structures.
 * (copied and modified from sizetest.c of ocfs2-tools)
 *
 * Copyright (C) 2004 Oracle Corporation.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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
 * Authors: Sunil Mushran
 * CMFS modification: Coly Li <i@coly.li>
 */

#include <stdio.h>
#include <stdint.h>
#include <cmfs-kernel/cmfs_fs.h>

#undef offsetof
#define offsetof(TYPE, MEMBER)	((unsigned long)&((TYPE *)0)->MEMBER)
#define ssizeof(TYPE, MEMBER)	((unsigned long)sizeof((TYPE *)0)->MEMBER)

#define START_TYPE(TYPE) do {\
	printf("[off]\t%-20s\t[size]\n", #TYPE);\
}while(0)

#define SHOW_OFFSET(TYPE, MEMBER) do {\
	printf("0x%03X\t%-20s\t+0x%02X\n",\
		offsetof(TYPE, MEMBER),\
		#MEMBER,\
		ssizeof(TYPE, MEMBER));\
}while(0)

#define END_TYPE(TYPE) do {\
	printf("\t%-20s\t0x%03X\n", "Total", sizeof(*((TYPE *)0)));\
}while(0)


void print_cmfs_vol_disk_hdr() {
	START_TYPE(cmfs_vol_disk_hdr);
	SHOW_OFFSET(struct cmfs_vol_disk_hdr, signature);
	SHOW_OFFSET(struct cmfs_vol_disk_hdr, mount_point);
	END_TYPE(struct cmfs_vol_disk_hdr);
	printf("\n");
}

void print_cmfs_vol_label() {
        START_TYPE(cmfs_vol_label);
	SHOW_OFFSET(struct cmfs_vol_label, label);
	SHOW_OFFSET(struct cmfs_vol_label, label_len);
	SHOW_OFFSET(struct cmfs_vol_label, vol_id);
	SHOW_OFFSET(struct cmfs_vol_label, vol_id_len);
	END_TYPE(struct cmfs_vol_label);
	printf("\n");
}

void print_cmfs_block_check() {
	START_TYPE(cmfs_block_check);
	SHOW_OFFSET(struct cmfs_block_check,bc_crc32e);
	SHOW_OFFSET(struct cmfs_block_check, bc_ecc);
	SHOW_OFFSET(struct cmfs_block_check, bc_reserved1);
	END_TYPE(struct cmfs_block_check);
	printf("\n");
}

void print_cmfs_extent_rec() {
	START_TYPE(cmfs_extent_rec);
	SHOW_OFFSET(struct cmfs_extent_rec, e_cpos);
	SHOW_OFFSET(struct cmfs_extent_rec, e_blkno);
	SHOW_OFFSET(struct cmfs_extent_rec, e_int_clusters);
	SHOW_OFFSET(struct cmfs_extent_rec, e_leaf_blocks);
	SHOW_OFFSET(struct cmfs_extent_rec, e_flags);
	SHOW_OFFSET(struct cmfs_extent_rec, e_reserved1);
	SHOW_OFFSET(struct cmfs_extent_rec, e_reserved2);
	SHOW_OFFSET(struct cmfs_extent_rec, e_reserved3);
	END_TYPE(struct cmfs_extent_rec);
	printf("\n");
}

void print_cmfs_extent_list() {
	START_TYPE(cmfs_extent_list);
	SHOW_OFFSET(struct cmfs_extent_list, l_tree_depth);
	SHOW_OFFSET(struct cmfs_extent_list, l_count);
	SHOW_OFFSET(struct cmfs_extent_list, l_next_free_rec);
	SHOW_OFFSET(struct cmfs_extent_list, l_reserved1);
	SHOW_OFFSET(struct cmfs_extent_list, l_reserved2);
	SHOW_OFFSET(struct cmfs_extent_list, l_recs);
	END_TYPE(struct cmfs_extent_list);
	printf("\n");
}

void print_cmfs_chain_rec() {
	START_TYPE(cmfs_chain_rec);
	SHOW_OFFSET(struct cmfs_chain_rec, c_free);
	SHOW_OFFSET(struct cmfs_chain_rec, c_total);
	SHOW_OFFSET(struct cmfs_chain_rec, c_blkno);
	END_TYPE(struct cmfs_chain_rec);
	printf("\n");
}

void print_cmfs_chain_list() {
	START_TYPE(cmfs_chain_list);
	SHOW_OFFSET(struct cmfs_chain_list, cl_cpg);
	SHOW_OFFSET(struct cmfs_chain_list, cl_bpc);
	SHOW_OFFSET(struct cmfs_chain_list, cl_count);
	SHOW_OFFSET(struct cmfs_chain_list, cl_next_free_rec);
	SHOW_OFFSET(struct cmfs_chain_list, cl_reserved1);
	SHOW_OFFSET(struct cmfs_chain_list, cl_recs);
	END_TYPE(struct cmfs_chain_list);
	printf("\n");
}

void print_cmfs_local_alloc() {
	START_TYPE(cmfs_local_alloc);
	SHOW_OFFSET(struct cmfs_local_alloc, la_bm_off);
	SHOW_OFFSET(struct cmfs_local_alloc, la_size);
	SHOW_OFFSET(struct cmfs_local_alloc, la_reserved1);
	SHOW_OFFSET(struct cmfs_local_alloc, la_bitmap);
	END_TYPE(struct cmfs_local_alloc);
	printf("\n");
}

void print_cmfs_truncate_rec() {
	START_TYPE(cmfs_truncate_rec);
	SHOW_OFFSET(struct cmfs_truncate_rec, t_start);
	SHOW_OFFSET(struct cmfs_truncate_rec, t_clusters);
	END_TYPE(struct cmfs_truncate_rec);
	printf("\n");
}

void print_cmfs_truncate_log() {
	START_TYPE(cmfs_truncate_log);
  	SHOW_OFFSET(struct cmfs_truncate_log, tl_count);
  	SHOW_OFFSET(struct cmfs_truncate_log, tl_used);
  	SHOW_OFFSET(struct cmfs_truncate_log, tl_reserved1);
  	SHOW_OFFSET(struct cmfs_truncate_log, tl_recs);
	END_TYPE(struct cmfs_truncate_log);
	printf("\n");
}

void print_cmfs_inline_data() {
	START_TYPE(cmfs_inline_data);
	SHOW_OFFSET(struct cmfs_inline_data, id_count);
	SHOW_OFFSET(struct cmfs_inline_data, id_reserved1);
	SHOW_OFFSET(struct cmfs_inline_data, id_data);
	END_TYPE(struct cmfs_inline_data);
	printf("\n");
}

void print_cmfs_super_block() {
	START_TYPE(cmfs_super_block);
	SHOW_OFFSET(struct cmfs_super_block, s_major_rev_level);
	SHOW_OFFSET(struct cmfs_super_block, s_minor_rev_level);
	SHOW_OFFSET(struct cmfs_super_block, s_mnt_count);
	SHOW_OFFSET(struct cmfs_super_block, s_max_mnt_count);
	SHOW_OFFSET(struct cmfs_super_block, s_state);
	SHOW_OFFSET(struct cmfs_super_block, s_errors);
	SHOW_OFFSET(struct cmfs_super_block, s_checkinterval);
	SHOW_OFFSET(struct cmfs_super_block, s_lastcheck);
	SHOW_OFFSET(struct cmfs_super_block, s_creator_os);
	SHOW_OFFSET(struct cmfs_super_block, s_feature_compat);
	SHOW_OFFSET(struct cmfs_super_block, s_feature_incompat);
	SHOW_OFFSET(struct cmfs_super_block, s_feature_ro_compat);
	SHOW_OFFSET(struct cmfs_super_block, s_root_blkno);
	SHOW_OFFSET(struct cmfs_super_block, s_system_dir_blkno);
	SHOW_OFFSET(struct cmfs_super_block, s_blocksize_bits);
	SHOW_OFFSET(struct cmfs_super_block, s_clustersize_bits);
	SHOW_OFFSET(struct cmfs_super_block, s_label);
	SHOW_OFFSET(struct cmfs_super_block, s_uuid);
	SHOW_OFFSET(struct cmfs_super_block, s_tunefs_flag);
	SHOW_OFFSET(struct cmfs_super_block, s_uuid_hash );
	SHOW_OFFSET(struct cmfs_super_block, s_first_cluster_group);
	SHOW_OFFSET(struct cmfs_super_block, s_xattr_inline_size);
	SHOW_OFFSET(struct cmfs_super_block, s_reserved1);
	END_TYPE(struct cmfs_super_block);
	printf("\n");
}

void print_cmfs_dinode() {
	START_TYPE(cmfs_dinode);
	SHOW_OFFSET(struct cmfs_dinode, i_signature);
	SHOW_OFFSET(struct cmfs_dinode, i_generation);
	SHOW_OFFSET(struct cmfs_dinode, i_links_count);
	SHOW_OFFSET(struct cmfs_dinode, i_uid);
	SHOW_OFFSET(struct cmfs_dinode, i_gid);
	SHOW_OFFSET(struct cmfs_dinode, i_size);
	SHOW_OFFSET(struct cmfs_dinode, i_atime);
	SHOW_OFFSET(struct cmfs_dinode, i_ctime);
	SHOW_OFFSET(struct cmfs_dinode, i_mtime);
	SHOW_OFFSET(struct cmfs_dinode, i_dtime);
	SHOW_OFFSET(struct cmfs_dinode, i_flags);
	SHOW_OFFSET(struct cmfs_dinode, i_mode);
	SHOW_OFFSET(struct cmfs_dinode, i_suballoc_slot);
	SHOW_OFFSET(struct cmfs_dinode, i_blkno);
	SHOW_OFFSET(struct cmfs_dinode, i_clusters);
	SHOW_OFFSET(struct cmfs_dinode, i_fs_generation);
	SHOW_OFFSET(struct cmfs_dinode, i_last_eb_blk);
	SHOW_OFFSET(struct cmfs_dinode, i_last_eb_blk);
	SHOW_OFFSET(struct cmfs_dinode, i_atime_nsec);
	SHOW_OFFSET(struct cmfs_dinode, i_ctime_nsec);
	SHOW_OFFSET(struct cmfs_dinode, i_mtime_nsec);
	SHOW_OFFSET(struct cmfs_dinode, i_attr);
	SHOW_OFFSET(struct cmfs_dinode, i_xattr_loc);
	SHOW_OFFSET(struct cmfs_dinode, i_refcount_loc);
	SHOW_OFFSET(struct cmfs_dinode, i_suballoc_loc);
	SHOW_OFFSET(struct cmfs_dinode, i_check);
	SHOW_OFFSET(struct cmfs_dinode, i_dyn_features);
	SHOW_OFFSET(struct cmfs_dinode, i_xattr_inline_size);
	SHOW_OFFSET(struct cmfs_dinode, i_reserved1);
	SHOW_OFFSET(struct cmfs_dinode, id1.i_pad1);
	SHOW_OFFSET(struct cmfs_dinode, id1.dev1.i_rdev);
	SHOW_OFFSET(struct cmfs_dinode, id1.bitmap1.i_used);
	SHOW_OFFSET(struct cmfs_dinode, id1.bitmap1.i_total);
	SHOW_OFFSET(struct cmfs_dinode, id1.journal1.ij_flags);
	SHOW_OFFSET(struct cmfs_dinode, id1.journal1.ij_recovery_generation);
	SHOW_OFFSET(struct cmfs_dinode, id2.i_super);
	SHOW_OFFSET(struct cmfs_dinode, id2.i_lab);
	SHOW_OFFSET(struct cmfs_dinode, id2.i_chain);
	SHOW_OFFSET(struct cmfs_dinode, id2.i_list);
	SHOW_OFFSET(struct cmfs_dinode, id2.i_dealloc);
	SHOW_OFFSET(struct cmfs_dinode, id2.i_data);
	SHOW_OFFSET(struct cmfs_dinode, id2.i_symlink);
	END_TYPE(struct cmfs_dinode);
	printf("\n");
}

void print_cmfs_dir_entry() {
	START_TYPE(cmfs_dir_entry);
	SHOW_OFFSET(struct cmfs_dir_entry, inode);
	SHOW_OFFSET(struct cmfs_dir_entry, rec_len);
	SHOW_OFFSET(struct cmfs_dir_entry, name_len);
	SHOW_OFFSET(struct cmfs_dir_entry, file_type);
	SHOW_OFFSET(struct cmfs_dir_entry, name);
	END_TYPE(struct cmfs_dir_entry);
	printf("\n");
}

void print_cmfs_dir_block_trailer() {
	START_TYPE(cmfs_dir_block_trailer);
	SHOW_OFFSET(struct cmfs_dir_block_trailer, db_compat_inode);
	SHOW_OFFSET(struct cmfs_dir_block_trailer, db_compat_rec_len);
	SHOW_OFFSET(struct cmfs_dir_block_trailer, db_compat_name_len);
	SHOW_OFFSET(struct cmfs_dir_block_trailer, db_reserved0);
	SHOW_OFFSET(struct cmfs_dir_block_trailer, db_free_rec_len);
	SHOW_OFFSET(struct cmfs_dir_block_trailer, db_signature);
	SHOW_OFFSET(struct cmfs_dir_block_trailer, db_reserved2);
	SHOW_OFFSET(struct cmfs_dir_block_trailer, db_free_next);
	SHOW_OFFSET(struct cmfs_dir_block_trailer, db_blkno);
	SHOW_OFFSET(struct cmfs_dir_block_trailer, db_reserved3);
	SHOW_OFFSET(struct cmfs_dir_block_trailer, db_check);
	END_TYPE(struct cmfs_dir_block_trailer);
	printf("\n");
}

int main(int argc, char **argv)
{	
	print_cmfs_vol_disk_hdr();
	print_cmfs_vol_label();
	print_cmfs_block_check();
	print_cmfs_extent_rec();
	print_cmfs_extent_list();
	print_cmfs_chain_rec();
	print_cmfs_chain_list();
	print_cmfs_local_alloc();
	print_cmfs_truncate_rec();
	print_cmfs_truncate_log();
	print_cmfs_inline_data();
	print_cmfs_super_block();
	print_cmfs_dinode();
	print_cmfs_dir_entry();
	print_cmfs_dir_block_trailer();

	return 0;
}
