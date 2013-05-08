/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * bitmap.c
 *
 * Basic bitmap routines for the OCFS2 userspace library.
 * (Copied and modified from ocfs2-tools/libocfs2/bitmap.c)
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

#include <cmfs/cmfs.h>
#include "cmfs_err.h"
#include "bitmap.h"

void cmfs_bitmap_free_region(struct cmfs_bitmap_region *br)
{
	if (br->br_bitmap)
		cmfs_free(&br->br_bitmap);
	cmfs_free(&br);
}

/* The public API */
void cmfs_bitmap_free(cmfs_bitmap *bitmap)
{
	struct rb_node *node;
	struct cmfs_bitmap_region *br;

	/*
	 * If the bitmap needs to do extra cleanup of region,
	 * it should have done it in destroy_notify. Same with
	 * the private pointers.
	 */
	if (bitmap->b_ops->destroy_notify)
		bitmap->b_ops->destroy_notify(bitmap);

	while((node = rb_first(&bitmap->b_regions)) != NULL) {
		br = rb_entry(node, struct cmfs_bitmap_region, br_node);
		rb_erase(&br->br_node, &bitmap->b_regions);
		cmfs_bitmap_free_region(br);
	}

	cmfs_free(&bitmap->b_description);
	cmfs_free(&bitmap);
}
