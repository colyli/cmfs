/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * find_block_inode.c
 *
 * Take a block number and returns the owning inode number.
 *
 * Copyright (C) 2006 Oracle.  All rights reserved.
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

#include "main.h"

extern struct dbgfs_gbls gbls;

struct block_array {
	uint64_t blkno;
	uint32_t inode;		/* Backing inode# */
	uint64_t offset;
	int data;		/* offset is valid if this is set */
#define STATUS_UNKNOWN	0
#define STATUS_USED	1
#define STATUS_FREE	2
	int status;
};

errcode_t find_block_inode(cmfs_filesys *fs,
			   uint64_t *blkno,
			   int count,
			   FILE *out)
{
	return -1;
}
