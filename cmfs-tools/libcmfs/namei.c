/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * namei.c
 *
 * CMFS directory lookup operations
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
 * This code is a port of e2fsprogs/lib/ext2fs/namei.c
 * Copyright (C) 1993, 1994, 1994, 1995 Theodore Ts'o.
 */

#define _XOPEN_SOURCE 600 /* Triggers magic in features.h */
#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <string.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cmfs/cmfs.h>


static errcode_t follow_link(cmfs_filesys *fs,
			     uint64_t root,
			     uint64_t dir,
			     uint64_t inode,
			     int link_count,
			     char *buf,
			     uint64_t *res_inode)
{
	return -1;
}


/*
 * This routine interprets a pathname in the context of the
 * currnet directory and the root directory, and returns
 * the inode of the containing directory, and a pointer to
 * the filename of the file (pointing into the pathname)
 * and the length of the filename.
 */
static errcode_t dir_namei(cmfs_filesys *fs,
			   uint64_t root,
			   uint64_t dir,
			   const char *pathname,
			   int pathlen,
			   int link_count,
			   char *buf,
			   const char **name,
			   int *namelen,
			   uint64_t *res_inode)
{
	char c;
	const char *thisname;
	int len;
	uint64_t inode;
	errcode_t ret;

	if ((c = *pathname) == '/') {
		dir = root;
		pathname++;
		pathlen--;
	}

	while (1) {
		thisname = pathname;
		for (len = 0; --pathlen >= 0; len++) {
			c = *(pathname++);
			if (c == '/')
				break;
		}

		if (pathlen < 0)
			break;

		ret = cmfs_lookup(fs,
				  dir,
				  thisname,
				  len,
				  buf,
				  &inode);
		if (ret)
			return ret;

		ret = follow_link(fs,
				  root,
				  dir,
				  inode,
				  link_count,
				  buf,
				  &dir);
		if (ret)
			return ret;
	}

	*name = thisname;
	*namelen = len;
	*res_inode = dir;
	return 0;
}

static errcode_t open_namei(cmfs_filesys *fs,
			    uint64_t root,
			    uint64_t base,
			    const char *pathname,
			    size_t pathlen,
			    int follow,
			    int link_count,
			    char *buf,
			    uint64_t *res_inode)
{
	const char *basename;
	int namelen;
	uint64_t dir, inode;
	errcode_t ret;

#ifdef NAMIE_DEBUG
	printf("%s:%d root=%lu, dir=%lu, path=%*s, lc=%d\n",
		__func__, __LINE__,
		root, base, pathlen, pathname, link_count);
#endif
	ret = dir_namei(fs,
			root,
			base,
			pathname,
			pathlen,
			link_count,
			buf,
			&basename,
			&namelen,
			&dir);

	if (ret)
		return ret;

	/* special case: '/usr/' etc */
	if (!namelen) {
		*res_inode = dir;
		return 0;
	}

	ret = cmfs_lookup(fs,
			  dir,
			  basename,
			  namelen,
			  buf,
			  &inode);
	if (ret)
		return ret;

	if (follow) {
		ret = follow_link(fs,
				  root,
				  dir,
				  inode,
				  link_count,
				  buf,
				  &inode);
		if (ret)
			return ret;
	}
#ifdef NAMEI_DEBUG
	printf("%s:%d (link_count=%d) returns %lu\n",
		link_count, inode);
#endif
	*res_inode = inode;
	return 0;
}

errcode_t cmfs_namei(cmfs_filesys *fs,
		     uint64_t root,
		     uint64_t cwd,
		     const char *name,
		     uint64_t *inode)
{
	char *buf;
	errcode_t ret;

	ret = cmfs_malloc_block(fs->fs_io, &buf);
	if (ret)
		goto out;

	ret = open_namei(fs, root, cwd, name, strlen(name), 0, 0, buf, inode);

	cmfs_free(&buf);
out:
	return ret;
}
