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

#include "cmfs_err.h"

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

static void cmfs_swap_extent_block_header(struct cmfs_extent_block *eb)
{
	eb->h_suballoc_slot	= bswap_16(eb->h_suballoc_slot);
	eb->h_suballoc_bit	= bswap_16(eb->h_suballoc_bit);
	eb->h_fs_generation	= bswap_32(eb->h_fs_generation);
	eb->h_blkno		= bswap_64(eb->h_blkno);
	eb->h_next_leaf_block	= bswap_64(eb->h_next_leaf_block);
}

void cmfs_swap_extent_block_to_cpu(cmfs_filesys *fs,
				   struct cmfs_extent_block *eb)
{
	if (cpu_is_little_endian)
		return;

	cmfs_swap_extent_block_header(eb);
	cmfs_swap_extent_list_to_cpu(fs, eb, &eb->h_list);
}

void cmfs_swap_extent_block_from_cpu(cmfs_filesys *fs,
				     struct cmfs_extent_block *eb)
{
	if (cpu_is_little_endian)
		return;

	cmfs_swap_extent_block_header(eb);
	cmfs_swap_extent_list_from_cpu(fs, eb, &eb->h_list);
}

errcode_t cmfs_read_extent_block_nocheck(cmfs_filesys *fs,
				 uint64_t blkno,
				 char *eb_buf)
{
	errcode_t ret;
	char *blk;
	struct cmfs_extent_block *eb;

	if ((blkno < CMFS_SUPER_BLOCK_BLKNO) ||
	    (blkno > fs->fs_blocks))
		return CMFS_ET_BAD_BLKNO;

	ret = cmfs_malloc_block(fs->fs_io, &blk);
	if (ret)
		return ret;

	ret = cmfs_read_blocks(fs, blkno, 1, blk);
	if (ret)
		goto out;

	eb = (struct cmfs_extent_block *)blk;
	if (memcmp(eb->h_signature, CMFS_EXTENT_BLOCK_SIGNATURE,
		   strlen(CMFS_EXTENT_BLOCK_SIGNATURE))) {
		ret = CMFS_ET_BAD_EXTENT_BLOCK_MAGIC;
		goto out;
	}

	memcpy(eb_buf, blk, fs->fs_blocksize);

	eb = (struct cmfs_extent_block *)eb_buf;
	cmfs_swap_extent_block_to_cpu(fs, eb);

out:
	cmfs_free(&blk);
	return ret;
}

errcode_t cmfs_read_extent_block(cmfs_filesys *fs,
				 uint64_t blkno,
				 char *eb_buf)
{
	errcode_t ret;
	struct cmfs_extent_block *eb =
		(struct cmfs_extent_block *)eb_buf;

	ret = cmfs_read_extent_block_nocheck(fs, blkno, eb_buf);

	if (ret == 0 && eb->h_list.l_next_free_rec > eb->h_list.l_count)
		ret = CMFS_ET_CORRUPT_EXTENT_BLOCK;

	return ret;
}

errcode_t cmfs_write_extent_block(cmfs_filesys *fs,
				  uint64_t blkno,
				  char *eb_buf)
{
	errcode_t ret;
	char *blk;
	struct cmfs_extent_block *eb;

	if (!(fs->fs_flags & CMFS_FLAG_RW))
		return CMFS_ET_RO_FILESYS;

	if ((blkno < CMFS_SUPER_BLOCK_BLKNO) ||
	    (blkno > fs->fs_blocks))
		return CMFS_ET_BAD_BLKNO;

	ret = cmfs_malloc_block(fs->fs_io, &blk);
	if (ret)
		return ret;

	memcpy(blk, eb_buf, fs->fs_blocksize);

	eb = (struct cmfs_extent_block *)blk;
	cmfs_swap_extent_block_from_cpu(fs, eb);

	ret = io_write_block(fs->fs_io, blkno, 1, blk);
	if (ret)
		goto out;
	fs->fs_flags |= CMFS_FLAG_CHANGED;
	ret = 0;
out:
	cmfs_free(&blk);

	return ret;
}

struct extent_context {
	cmfs_filesys *fs;
	int (*func)(cmfs_filesys *fs,
		    struct cmfs_extent_rec *rec,
		    int tree_depth,
		    uint32_t ccount,
		    uint64_t ref_blkno,
		    int ref_recno,
		    void *priv_data);
	uint32_t ccount;
	int flags;
	errcode_t errcode;
	char **eb_bufs;
	void *priv_data;
	uint64_t last_eb_blkno;
	uint64_t last_eb_cpos;
};

static int extent_iterate_eb(struct cmfs_extent_rec *eb_rec,
			     int tree_depth,
			     uint64_t ref_blkno,
			     int ref_recno,
			     struct extent_context *ctxt);

/* XXX not implemented yet */
static int update_leaf_rec(struct extent_context *ctxt,
			   struct cmfs_extent_rec *before,
			   struct cmfs_extent_rec *current)
{
	return 0;
}

static int update_eb_rec(struct extent_context *ctxt,
			 struct cmfs_extent_rec *before,
			 struct cmfs_extent_rec *current)
{
	return 0;
}

static int extent_iterate_el(struct cmfs_extent_list *el,
			     uint64_t ref_blkno,
			     struct extent_context *ctxt)
{
	struct cmfs_extent_rec before;
	int iret = 0;
	int i;

	for (i = 0; i < el->l_next_free_rec; i++) {
		before = el->l_recs[i];

		if (el->l_tree_depth) {
			iret |= extent_iterate_eb(&el->l_recs[i],
						  el->l_tree_depth,
						  ref_blkno,
						  i,
						  ctxt);
			if (iret & CMFS_EXTENT_CHANGED)
				iret |= update_eb_rec(ctxt,
						      &before,
						      &el->l_recs[i]);

			if (el->l_recs[i].e_int_blocks &&
			    el->l_recs[i].e_cpos >= ctxt->last_eb_cpos) {
				/* XXX why?
				 * Only set last_eb_blkno if current extent
				 * list point to leaf blocks
				 */
				if (el->l_tree_depth == 1)
					ctxt->last_eb_blkno =
						el->l_recs[i].e_blkno;
				ctxt->last_eb_cpos = el->l_recs[i].e_cpos;
			}

		} else {
			/* XXX why ? 
			 * For a sparse file, we may find an empty record
			 * in the left most record, just skip it.
			 */
			if (!i && !el->l_recs[i].e_leaf_blocks)
				continue;
			iret |= (*ctxt->func)(ctxt->fs,
					      &el->l_recs[i],
					      el->l_tree_depth,
					      ctxt->ccount,
					      ref_blkno,
					      i,
					      ctxt->priv_data);
			if (iret & CMFS_EXTENT_CHANGED)
				iret |= update_leaf_rec(ctxt,
							&before,
							&el->l_recs[i]);
			ctxt->ccount += cmfs_rec_clusters(ctxt->fs,
							  el->l_tree_depth,
							  &el->l_recs[i]);
		}

		if (iret & (CMFS_EXTENT_ABORT | CMFS_EXTENT_ERROR))
			break;
	}

	if (iret & CMFS_EXTENT_CHANGED) {
		for (i = 0; i < el->l_count; i++) {
			if (cmfs_rec_clusters(ctxt->fs,
					      el->l_tree_depth,
					      &el->l_recs[i]))
				continue;
			/* XXX how about a hole in file ? */
			el->l_next_free_rec = i;
			break;
		}
	}

	return iret;
}

static int extent_iterate_eb(struct cmfs_extent_rec *eb_rec,
			     int ref_tree_depth,
			     uint64_t ref_blkno,
			     int ref_recno,
			     struct extent_context *ctxt)
{
	int iret = 0, changed = 0, flags;
	int tree_depth = ref_tree_depth - 1;
	struct cmfs_extent_block *eb;
	struct cmfs_extent_list *el;

	if (!(ctxt->flags & CMFS_EXTENT_FLAG_DEPTH_TRAVERSE) &&
	    !(ctxt->flags & CMFS_EXTENT_FLAG_DATA_ONLY))
		iret = (ctxt->func)(ctxt->fs,
				    eb_rec,
				    ref_tree_depth,
				    ctxt->ccount,
				    ref_blkno,
				    ref_recno,
				    ctxt->priv_data);
	if (!eb_rec->e_blkno || (iret & CMFS_EXTENT_ABORT))
		goto out;
	if ((eb_rec->e_blkno < CMFS_SUPER_BLOCK_BLKNO) ||
	    (eb_rec->e_blkno > ctxt->fs->fs_blocks)) {
		ctxt->errcode = CMFS_ET_BAD_BLKNO;
		iret |= CMFS_EXTENT_ERROR;
		goto out;
	}

	ctxt->errcode = cmfs_read_extent_block(ctxt->fs,
					       eb_rec->e_blkno,
					       ctxt->eb_bufs[tree_depth]);
	if (ctxt->errcode) {
		iret |= CMFS_EXTENT_ERROR;
		goto out;
	}

	eb = (struct cmfs_extent_block *)ctxt->eb_bufs[tree_depth];
	el = &eb->h_list;

	if ((el->l_tree_depth != tree_depth) ||
	    (eb->h_blkno != eb_rec->e_blkno)) {
		ctxt->errcode = CMFS_ET_CORRUPT_EXTENT_BLOCK;
		iret |= CMFS_EXTENT_ERROR;
		goto out;
	}

	flags = extent_iterate_el(el, eb_rec->e_blkno, ctxt);
	changed |= flags;
	if (flags & (CMFS_EXTENT_ABORT | CMFS_EXTENT_ERROR))
		iret |= flags & (CMFS_EXTENT_ABORT | CMFS_EXTENT_ERROR);

	/*
	 * If the list was changed, we should write the changes to disk.
	 * Note: For a sparse file, we may have an empty extent block.
	 */
	if (changed & CMFS_EXTENT_CHANGED) {
		ctxt->errcode =
			cmfs_write_extent_block(ctxt->fs,
						eb_rec->e_blkno,
						ctxt->eb_bufs[tree_depth]);
		if (ctxt->errcode) {
			iret |= CMFS_EXTENT_ERROR;
			goto out;
		}
	}

	if ((ctxt->flags & CMFS_EXTENT_FLAG_DEPTH_TRAVERSE) &&
	    !(ctxt->flags & CMFS_EXTENT_FLAG_DATA_ONLY) &&
	    !(iret & (CMFS_EXTENT_ABORT | CMFS_EXTENT_ERROR)))
		iret = ctxt->func(ctxt->fs,
				  eb_rec,
				  ref_tree_depth,
				  ctxt->ccount,
				  ref_blkno,
				  ref_recno,
				  ctxt->priv_data);
out:
	return iret;
}

errcode_t cmfs_extent_iterate_inode(cmfs_filesys *fs,
				    struct cmfs_dinode *inode,
				    int flags,
				    char *block_buf,
				    int (*func)(cmfs_filesys *fs,
					        struct cmfs_extent_rec *rec,
						int tree_depth,
						uint32_t ccount,
						uint64_t ref_blkno,
						int ref_recno,
						void *priv_data),
				    void *priv_data)
{
	int i;
	int iret = 0;
	struct cmfs_extent_list *el;
	errcode_t ret;
	struct extent_context ctxt;

	ret = CMFS_ET_INODE_NOT_VALID;
	if (!(inode->i_flags & CMFS_VALID_FL))
		goto out;

	ret = CMFS_ET_INODE_CANNOT_BE_ITERATED;
	if (inode->i_flags & (CMFS_SUPER_BLOCK_FL |
			      CMFS_LOCAL_ALLOC_FL |
			      CMFS_CHAIN_FL))
		goto out;

	if (inode->i_dyn_features & CMFS_INLINE_DATA_FL)
		goto out;

	el = &inode->id2.i_list;
	if (el->l_tree_depth) {
		ret = cmfs_malloc0(sizeof(char *) * el->l_tree_depth,
				   &ctxt.eb_bufs);
		if (ret)
			goto out;

		if (block_buf) {
			ctxt.eb_bufs[0] = block_buf;
		} else {
			ret = cmfs_malloc0(fs->fs_blocksize * el->l_tree_depth,
					   &ctxt.eb_bufs[0]);
			if (ret)
				goto out_eb_bufs;
		}

		for (i = 1; i < el->l_tree_depth; i++)
			ctxt.eb_bufs[i] = ctxt.eb_bufs[0] +
					  i * fs->fs_blocksize;

	} else {
		ctxt.eb_bufs = NULL;
	}

	ctxt.fs = fs;
	ctxt.func = func;
	ctxt.priv_data = priv_data;
	ctxt.flags = flags;
	ctxt.ccount = 0;
	ctxt.last_eb_blkno = 0;
	ctxt.last_eb_cpos = 0;

	ret = 0;
	iret |= extent_iterate_el(el, 0, &ctxt);
	if (iret & CMFS_EXTENT_ERROR)
		ret = ctxt.errcode;

	if (iret & CMFS_EXTENT_ABORT)
		goto out_abort;

	/* we can only trust ctxt.last_eb_blkno if we walked the whole tree */
	if (inode->i_last_eb_blk != ctxt.last_eb_blkno) {
		inode->i_last_eb_blk = ctxt.last_eb_blkno;
		iret |= CMFS_EXTENT_CHANGED;
	}

out_abort:
	if (!ret && (iret & CMFS_EXTENT_CHANGED))
		ret = cmfs_write_inode(fs, inode->i_blkno, (char *)inode);
out_eb_bufs:
	if (ctxt.eb_bufs) {
		if (!block_buf && ctxt.eb_bufs[0])
			cmfs_free(&ctxt.eb_bufs[0]);
		cmfs_free(&ctxt.eb_bufs);
	}
	
out:
	return ret;
}

struct block_context {
	int (*func)(cmfs_filesys *fs,
		    uint64_t blkno,
		    uint64_t bcount,
		    uint16_t ext_flags,
		    void *priv_data);
	int flags;
	struct cmfs_dinode *inode;
	errcode_t errcode;
	void *priv_data;
};

static int block_iterate_func(cmfs_filesys *fs,
			      struct cmfs_extent_rec *rec,
			      int tree_depth,
			      uint32_t ccount,
			      uint64_t ref_blkno,
			      int ref_recno,
			      void *priv_data)
{
	struct block_context *ctxt = priv_data;
	uint64_t blkno, bcount, bend;
	int iret = 0;

	bcount = cmfs_clusters_to_blocks(fs, rec->e_cpos);
	bend = bcount + cmfs_clusters_to_blocks(
			fs, cmfs_rec_clusters(fs, tree_depth, rec));

	for (blkno = rec->e_blkno; bcount < bend; blkno ++, bcount ++) {
		if (((bcount * fs->fs_blocksize) >= ctxt->inode->i_size) &&
		    !(ctxt->flags & CMFS_BLOCK_FLAG_APPEND))
			break;
		iret = (ctxt->func)(fs,
				    blkno,
				    bcount,
				    rec->e_flags,
				    ctxt->priv_data);
		if (iret & CMFS_BLOCK_ABORT)
			break;
	}

	return iret;
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
	errcode_t ret;
	struct block_context ctxt;

	ctxt.inode = inode;
	ctxt.flags = flags;
	ctxt.func = func;
	ctxt.errcode = 0;
	ctxt.priv_data = priv_data;

	ret = cmfs_extent_iterate_inode(fs,
					inode,
					CMFS_EXTENT_FLAG_DATA_ONLY,
					NULL,
					block_iterate_func,
					&ctxt);

	return ret;
}

errcode_t cmfs_block_iterate(cmfs_filesys *fs,
			     uint64_t blkno,
			     int flags,
			     int (*func)(cmfs_filesys *fs,
				         uint64_t blkno,
					 uint64_t bcount,
					 uint16_t ext_flags,
					 void *priv_data),
			     void *priv_data)
{
	struct cmfs_dinode *inode;
	errcode_t ret;
	char *buf;

	ret = cmfs_malloc_block(fs->fs_io, &buf);
	if (ret)
		return ret;

	ret = cmfs_read_inode(fs, blkno, buf);
	if (ret)
		goto out_buf;

	inode = (struct cmfs_dinode *)buf;
	ret = cmfs_block_iterate_inode(fs, inode, flags, func, priv_data);

out_buf:
	cmfs_free(&buf);
	return ret;
}































































