/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * bitmap.h
 * (copied and modified from libocfs2/bitmap.h)
 *
 * Structures for allocation bitmaps for the CMFS userspace library.
 *
 * Copyright (C) 2004 Oracle.  All rights reserved.
 * CMFS modification, Copyright 2012, Coly Li <i@coly.li>
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
 * Authors: Joel Becker
 * CMFS modification: Coly Li <i@coly.li>
 */

#ifndef _BITMAP_H
#define _BITMAP_H

#include <cmfs/kernel-rbtree.h>
#include <et/com_err.h>
#include <unistd.h>
#include <cmfs/cmfs.h>

struct cmfs_bitmap_region {
	struct rb_node br_node;
	uint64_t br_start_bit;
	int br_bitmap_start;
	int br_valid_bits;
	int br_total_bits;

	size_t br_bytes;
	int br_set_bits;
	uint8_t *br_bitmap;
	void *br_private;
};

struct cmfs_bitmap_operations {
	errcode_t (*set_bit)(cmfs_bitmap *bitmap, uint64_t bit, int *oldval);
	errcode_t (*clear_bit)(cmfs_bitmap *bitmap, uint64_t bit, int *oldval);
	errcode_t (*test_bit)(cmfs_bitmap *bitmap, uint64_t bit, int *val);
	errcode_t (*find_next_set)(cmfs_bitmap *bitmap, uint64_t start, uint64_t *found);
	errcode_t (*find_next_clear)(cmfs_bitmap *bitmap, uint64_t start, uint64_t *found);
	int (*merge_region)(cmfs_bitmap *bitmap, struct cmfs_bitmap_region *prev, struct cmfs_bitmap_region *next);
	errcode_t (*read_bitmap)(cmfs_bitmap *bitmap);
	errcode_t (*write_bitmap)(cmfs_bitmap *bitmap);
	void (*destroy_notify)(cmfs_bitmap *bitmap);
	void (*bit_change_notify)(cmfs_bitmap *bitmap, struct cmfs_bitmap_region *br, uint64_t bitno, int new_val);
	errcode_t (*alloc_range)(cmfs_bitmap *bitmap, uint64_t min_len, uint64_t len, uint64_t *first_bit, uint64_t *bits_found);
	errcode_t (*clear_range)(cmfs_bitmap *bitmap, uint64_t len, uint64_t first_bit);
};


struct _cmfs_bitmap {
	cmfs_filesys *b_fs;
	uint64_t b_set_bits;
	uint64_t b_total_bits;
	char *b_description;
	struct cmfs_bitmap_operations *b_ops;
	struct rb_root b_regions;
	void *b_private;	
};


#endif
