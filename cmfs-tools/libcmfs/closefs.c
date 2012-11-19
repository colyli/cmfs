/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * closefs.c
 *
 * Close an CMFS filesystem.  Part of the CMFS userspace library.
 * (Copied and modified from ocfs2-tools/libocfs2/closefs.c)
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
 * Ideas taken from e2fsprogs/lib/ext2fs/closefs.c
 *   Copyright (C) 1993, 1994, 1995, 1996 Theodore Ts'o.
 */

#define _XOPEN_SOURCE 600  /* Triggers XOPEN2K in features.h */
#define _LARGEFILE64_SOURCE

#include <cmfs/cmfs.h>

errcode_t cmfs_flush(cmfs_filesys *fs)
{
	/* XXX: not implemented yet */
	return 0;
}


errcode_t cmfs_close(cmfs_filesys *fs)
{
	errcode_t ret;

	if (fs->fs_flags & CMFS_FLAG_DIRTY) {
		ret = cmfs_flush(fs);
		if (ret)
			return ret;
	}

	cmfs_freefs(fs);
	return 0;
}
