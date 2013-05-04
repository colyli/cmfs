/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * dir_iterate.h
 *
 * Structures for dir iteration for the CMFS userspace library.
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
 * Authors: Joel Becker
 */

#ifndef _DIR_ITERATE_H
#define _DIR_ITERATE_H

struct dir_context {
	uint64_t dir;
	int flags;
	struct cmfs_dinode *di;
	char *buf;
	int (*func)(uint64_t dir,
		    int entry,
		    struct cmfs_dir_entry *dirent,
		    uint64_t blocknr,
		    int offset,
		    int blocksize,
		    char *buf,
		    void *priv_data);
	void *priv_data;
	errcode_t errcode;
};

extern int cmfs_process_dir_block(cmfs_filesys *fs,
				   uint64_t	blocknr,
				   uint64_t	blockcnt,
				   uint16_t	ext_flags,
				   void		*priv_data);

#define CMFS_DIR_PAD                   4
#define CMFS_DIR_ROUND                 (CMFS_DIR_PAD - 1)
#define CMFS_DIR_MEMBER_LEN            offsetof(struct cmfs_dir_entry, name)
#define CMFS_DIR_REC_LEN(name_len)     (((name_len) + CMFS_DIR_MEMBER_LEN + \
                                          CMFS_DIR_ROUND) & \
                                         ~CMFS_DIR_ROUND)

#endif  /* _DIR_ITERATE_H */
