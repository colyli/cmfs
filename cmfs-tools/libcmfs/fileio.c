/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * fileio.c
 *
 * I/O to files.  Part of the OCFS2 userspace library.
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
 * Ideas taken from e2fsprogs/lib/ext2fs/fileio.c
 *   Copyright (C) 1997 Theodore Ts'o.
 */

#define _XOPEN_SOURCE 600  /* Triggers XOPEN2K in features.h */
#define _LARGEFILE64_SOURCE

#include <string.h>
#include <limits.h>
#include <inttypes.h>

#include <cmfs/cmfs.h>
#include "cmfs_err.h"

struct read_whole_context {
	char		*buf;
	char		*ptr;
	int		size;
	int		offset;
	errcode_t	errcode;
};

errcode_t cmfs_file_read(cmfs_cached_inode *ci,
			 void *buf,
			 uint32_t count,
			 uint64_t offset,
			 uint32_t *got)
{
	cmfs_filesys *fs = ci->ci_fs;
	errcode_t ret = 0;
	char *ptr = buf;
	uint32_t wanted_blocks;
	uint64_t contig_blocks;
	uint64_t v_blkno;
	uint64_t p_blkno;
	uint32_t tmp;
	uint64_t num_blocks;
	uint16_t extent_flags;

	/*
	 * o_direct requires aligned io, here we use
	 * blocksize bytes alignment
	 */
	tmp = fs->fs_blocksize -1;
	if ((count & tmp) || (offset & (uint64_t)tmp) ||
	    ((unsigned long)ptr & tmp))
		return CMFS_ET_INVALID_ARGUMENT;

	wanted_blocks = count >> CMFS_RAW_SB(fs->fs_super)->s_blocksize_bits;
	v_blkno = offset >> CMFS_RAW_SB(fs->fs_super)->s_blocksize_bits;
	*got = 0;

	num_blocks = (ci->ci_inode->i_size + fs->fs_blocksize - 1) >>
			CMFS_RAW_SB(fs->fs_super)->s_blocksize_bits;

	if (v_blkno >= num_blocks)
		return 0;

	if (v_blkno + wanted_blocks > num_blocks)
		wanted_blocks = (uint32_t)(num_blocks - v_blkno);

	while(wanted_blocks) {
		ret = cmfs_extent_map_get_blocks(ci,
						 v_blkno,
						 1,
						 &p_blkno,
						 &contig_blocks,
						 &extent_flags);
		if (ret)
			return ret;

		if (contig_blocks > wanted_blocks)
			contig_blocks = wanted_blocks;

		if (!p_blkno || extent_flags & CMFS_EXT_UNWRITTEN) {
			/*
			 * A hole or unwritten extent met,
			 * so just empty the content
			 */
			memset(ptr, 0, contig_blocks * fs->fs_blocksize);
		} else {
			ret = cmfs_read_blocks(fs,
					       p_blkno,
					       contig_blocks,
					       ptr);
			if (ret)
				return ret;
		}

		*got += (contig_blocks <<
			 CMFS_RAW_SB(fs->fs_super)->s_blocksize_bits);
		wanted_blocks -= contig_blocks;

		if (wanted_blocks) {
			ptr += (contig_blocks <<
				(CMFS_RAW_SB(fs->fs_super)->s_blocksize_bits));
			v_blkno += (uint64_t)contig_blocks;
		} else {
			if (*got + offset > ci->ci_inode->i_size)
				*got = (uint32_t)
					(ci->ci_inode->i_size - offset);
			/* quit while() */
		}
	};

	return ret;
}
