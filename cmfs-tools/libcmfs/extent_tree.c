/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * extent_tree.c
 * (Copied and modified from ocfs2-tools/libocfs2/extent_tree.c)
 *
 * Copyright (C) 2009 Oracle.  All rights reserved.
 * CMFS modification by Coly Li, i@coly.li
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

/*
 * Traverse a btree path in search of cpos, starting at root_el.
 *
 * This code can be called with a cpos larger than the tree, in
 * which case it will return the rightmost path.
 */

#include <et/com_err.h>
#include <assert.h>
#include <string.h>

#include <cmfs/cmfs.h>
#include <cmfs-kernel/cmfs_fs.h>

#include "cmfs_err.h"
#include "extent_tree.h"

typedef errcode_t (path_insert_t)(void *, char *);

static errcode_t __cmfs_find_path(cmfs_filesys *fs,
				  struct cmfs_extent_list *root_el,
				  uint64_t cpos,
				  path_insert_t *func,
				  void *data)
{
	int i, ret = 0;
	uint32_t range;
	uint64_t blkno;
	char *buf = NULL;
	struct cmfs_extent_block *eb;
	struct cmfs_extent_list *el;
	struct cmfs_extent_rec *rec;

	el = root_el;
	while (el->l_tree_depth) {
		if (el->l_next_free_rec == 0) {
			ret = CMFS_ET_CORRUPT_EXTENT_BLOCK;
			goto out;
		}

		for (i = 0; i < el->l_next_free_rec - 1; i++) {
			rec = &el->l_recs[i];

			/*
			 * In the case that cpos is off the allocation
			 * tree, this should just wind up returning the
			 * rgiht most record
			 */
			range = rec->e_cpos +
				cmfs_rec_clusters(fs, el->l_tree_depth, rec);
			if (cpos >= rec->e_cpos && cpos < range)
				break;
		}

		blkno = el->l_recs[i].e_blkno;
		if (blkno == 0) {
			ret = CMFS_ET_CORRUPT_EXTENT_BLOCK;
			goto out;
		}

		ret = cmfs_malloc_block(fs->fs_io, &buf);
		if (ret)
			return ret;

		ret = cmfs_read_extent_block(fs, blkno, buf);
		if (ret)
			goto out;

		eb = (struct cmfs_extent_block *)buf;
		el = &eb->h_list;

		if (el->l_next_free_rec > el->l_count) {
			ret = CMFS_ET_CORRUPT_EXTENT_BLOCK;
			goto out;
		}

		/*
		 * The user's callback must give us the tip for how
		 * to handle the buf we allocated by return value.
		 *
		 * 1) return '0':
		 *    the function succeeds, and it will use the buf
		 *    and take care of the buf release.
		 *
		 * 2) return > 0:
		 *    the function succeeds, and there is no need fo buf,
		 *    so we will release it.
		 *
		 *
		 */
		if (func) {
			ret = func(data, buf);

			if (ret == 0) {
				buf = NULL;
				continue;
			} 
			
			if (ret < 0)
				goto out;
		}
		cmfs_free(&buf);
		buf = NULL;
	}

out:
	if (buf)
		cmfs_free(&buf);
	return ret;
}

/*
 * Insert an extent block at given index.
 *
 * Note:
 * This buf will be inserted into the path, so the caller shouldn't free it.
 */
static inline void cmfs_path_insert_eb(struct cmfs_path *path,
				       int index,
				       char *buf)
{
	struct cmfs_extent_block *eb = (struct cmfs_extent_block *)buf;
	/*
	 * Right now, no root buf is an extent block, so this helps
	 * catch code errors with dinode trees. The assertion can
	 * be safely removed if we ever need to insert extent block
	 * structs at the root.
	 */
	assert(index);

	path->p_node[index].blkno = eb->h_blkno;
	path->p_node[index].buf = (char *)buf;
	path->p_node[index].el = &eb->h_list;
}

/*
 * Given an initialized path (that is, it has a valid root extent
 * list), this function will traverse the btree in search of the
 * path which would contain cpos.
 *
 * The path traveled is recorded in the path structure.
 *
 * Note that this will not do any comparisons on leaf node extent
 * records, so it will work find in the case that we just added a
 * tree branch.
 */
struct find_path_data{
	int index;
	struct cmfs_path *path;
};

static errcode_t find_path_ins(void *data, char *eb)
{
	struct find_path_data *fp = data;

	cmfs_path_insert_eb(fp->path, fp->index, eb);
	fp->index ++;

	return 0;
}

int cmfs_find_path(cmfs_filesys *fs,
		   struct cmfs_path *path,
		   uint64_t cpos)
{
	struct find_path_data data;

	data.index = 1;
	data.path = path;
	return __cmfs_find_path(fs,
				path_root_el(path),
				cpos,
				find_path_ins,
				&data);
}

static struct cmfs_path *cmfs_new_path(char *buf,
				       struct cmfs_extent_list *root_el,
				       uint64_t blkno)
{
	struct cmfs_path *path = NULL;
	assert(root_el->l_tree_depth < CMFS_MAX_PATH_DEPTH);

	cmfs_malloc0(sizeof(*path), &path);
	if (path) {
		path->p_tree_depth = root_el->l_tree_depth;
		path->p_node[0].blkno = blkno;
		path->p_node[0].buf = buf;
		path->p_node[0].el = root_el;
	}

	return path;
}

/*
 * Reset the actual path elements so that we can re-use the structure
 * to build another path. Generally this involves freeing the buffer
 * heads.
 */
static void cmfs_reinit_path(struct cmfs_path *path,
			     int keep_root)
{
	int i, start = 0, depth = 0;
	struct cmfs_path_item *node;

	if (keep_root)
		start = 1;

	for (i = start; i < path_num_items(path); i++) {
		node = &path->p_node[i];
		if (!node->buf)
			continue;

		cmfs_free(&(node->buf));
		node->blkno = 0;
		node->buf = NULL;
		node->el = NULL;
	}

	/*
	 * Tree depth may change during truncate, or insert. If
	 * we're keeping the root extent list, then make sure that
	 * our path structure reflects to proper depth
	 */
	if (keep_root)
		depth = path_root_el(path)->l_tree_depth;

	path->p_tree_depth = depth;
}

void cmfs_free_path(struct cmfs_path *path)
{
	/*
	 * We don't free the root because often in libcmfs the root
	 * is a shared buffer such as the inode. Caller must be
	 * responsible for handling the root of the path.
	 */
	if (path) {
		cmfs_reinit_path(path, 1);
		cmfs_free(&path);
	}
}

/*
 * Find the leaf block in the tree which would contain cpos.
 * No checking of the actual leaf is done.
 */
int cmfs_tree_find_leaf(cmfs_filesys *fs,
			struct cmfs_extent_list *el,
			uint64_t el_blkno,
			char *el_blk,
			uint64_t cpos,
			char **leaf_buf)
{
	int ret;
	char *buf = NULL;
	struct cmfs_path *path = NULL;

	assert(el->l_tree_depth > 0);

	path = cmfs_new_path(el_blk, el, el_blkno);
	if (!path) {
		ret = CMFS_ET_NO_MEMORY;
		goto out;
	}

	ret = cmfs_find_path(fs, path, cpos);
	if (ret)
		goto out;

	ret = cmfs_malloc_block(fs->fs_io, &buf);
	if (ret)
		goto out;

	memcpy(buf, path_leaf_buf(path), fs->fs_blocksize);
	*leaf_buf = buf;
out:
	cmfs_free_path(path);
	return ret;
}

int cmfs_find_leaf(cmfs_filesys *fs,
		   struct cmfs_dinode *di,
		   uint64_t cpos,
		   char **leaf_buf)
{
	return cmfs_tree_find_leaf(fs,
				   &di->id2.i_list,
		       		   di->i_blkno,
				   (char *)di,
				   cpos,
				   leaf_buf);
}

/*
 * Return the index of the extent record which contains cluster #v_cluster,
 * -1 is returned if it was not found.
 *
 * should work fine on interior and exterior nodes.
 */
int cmfs_search_extent_list(cmfs_filesys *fs,
			    struct cmfs_extent_list *el,
			    uint64_t v_cluster)
{
	int ret = -1;
	int i;
	struct cmfs_extent_rec *rec;
	uint64_t rec_end, rec_start, clusters;

	for (i = 0; i < el->l_next_free_rec; i++) {
		rec = &el->l_recs[i];

		rec_start = rec->e_cpos;
		clusters = cmfs_rec_clusters(fs, el->l_tree_depth, rec);
		rec_end = rec_start + clusters;

		if ((v_cluster >= rec_start) && (v_cluster < rec_end)) {
			ret = i;
			break;
		}
	}

	return ret;
}
