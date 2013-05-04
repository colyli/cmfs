/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * lookup.c
 *
 * Directory lookup routines for the CMFS userspace library.
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
 *  This code is a port of e2fsprogs/lib/ext2fs/lookup.c
 *  Copyright (C) 1993, 1994, 1994, 1995 Theodore Ts'o.
 */

#define _XOPEN_SOURCE 600 /* Triggers magic in features.h */
#define _LARGEFILE64_SOURCE

#include <com_err.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include <cmfs/cmfs.h>
#include "cmfs_err.h"

struct lookup_struct  {
	const char	*name;
	int		len;
	uint64_t	*inode;
	int		found;
};	

static int lookup_proc(struct cmfs_dir_entry *dirent,
		       uint64_t blocknr,
		       int offset,
		       int blocksize,
		       char *buf,
		       void *priv_data)
{
	struct lookup_struct *ls = (struct lookup_struct *)priv_data;

	if (ls->len != (dirent->name_len & 0xff))
		return 0;
	if (strncmp(ls->name, dirent->name, (dirent->name_len & 0xff)))
		return 0;
	*ls->inode = dirent->inode;
	ls->found++;
	return CMFS_DIRENT_ABORT;
}

errcode_t cmfs_lookup(cmfs_filesys *fs,
		      uint64_t dir,
		      const char *name,
		      int namelen,
		      char *buf,
		      uint64_t *inode)
{
	errcode_t ret;
	struct lookup_struct ls;
	char *di_buf = NULL;

	ls.name = name;
	ls.len = namelen;
	ls.inode = inode;
	ls.found = 0;

	ret = cmfs_malloc_block(fs->fs_io, &di_buf);
	if (ret)
		goto out;
	ret = cmfs_read_inode(fs, dir, di_buf);
	if (ret)
		goto out;

	ret = cmfs_dir_iterate(fs,
			       dir,
			       0,
			       buf,
			       lookup_proc,
			       &ls);
	if (ret)
		goto out;

	ret = (ls.found) ? 0 : CMFS_ET_FILE_NOT_FOUND;
out:
	if (di_buf)
		cmfs_free(&di_buf);

	return ret;
}


