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
#include <et/com_err.h>
#include <cmfs-kernel/cmfs_fs.h>
#include <cmfs/cmfs.h>


/* falgs for cmfs_filesys structure */
#define CMFS_FLAG_RO			0x00
#define CMFS_FLAG_RW			0x01
#define CMFS_FLAG_CHANGED		0x02
#define CMFS_FLAG_DIRTY			0x04
#define CMFS_FLAG_SWAP_BYTES		0x08
#define CMFS_FLAG_BUFFERED		0x10
#define CMFS_FLAG_IMAGE_FILE		0x20
#define CMFS_FLAG_NO_ECC_CHECKS		0x40
#define CMFS_FLAG_HARD_RO		0x80



/* Check if mounted flags */
#define CMFS_MF_MOUNTED			0x01
#define CMFS_MF_ISROOT			0x02
#define CMFS_MF_READONLY		0x04
#define CMFS_MF_SWAP			0x08
#define CMFS_MF_BUSY			0x10

/* define CMFS_SB for cmfs-tools */
#define CMFS_SB(sb)	(sb)

enum cmfs_block_type {
	CMFS_BLOCK_UNKNOW,
	CMFS_BLOCK_INODE,
	CMFS_BLOCK_SUPERBLOCK,
	CMFS_BLOCK_EXTENT_BLOCK,
	CMFS_BLOCK_GROUP_DESCRIPTOR,
	CMFS_BLOCK_DIR_BLOCK,
};


typedef struct _cmfs_filesys cmfs_filesys;
typedef struct _io_channel io_channel;
typedef struct _cmfs_cached_inode cmfs_cached_inode;
typedef struct _cmfs_dinode cmfs_dinode;
typedef struct _cmfs_bitmap cmfs_bitmap;
typedef struct _cmfs_fs_options cmfs_fs_options;

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

struct _cmfs_fs_options {
	uint32_t opt_compat;
	uint32_t opt_incompat;
	uint32_t opt_ro_compat;
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
	uint32_t max_bits = 8 * cmfs_group_bitmap_size(blocksize, 0);

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

errcode_t cmfs_check_if_mounted(const char *file, int *mount_flags);
void io_get_stats(io_channel *channel, struct cmfs_io_stats *stats);
struct cmfs_dir_block_trailer *cmfs_dir_trailer_from_block(cmfs_filesys *fs,
							   void  *data);
void cmfs_init_dir_trailer(cmfs_filesys *fs,
			   struct cmfs_dinode *di,
			   uint64_t blkno,
			   void *buf);
errcode_t cmfs_swap_dir_entries_from_cpu(void *buf, uint64_t bytes);
errcode_t cmfs_swap_dir_entries_to_cpu(void *buf, uint64_t bytes);
void cmfs_swap_dir_trailer(struct cmfs_dir_block_trailer *trailer);
unsigned int cmfs_dir_trailer_blk_off(cmfs_filesys *fs);
errcode_t io_set_blksize(io_channel *channel, int blksize);
int cmfs_supports_dir_trailer(cmfs_filesys *fs);
errcode_t cmfs_open(const char *name,
		    int flags,
		    unsigned int superblock,
		    unsigned int block_size,
		    cmfs_filesys **ret_fs);
errcode_t cmfs_close(cmfs_filesys *fs);
errcode_t cmfs_flush(cmfs_filesys *fs);
errcode_t cmfs_validate_meta_ecc(cmfs_filesys *fs,
				void *data,
				struct cmfs_block_check *bc);
void cmfs_freefs(cmfs_filesys *fs);
errcode_t cmfs_malloc(unsigned long size, void *ptr);
errcode_t cmfs_malloc0(unsigned long size, void *ptr);
errcode_t cmfs_malloc_blocks(io_channel *channel, int num_blocks, void *ptr);
errcode_t cmfs_malloc_block(io_channel *channel, void *ptr);
errcode_t cmfs_free(void *ptr);
int io_get_blksize(io_channel *channel);
errcode_t cmfs_get_device_size(const char *file,
			       int blocksize,
			       uint64_t *retblocks);
errcode_t cmfs_get_device_sectsize(const char *file, int *sectsize);
errcode_t io_read_block_nocache(io_channel *channel,
				int64_t blkno,
				int count,
				char *data);
void cmfs_swap_inode_to_cpu(cmfs_filesys *fs, struct cmfs_dinode *di);
void cmfs_swap_inode_from_cpu(cmfs_filesys *fs, struct cmfs_dinode *di);
errcode_t io_open(const char *name, int flags, io_channel **channel);
int io_is_device_readonly(io_channel *channel);
errcode_t io_read_block(io_channel *channel,
			int64_t blkno,
			int count,
			char *data);
errcode_t io_close(io_channel *channel);
void cmfs_swap_extent_list_to_cpu(cmfs_filesys *fs,
				  void *obj,
				  struct cmfs_extent_list *el);
void cmfs_swap_extent_list_from_cpu(cmfs_filesys *fs,
				    void *obj,
				    struct cmfs_extent_list *el);
void cmfs_swap_group_desc_from_cpu(cmfs_filesys *fs,
				   struct cmfs_group_desc *gd);
void cmfs_swap_group_desc_to_cpu(cmfs_filesys *fs,
				   struct cmfs_group_desc *gd);
errcode_t cmfs_parse_feature(const char *opts,
			     cmfs_fs_options *feature_flags,
			     cmfs_fs_options *reverse_flags);
errcode_t cmfs_merge_feature_with_default_flags(cmfs_fs_options *dest,
						cmfs_fs_options *feature_set,
						cmfs_fs_options *reverse_set);
errcode_t cmfs_read_extent_block(cmfs_filesys *fs,
				 uint64_t blkno,
				 char *eb_buf);
errcode_t cmfs_lookup(cmfs_filesys *fs,
		      uint64_t dir,
		      const char *name,
		      int namelen,
		      char *buf,
		      uint64_t *inode);
errcode_t cmfs_check_directory(cmfs_filesys *fs,
			       uint64_t dir);
errcode_t cmfs_dir_iterate(cmfs_filesys *fs,
			   uint64_t dir,
			   int flags,
			   char *block_buf,
			   int (*func)(struct cmfs_dir_entry *dirent,
				       uint64_t blocknr,
				       int offset,
				       int blocksize,
				       char *buf,
				       void *prov_data),
			   void *priv_data);
errcode_t cmfs_read_inode(cmfs_filesys *fs,
			  uint64_t blkno,
			  char *inode_buf);
errcode_t cmfs_read_group_desc(cmfs_filesys *fs,
			       uint64_t blkno,
			       char *gd_buf);
errcode_t cmfs_read_dir_block(cmfs_filesys *fs,
			      struct cmfs_dinode *di,
			      uint64_t block,
			      void *buf);
errcode_t cmfs_block_iterate(cmfs_filesys *fs,
			     uint64_t blkno,
			     int flags,
			     int (*func)(cmfs_filesys *fs,
				         uint64_t blkno,
					 uint64_t bcount,
					 uint16_t ext_flags,
					 void *priv_data),
			     void *priv_data);
errcode_t cmfs_block_iterate_inode(cmfs_filesys *fs,
				   struct cmfs_dinode *inode,
				   int flags,
				   int (*func)(cmfs_filesys *fs,
					       uint64_t blkno,
					       uint64_t bcount,
					       uint16_t ext_flags,
					       void *priv_data),
				   void *priv_data);
errcode_t cmfs_block_iterate_inode(cmfs_filesys *fs,
				   struct cmfs_dinode *inode,
				   int flags,
				   int (*func)(cmfs_filesys *fs,
					       uint64_t blkno,
					       uint64_t bcount,
					       uint16_t ext_flags,
					       void *priv_data),
				   void *priv_data);
errcode_t cmfs_snprint_extent_flags(char *str,
				    size_t size,
				    uint8_t flags);
errcode_t cmfs_read_cached_inode(cmfs_filesys *fs,
				 uint64_t blkno,
				 cmfs_cached_inode **ret_ci);
errcode_t cmfs_file_read(cmfs_cached_inode *ci,
			 void *buf,
			 uint32_t count,
			 uint64_t offset,
			 uint32_t *got);
errcode_t cmfs_free_cached_inode(cmfs_filesys *fs,
				 cmfs_cached_inode *cinode);
errcode_t cmfs_read_dir_block(cmfs_filesys *fs,
			      struct cmfs_dinode *di,
			      uint64_t block,
			      void *buf);
errcode_t cmfs_snprint_feature_flags(char *str,
				     size_t size,
				     cmfs_fs_options *flags);

/*
 * ${foo}_to_${bar} is a floor function. blocks_to_clusters() will
 * return the cluster that contains a block, not the number of clusters
 * that hold a given number of blocks.
 *
 * ${foo}_in_${bar} is a ceiling function. clusters_in_blocks()
 * will give the number of clusters needed to hold a given number
 * of blocks.
 *
 * These functions return UINTxx_MAX when they overflow, but UINTxx_MAX
 * cannot be used to check overflow. UINTxx_MAX is a valid value in
 * much of cmfs. the caller is responsible for preventing overflow
 * before using these function.
 */

static inline uint64_t cmfs_clusters_to_blocks(cmfs_filesys *fs,
					       uint32_t clusters)
{
	int c_to_b_bits =
		CMFS_RAW_SB(fs->fs_super)->s_clustersize_bits -
		CMFS_RAW_SB(fs->fs_super)->s_blocksize_bits;

	return (uint64_t)(clusters << c_to_b_bits);
}

static inline uint32_t cmfs_blocks_to_clusters(cmfs_filesys *fs,
					       uint64_t blocks)
{
	int b_to_c_bits =
		CMFS_RAW_SB(fs->fs_super)->s_clustersize_bits -
		CMFS_RAW_SB(fs->fs_super)->s_blocksize_bits;
	uint64_t ret = blocks >> b_to_c_bits;

	if (ret > UINT32_MAX)
		ret = UINT32_MAX;

	return (uint32_t)ret;
}


static inline int cmfs_meta_ecc(struct cmfs_super_block *csb)
{
	if (CMFS_HAS_COMPAT_FEATURE(csb,
				    CMFS_FEATURE_COMPAT_META_ECC))
		return 1;
	return 0;
}

static inline int cmfs_supports_indexed_dirs(struct cmfs_super_block *csb)
{
	if (CMFS_HAS_COMPAT_FEATURE(csb,
				    CMFS_FEATURE_COMPAT_INDEXED_DIRS))
		return 1;
	return 0;
}

/*
 * When we are swapping an element of some kind of list, a mistaken count
 * can lead us to go beyond the edge of a block buffer. This function
 * guards agains that. It returns 1 if the element would walk off the end
 * of the block buffer.
 */
static inline int cmfs_swap_barrier(cmfs_filesys *fs,
				    void *block_buffer,
				    void *element,
				    size_t element_size)
{
	char *limit, *end;

	limit = block_buffer + fs->fs_blocksize;
	end = element + element_size;

	return (end > limit);
}

/*
 * Helper function to look at the # of clusters in an extent record
 */
static inline uint32_t cmfs_rec_clusters(cmfs_filesys *fs,
					 uint16_t tree_depth,
					 struct cmfs_extent_rec *rec)
{
	uint64_t ret;
	/*
	 * Cluster count in extent records is slightly different
	 * between interior nodes and leaf nodes. This is to
	 * support unwritten extents which need a flags field
	 * in leaf node records, thus shrinking the available
	 * space for a clusters field.
	 */
	if (tree_depth)
		ret = rec->e_int_blocks;
	else
		ret = rec->e_leaf_blocks;
	
	return (uint32_t)cmfs_blocks_to_clusters(
					fs, ret);
}

static inline uint32_t cmfs_clusters_in_bytes(cmfs_filesys *fs,
					      uint64_t bytes)
{
	uint64_t ret = bytes + fs->fs_clustersize - 1;
	if (ret < bytes) /* deal with wrapping */
		ret = UINT64_MAX;

	ret = ret >> CMFS_RAW_SB(fs->fs_super)->s_clustersize_bits;
	if (ret > UINT32_MAX)
		ret = UINT32_MAX;

	return (uint32_t)ret;
}

static inline void cmfs_set_rec_blocks(cmfs_filesys *fs,
				        uint16_t tree_depth,
					struct cmfs_extent_rec *rec,
					uint32_t blocks)
{
	if (tree_depth)
		rec->e_int_blocks = blocks;
	else
		rec->e_leaf_blocks = blocks;
}















#endif
