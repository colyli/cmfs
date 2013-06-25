/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * extent_map.c
 *
 * In-memory extent map for the OCFS2 userspace library.
 * (Copied and modified from ocfs2-toos/libocfs2/extent_map.c)
 *
 * Copyright (C) 2004 Oracle.  All rights reserved.
 * CMFS modification by Coly Li <i@coly.li>
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

#define _XOPEN_SOURCE 600 /* Triggers magic in features.h */
#define _LARGEFILE64_SOURCE

#include <assert.h>

#include <cmfs/cmfs.h>
#include <cmfs-kernel/cmfs_fs.h>

#include "cmfs_err.h"

/*
 * Return the 1st index within el which contains an extent
 * start larger then v_cluster
 */
static int cmfs_search_for_hole_index(struct cmfs_extent_list *el,
				      uint64_t v_cluster)
{
	int i;
	struct cmfs_extent_rec *rec;

	for (i = 0; i < el->l_next_free_rec; i++) {
		rec = &el->l_recs[i];
		if (v_cluster < rec->e_cpos)
			break;
	}
	return i;
}

/*
 * Figure out the size of a hole which starts at v_cluster within the
 * given extent list.
 *
 * If there is no more allocation past v_cluster, we return the maximum
 * cluster size minus v_cluster.
 *
 * If we have in-inode extents, then el points to the dinode list and
 * eb_buf is NULL. Otherwise, eb_buf should point to the extent block
 * containing el.
 */
static int cmfs_figure_hole_clusters(cmfs_cached_inode *cinode,
				     struct cmfs_extent_list *el,
				     char *eb_buf,
				     uint64_t v_cluster,
				     uint64_t *num_clusters)
{
	int ret, i;
	char *next_eb_buf = NULL;
	struct cmfs_extent_block *eb, *next_eb;

	i = cmfs_search_for_hole_index(el, v_cluster);

	if (i == el->l_next_free_rec && eb_buf) {
		eb = (struct cmfs_extent_block *)eb_buf;

		/*
		 * Check the next leaf for any extents
		 */
		if (eb->h_next_leaf_block == 0)
			goto no_more_extents;

		ret = cmfs_malloc_block(cinode->ci_fs->fs_io,
					&next_eb_buf);
		if (ret)
			goto out;

		ret = cmfs_read_extent_block(cinode->ci_fs,
					     eb->h_next_leaf_block,
					     next_eb_buf);
		if (ret)
			goto out;

		next_eb = (struct cmfs_extent_block *)next_eb_buf;
		el = &next_eb->h_list;

		i = cmfs_search_for_hole_index(el, v_cluster);
		if (i > 0) {
			if ((i > 1) || cmfs_rec_clusters(cinode->ci_fs,
							 el->l_tree_depth,
							 &el->l_recs[0])) {
				ret = CMFS_ET_CORRUPT_EXTENT_BLOCK;
				goto out;
			}
		}

	}

no_more_extents:
	if (i == el->l_next_free_rec) {
		/*
		 * we're at the end of our existing allocation.
		 * Just return the maximum number of clusters we
		 * could possibly allocate.
		 */
		*num_clusters = UINT64_MAX - v_cluster;
	} else {
		*num_clusters = el->l_recs[i].e_cpos - v_cluster;
	}

	ret = 0;
out:
	if (next_eb_buf)
		cmfs_free(&next_eb_buf);
	return ret;
}

errcode_t cmfs_get_clusters(cmfs_cached_inode *cinode,
			    uint64_t v_cluster,
			    uint64_t *p_cluster,
			    uint64_t *num_clusters,
			    uint16_t *extent_flags)
{
	int i;
	uint16_t flags = 0;
	errcode_t ret = 0;
	cmfs_filesys *fs = cinode->ci_fs;
	struct cmfs_dinode *di;
	struct cmfs_extent_block *eb;
	struct cmfs_extent_list *el;
	struct cmfs_extent_rec *rec;
	char *eb_buf = NULL;
	uint32_t coff;

	di = cinode->ci_inode;
	el = &di->id2.i_list;

	if (el->l_tree_depth) {
		ret = cmfs_find_leaf(fs, di, v_cluster, &eb_buf);
		if (ret)
			goto out;

		eb = (struct cmfs_extent_block *)eb_buf;
		el = &eb->h_list;

		if (el->l_tree_depth) {
			ret = CMFS_ET_CORRUPT_EXTENT_BLOCK;
			goto out;
		}
	}

	i = cmfs_search_extent_list(fs, el, v_cluster);
	if (i == -1) {
		/*
		 * A hole was found. Return some canned values that
		 * callers can key on. If asked for, num_clusters will
		 * be populated with the size of the hole.
		 */
		*p_cluster = 0;
		if (num_clusters) {
			ret = cmfs_figure_hole_clusters(cinode,
							el,
							eb_buf,
							v_cluster,
							num_clusters);
			if (ret)
				goto out;
		}
	} else {
		rec = &el->l_recs[i];
		assert(v_cluster >= rec->e_cpos);

		if (!rec->e_blkno) {
			ret = CMFS_ET_BAD_BLKNO;
			goto out;
		}

		coff = v_cluster - rec->e_cpos;

		*p_cluster = cmfs_blocks_to_clusters(fs, rec->e_blkno);
		*p_cluster = *p_cluster + coff;

		if (num_clusters)
			*num_clusters =
				cmfs_rec_clusters(fs, el->l_tree_depth, rec) -
				coff;
		flags = rec->e_flags;
	}
	if (extent_flags)
		*extent_flags = flags;

out:
	if (eb_buf)
		cmfs_free(&eb_buf);
	return ret;
}

errcode_t cmfs_extent_map_get_blocks(cmfs_cached_inode *cinode,
				     uint64_t v_blkno,
				     int count,
				     uint64_t *p_blkno,
				     uint64_t *ret_count,
				     uint16_t *extent_flags)
{
	errcode_t ret;
	int bpc;
	uint64_t cpos, num_clusters = -1, p_cluster = -1;
	uint64_t boff = 0;
	cmfs_filesys *fs = cinode->ci_fs;

	bpc = cmfs_clusters_to_blocks(fs, 1);
	cpos = cmfs_blocks_to_clusters(fs, v_blkno);

	ret = cmfs_get_clusters(cinode,
				cpos,
				&p_cluster,
				&num_clusters,
				extent_flags);
	if (ret)
		goto out;

	/*
	 * p_cluster == 0 indicates a hole
	 */
	if (p_cluster) {
		boff = cmfs_clusters_to_blocks(fs, p_cluster);
		boff += (v_blkno & (uint64_t)(bpc - 1));
	}

	*p_blkno = boff;

	if (ret_count) {
		*ret_count = cmfs_clusters_to_blocks(fs, num_clusters);
		*ret_count -= v_blkno & (uint64_t)(bpc - 1);
	}

out:
	return ret;
}

