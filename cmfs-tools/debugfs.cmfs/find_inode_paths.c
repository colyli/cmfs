/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * find_inode_paths.c
 *
 * Takes an inode block number and find all paths leading to it.
 *
 * Copyright (C) 2004, 2011 Oracle.  All rights reserved.
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
 *  This code is a port of e2fsprogs/lib/ext2fs/dir_iterate.c
 *  Copyright (C) 1993, 1994, 1994, 1995, 1996, 1997 Theodore Ts'o.
 */

#include "main.h"

struct walk_path {
	char *argv0;
	FILE *out;
	cmfs_filesys *fs;
	char *path;
	uint32_t found;
	uint32_t count;
	int findall;
	uint64_t *inode;
};

errcode_t find_inode_paths(cmfs_filesys *fs,
			   char **args,
			   int findall,
			   uint32_t count,
			   uint64_t *blknos,
			   FILE *out)
{
	return -1;
}
