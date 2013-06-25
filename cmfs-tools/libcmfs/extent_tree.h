/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * extent_tree.h
 *
 * (copied and modified from ocfs2-tools/libocfs2/extent_tree.h)
 *
 * Copyright (C) 2009 Oracle.  All rights reserved.
 * CMFS modification, by Coly Li <i@coly.li>
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
 * Structures which describe a path though a btree, and functions to
 * manipulate them.
 *
 * This idea here is to be as generic as possible with the tree
 * manipulation code.
 */
struct cmfs_path_item {
	uint64_t blkno;
	char *buf;
	struct cmfs_extent_list *el;
};

#define CMFS_MAX_PATH_DEPTH	5

struct cmfs_path {
	int p_tree_depth;
	struct  cmfs_path_item p_node[CMFS_MAX_PATH_DEPTH];
};


#define path_root_blkno(_path)	((_path)->p_node[0].blkno)
#define path_root_buf(_path)	((_path)->p_node[0].buf)
#define path_root_el(_path)	((_path)->p_node[0].el)
#define path_leaf_blkno(_path)	((_path)->p_node[(_path)->p_tree_depth].bkno)
#define path_leaf_buf(_path)	((_path)->p_node[(_path)->p_tree_depth].buf)
#define path_leaf_el(_path)	((_path)->p_node[(_path)->p_tree_depth].el)
#define path_num_items(_path)	((_path)->p_tree_depth + 1)
