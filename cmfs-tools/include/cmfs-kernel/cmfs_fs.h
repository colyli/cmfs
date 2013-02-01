/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * cmfs_fs.h
 * (copied and modified from ocfs2_fs.h from ocfs2-tools)
 *
 * On-disk structures for CMFS.
 *
 * Copyright (C) 2002, 2004 Oracle. All rights reserved.
 * CMFS modification, Copyright (C) 2013, Coly Li <i@coly.li>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License, version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#ifndef _CMFS_FS_H
#define _CMFS_FS_H

#include <linux/types.h>
#include <stdint.h>
#include <stddef.h>

/* Version */
#define CMFS_MAJOR_REV_LEVEL		0
#define CMFS_MINOR_REV_LEVEL		1

/* XXX: currently hard code to 2 */
#define CMFS_PER_CPU_NUM		2
/*
 * A CMFS volume starts this way:
 * Sector 0: Valid cmfs_vol_disk_hdr
 * Sector 1: Valid cmfs_vol_label
 * Block CMFS_SUPER_BLOCK_BLKNO: CMFS superblock
 *
 * All other structures are found from the superblock information.
 *
 * CMFS_SUPER_BLOCK_BLKNO is in blocks other than sectors, it is
 * 4096 bytes into disk/partition.
 */
#define CMFS_SUPER_BLOCK_BLKNO		2

#define CMFS_MAX_VOL_SIGNATURE_LEN	128
#define CMFS_MAX_MOUNT_POINT_LEN	128
#define CMFS_MAX_VOL_LABEL_LEN		 64
#define CMFS_MAX_VOL_ID_LENGTH		 16
#define CMFS_VOL_UUID_LEN		 16
#define CMFS_MAX_FILENAME_LEN		255

/* Slot map indicator for an empty slot */
#define CMFS_INVALID_SLOT		-1
#define CMFS_MAX_SLOTS			255

/*
 * Min block size = Max block size = 4KB
 * Min cluster size: 4KB
 * Max cluster size: 32MB
 */
#define CMFS_MIN_CLUSTERSIZE		4096
#define CMFS_MIN_CLUSTERSIZE_BITS	12
#define CMFS_MAX_CLUSTERSIZE		(32*(1<<20))
#define CMFS_MAX_CLUSTERSIZE_BITS	25
#define CMFS_DEFAULT_CLUSTERSIZE	(1<<20)
#define CMFS_MAX_BLOCKSIZE		CMFS_MIN_CLUSTERSIZE
#define CMFS_MIN_BLOCKSIZE		CMFS_MAX_BLOCKSIZE

/* Signature strings */
#define CMFS_SUPER_BLOCK_SIGNATURE	"CMFSV1"
#define CMFS_INODE_SIGNATURE		"INODE01"
#define CMFS_EXTENT_BLOCK_SIGNATURE	"EXBLK01"
#define CMFS_GROUP_DESC_SIGNATURE	"GROUP01"
#define CMFS_DIR_TRAILER_SIGNATURE	"DIRTRL1"

/*
 * Flags on cmfs_dinode.i_flags
 */
#define CMFS_VALID_FL		(0x00000001)	/* Inode is valid */
#define CMFS_UNUSED1_FL		(0x00000002)
#define CMFS_ORPHANED_FL	(0x00000004)	/* On the orphan list */
#define CMFS_UNUSED2_FL		(0x00000008)
/* System inode flags */
#define CMFS_SYSTEM_FL		(0x00000010)	/* System inode */
#define CMFS_SUPER_BLOCK_FL	(0x00000020)	/* Super block */
#define CMFS_LOCAL_ALLOC_FL	(0x00000040)	/* Slot local alloc bitmap */
#define CMFS_BITMAP_FL		(0x00000080)	/* Allocation bitmap */
#define CMFS_JOURNAL_FL		(0x00000100)	/* Journal */
#define CMFS_CHAIN_FL		(0x00000200)	/* Chain allocator */
#define CMFS_DEALLOC_FL		(0x00000400)	/* Truncate log */

/*
 * Flags on cmfs_dinode.i_dyn_features
 *
 * These can change much more often than i_flags. When adding
 * flags, keep in mind that i_dyn_features in only 16 bits
 * wide.
 *
 * XXX: Currently most of them will not be supported
 */
#define CMFS_INLINE_DATA_FL	(0x0001)	/* Data stored in inode block */
#define CMFS_HAS_XATTR_FL	(0x0002)
#define CMFS_INLINE_XATTR_FL	(0x0004)
#define CMFS_INDEXED_DIR_FL	(0x0008)
#define CMFS_HAS_REFCOUNT_FL	(0x0010)

/*
 * CMFS directory file types. Only the low 3 bits are used.
 * The other bits are reserved now
 */
#define CMFS_FT_UNKNOWN		0
#define CMFS_FT_REG_FILE	1
#define CMFS_FT_DIR		2
#define CMFS_FT_CHRDEV		3
#define CMFS_FT_BLKDEV		4
#define CMFS_FT_FIFO		5
#define CMFS_FT_SOCK		6
#define CMFS_FT_SYMLINK		7

#define CMFS_FT_MAX		8

/*
 * Directory entry related stuffs
 *
 * CMFS_DIR_PAD defines the directory entries boundaries
 * Note: it must be a multiple of 4
 */
#define CMFS_DIR_PAD			4
#define CMFS_DIR_ROUND			(CMFS_DIR_PAD - 1)
#define CMFS_DIR_MEMBER_LEN		offsetof(struct cmfs_dir_entry, name)
#define CMFS_DIR_REC_LEN(name_len)	(((name_len) + CMFS_DIR_MEMBER_LEN + \
					  CMFS_DIR_ROUND) & \
					 ~CMFS_DIR_ROUND)
#define CMFS_DIR_MIN_REC_LEN		CMFS_DIR_REC_LEN(1)

#define CMFS_LINK_MAX		32000
#define CMFS_DX_LINK_MAX	((1U<<31) - 1U)
#define CMFS_LINKS_HI_SHIFT	16
#define CMFS_DX_ENTRIES_MAX	(0xffffffffU)


#define CMFS_RAW_SB(dinode)	(&((dinode)->id2.i_super))
#define CMFS_HAS_COMPAT_FEATURE(sb, mask) \
	(CMFS_SB(sb)->s_feature_compat & (mask))


/* Compatibility flags */
#define CMFS_FEATURE_COMPAT_BACKUP_SB		0x0001
#define CMFS_FEATURE_COMPAT_HAS_JOURNAL		0x0002
#define CMFS_FEATURE_COMPAT_INLINE_DATA		0x0004
#define CMFS_FEATURE_COMPAT_META_ECC		0x0008
#define CMFS_FEATURE_COMPAT_INDEXED_DIRS	0x0010
#define CMFS_FEATURE_COMPAT_REFCOUNT_TREE	0x0020
#define CMFS_FEATURE_COMPAT_UNWRITTEN		0x0040
#define CMFS_FEATURE_COMPAT_TUNEFS_INPROG	0x0080
#define CMFS_FEATURE_COMPAT_RESIZE_INPROG	0x0100



/* System file index */
enum {
	BAD_BLOCK_SYSTEM_INODE = 0,
	GLOBAL_INODE_ALLOC_SYSTEM_INODE,
	GLOBAL_BITMAP_SYSTEM_INODE,
#define CMFS_LAST_GLOBAL_SYSTEM_INODE GLOBAL_BITMAP_SYSTEM_INODE
//	ORPHAN_DIR_SYSTEM_INODE,
#define CMFS_FIRST_LOCAL_SYSTEM_INODE EXTENT_ALLOC_SYSTEM_INODE
	EXTENT_ALLOC_SYSTEM_INODE,
	INODE_ALLOC_SYSTEM_INODE,
	JOURNAL_SYSTEM_INODE,
	LOCAL_ALLOC_SYSTEM_INODE,
	TRUNCATE_LOG_SYSTEM_INODE,
#define CMFS_LAST_LOCAL_SYSTEM_INODE TRUNCATE_LOG_SYSTEM_INODE
	NUM_SYSTEM_INODES
};
#define NUM_GLOBAL_SYSTEM_INODES CMFS_FIRST_LOCAL_SYSTEM_INODE
#define NUM_LOCAL_SYSTEM_INODES \
		(NUM_SYSTEM_INODES - CMFS_FIRST_LOCAL_SYSTEM_INODE)

/*
 * CMFS volume header, lives at sector 0.
 */
struct cmfs_vol_disk_hdr {
	uint8_t signature[CMFS_MAX_VOL_SIGNATURE_LEN];
	uint8_t mount_point[CMFS_MAX_MOUNT_POINT_LEN];
};

/*
 * CMFS volume label, lives at sector 1.
 */
struct cmfs_vol_label {
	uint8_t label[CMFS_MAX_VOL_LABEL_LEN];
	uint16_t label_len;
	uint8_t vol_id[CMFS_MAX_VOL_ID_LENGTH];
	uint16_t vol_id_len;
};

/*
 * Block checking structure. This is used in metadata to validate the
 * contents. If CMFS_FEATURE_INCOMPAT_META_ECC is not set, it is all
 * zeros.
 */
struct cmfs_block_check {
/*00*/	__le32 bc_crc32e;
	__le16 bc_ecc;
	__le16 bc_reserved1;
/*08*/
};

/*
 * On disk extent record for CMFS
 * It describes a range of blocks on disk.
 *
 * Length fields are devided into interior and leaf node version.
 * This leaves room for a flags field (CMFS_EXT_*) in the leaf nodes.
 */
struct cmfs_extent_rec {
/*00*/	__le64 e_cpos;		/* Offset into the file, in clusters */
/*08*/	__le64 e_blkno;		/* Physical disk offset, in blocks */
/*10*/	union {
		__le64 e_int_clusters;	/* Clusters covered by all children */
//		__le64 e_int_blocks;	/* blocks covered by all children */
		struct {
			__le32 e_leaf_blocks;	/* blocks covered by this extent
		       				   for 4KB block, the max length
						   of single extent is 16TB */
			uint8_t e_flags;	/* extent flags */
			uint8_t e_reserved1;
			__le16	e_reserved2;
			__le64	e_reserved3;
		};
	};
/*20*/
};

/*
 * On disk extent list for CMFS (node in teh tree). Note that this
 * is contained inside cmfs_dinode or cmfs_extent_block, so the
 * offsets are relative to cmfs_dinode.id2.i_list or
 * cmfs_extent_block.h_list, respectively.
 */
struct cmfs_extent_list {
/*00*/	__le16 l_tree_depth;	/* Extent tree depth from this point.
				   0 means data extents hang directly
				   off this header (a leaf)
				   NOTE: the high 8 bits cannot be
				   used - tree_depth is never that big.
				 */
	__le16 l_count;		/* Number of extent records */
	__le16 l_next_free_rec;	/* Next unused extent slot */
	__le16 l_reserved1;
/*10*/	__le64 l_reserved2[3];	/* Pad to sizeof(cmfs_extent_rec) */
/*20*/	struct cmfs_extent_rec l_recs[0];	/* Extent records */
};

/*
 * On disk chain record format within chain list for CMFS.
 */
struct cmfs_chain_rec {
/*00*/	__le32 c_free;	/* Number of free bits in this chain */
	__le32 c_total;	/* Number of total bits in this chain */
	__le64 c_blkno;	/* Physical disk offset (blocks) of 1st group */
/*10*/
};

/*
 * On disk allocation chain list for CMFS. Note that this is
 * contained inside cmfs_inode, so the offsets are relative to
 * cmfs_dinode.id2.i_chain.
 */
struct cmfs_chain_list {
/*00*/	__le16 cl_cpg;			/* Clusters per Block Group */
	__le16 cl_bpc;			/* Bits per Cluster */
	__le16 cl_count;		/* Total chains in this list */
	__le16 cl_next_free_rec;	/* Next unused chain slot */
	__le64 cl_reserved1;
/*10*/	struct cmfs_chain_rec cl_recs[0];	/* Chain records */
};

/*
 * Local allocation bitmap for CMFS per-cpu slots
 * Note that it exists inside cmfs_dinode, so all offsets
 * are relative to the start of cmfs_dinode.id2.
 */
struct cmfs_local_alloc {
/*00*/	__le32 la_bm_off;	/* Starting bit offset in main bitmap */
	__le16 la_size;		/* Size of included bitmap, in bytes */
	__le16 la_reserved1[5];
/*10*/	uint8_t	la_bitmap[0];
};

/*
 * On disk truncate record format for truncate log
 */
struct cmfs_truncate_rec {
	__le32 t_start;		/* 1st cluster in this log */
	__le32 t_clusters;	/* Number of total clusters convered */
};

/*
 * On disk deallocation log for CMFS. Note that this is
 * contained inside cmfs_dinode, so the offsets are
 * relative to cmfs_dinode.id2.i_dealloc.
 */
struct cmfs_truncate_log {
/*00*/	__le16 tl_count;			/* Total records in this log*/
	__le16 tl_used;				/* Number of records in use*/
	__le16 tl_reserved1[6];
/*10*/	struct cmfs_truncate_rec tl_recs[0];	/* Truncate records */
};

/*
 * Data-in-inode header. This is only used for i_dyn_features
 * has CMFS_INLINE_DATA_FL set
 */
struct cmfs_inline_data {
/*00*/	__le16 id_count;	/* Number of bytes can be used for data,
				   starting at id_data */
	__le16 id_reserved1[3];
/*08*/	uint8_t id_data[0];	/* Start of user data */
};

/*
 * On disk superblock for CMFS
 * Note that it is contained inside cmfs_dinode, so all offsets
 * are relative to the start of cmfs_dinode.id2.
 */
struct cmfs_super_block {
/*00*/	__le16 s_major_rev_level;
	__le16 s_minor_rev_level;
	__le16 s_mnt_count;
	__le16 s_max_mnt_count;
	__le16 s_state;
	__le16 s_errors;
	__le32 s_checkinterval;
/*10*/	__le64 s_lastcheck;
	__le32 s_creator_os;
	__le32 s_feature_compat;
/*20*/	__le32 s_feature_incompat;
	__le32 s_feature_ro_compat;
	__le64 s_root_blkno;
/*30*/	__le64 s_system_dir_blkno;
	__le32 s_blocksize_bits;
	__le32 s_clustersize_bits;
/*40*/	uint8_t s_label[CMFS_MAX_VOL_LABEL_LEN];
/*80*/	uint8_t s_uuid[CMFS_VOL_UUID_LEN];
/*90*/	__le16 s_tunefs_flag;
	__le16 s_xattr_inline_size;
	__le32 s_uuid_hash;
	__le64 s_first_cluster_group;
/*A0*/
/* XXXX: should pad all rest space of dinode which contains the super block to zero */
};


/*
 * On disk CMFS inode format
 */
struct cmfs_dinode {
/*00*/	uint8_t i_signature[8];		/* Signature for validation */
	__le32 i_generation;		/* Generation number */
	__le32 i_links_count;		/* Links count */
/*10*/	__le32 i_uid;			/* Owner UID */
	__le32 i_gid;			/* Owner GID */
	__le64 i_size;
/*20*/	__le64 i_atime;
	__le64 i_ctime;
/*30*/	__le64 i_mtime;
	__le64 i_dtime;
/*40*/	__le32 i_flags;
	__le16 i_mode;
	__le16 i_suballoc_slot;		/* Slot suballocator this inode
					   belongs to */
	__le64 i_blkno;
/*50*/	__le32 i_clusters;		/* Cluster count */
	__le32 i_fs_generation;
	__le64 i_last_eb_blk;
/*60*/	__le32 i_atime_nsec;
	__le32 i_ctime_nsec;
	__le32 i_mtime_nsec;
	__le32 i_attr;
/*70*/	__le64 i_xattr_loc;
	__le64 i_refcount_loc;
/*80*/	__le64 i_suballoc_loc;
/*88*/ struct cmfs_block_check i_check;
/*90*/	__le16 i_dyn_features;
	__le16 i_xattr_inline_size;
	__le16 i_suballoc_bit;	/* XXX: will move it to the location after i_suballoc_slot */
	__le16 i_reserved1[1];
/*98*/	union {
		__le64 i_pad1;
		struct {
			__le64 i_rdev;	/* Raw device number */
		} dev1;
		struct {
			__le32 i_used;
			__le32 i_total;
		} bitmap1;
		struct {
			__le32 ij_flags;
			__le32 ij_recovery_generation;
		} journal1;
	} id1;
/*A0*/ union {
		struct cmfs_super_block		i_super;
		struct cmfs_local_alloc		i_lab;
		struct cmfs_chain_list		i_chain;
		struct cmfs_extent_list		i_list;
		struct cmfs_truncate_log	i_dealloc;
		struct cmfs_inline_data		i_data;
		uint8_t				i_symlink[0];
	} id2;
/* Actual on-disk size is one block (4KB) */

};

/*
 * On disk directory entry structure for CMFS
 *
 * Packed as this structure is variable size and accessed
 * unsligned on 32/64-bit platform
 */
struct cmfs_dir_entry {
/*00*/	__le64 inode;				/* Inode number */
	__le16 rec_len;				/* Directory entry length */
	uint8_t name_len;			/* Name length */
	uint8_t file_type;
/*0c*/	char name[CMFS_MAX_FILENAME_LEN];	/* File name */
/* Actual on-disk length specified by rec_len */
} __attribute__((packed));

/*
 * Per-block record for the unindexed directry btree.
 * This is carefully crafted so that the rec_len and name_len records
 * of a cmfs_dir_entry are mirrored. That way, the directory manipulation
 * code needs a minial amount of update.
 *
 * NOTE: Keep this structure aligned to a multiple of 4 bytes.
 */
struct cmfs_dir_block_trailer {
/*00*/	__le64	db_compat_inode;	/* Always zero. Was inode */
	__le16	db_compat_rec_len;	/* Backwards compatible with
					   cmfs_dir_entry */
	uint8_t	db_compat_name_len;	/* Always zero. Was name_len */
	uint8_t	db_reserved0;
	__le16	db_reserved1;
	__le16	db_free_rec_len;	/* Size of largest empty hole
					   in this block. */
/*10*/	uint8_t	db_signature[8];	/* Signature for verification */
	__le64	db_reserved2;
/*20*/	__le64	db_free_next;		/* Next block in list */
	__le64	db_blkno;		/* Offset o disk, in blocks */
/*30*/	__le64	db_parent_dinode;	/* dinode which owns me, in
					   blocks */
	__le64	db_reserved3;
/*40*/	struct cmfs_block_check db_check; /* Error checking */
};

/*
 * Largest bigmap for a block (suballocator) group in bytes.
 * This limit does not affect cluster groups (global allocator).
 * Cluster group bitmaps run to the end of the block.
 */
#define CMFS_MAX_BG_BITMAP_SIZE		256
/*
 * On disk allocator group structure for CMFS
 */
struct cmfs_group_desc {
/*00*/	uint8_t	bg_signature[8];		/* Signature for validation */
	__le16	bg_size;			/* Size of included bitmap in bytes */
	__le16	bg_bits;			/* Bits represented by this group */
	__le16	bg_free_bits_count;		/* Free bits count */
	__le16	bg_chain;			/* What chain I am in */
/*10*/	__le32	bg_generation;
	__le32	bg_reserved1;
	__le64	bg_next_group;			/* Next group in my list, in blocks */
/*20*/	__le64	bg_parent_dinode;		/* dinode which owns me, in blocks */
	__le64	bg_blkno;			/* Where desc stored on disk, in blocks */
/*30*/	struct cmfs_block_check bg_check;
	__le64	bg_reserved2;
/*40*/	union {
		uint8_t bg_bitmap[0];
		struct {
/* XXX: remove this latter ? */
			uint8_t bg_bitmap_filler[CMFS_MAX_BG_BITMAP_SIZE];
/*140*/			struct cmfs_extent_list bg_list;
		};
	};
/* Actual on-disk size is one block */
};

static inline int cmfs_group_bitmap_size(int blocksize,
					 int suballocator,
					 uint32_t feature_incompat)
{
	int size = blocksize -
		offsetof(struct cmfs_group_desc, bg_bitmap);

	return size;
}

static inline int cmfs_chain_recs_per_inode(int blocksize)
{
	int size;

	size = blocksize -
		offsetof(struct cmfs_dinode, id2.i_chain.cl_recs);

	return (size/(sizeof(struct cmfs_chain_rec)));
}

static inline int cmfs_max_inline_data(int blocksize,
				       struct cmfs_dinode *di)
{
	return (blocksize - 
		offsetof(struct cmfs_dinode, id2.i_data.id_data));
}

static inline int cmfs_local_alloc_size(int blocksize)
{
	int size;

	size = blocksize -
		offsetof(struct cmfs_dinode, id2.i_lab.la_bitmap);

	return size;
}

static inline int cmfs_truncate_recs_per_inode(int blocksize)
{
	int size;

	size = blocksize -
		offsetof(struct cmfs_dinode, id2.i_dealloc.tl_recs);

	return (size / sizeof(struct cmfs_truncate_rec));
}

static inline int cmfs_extent_recs_per_inode(int blocksize)
{
	int size;

	size = blocksize -
		offsetof(struct cmfs_dinode, id2.i_list.l_recs);

	return size / (sizeof(struct cmfs_extent_rec));
}




































#endif
