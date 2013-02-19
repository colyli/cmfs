/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * dump.h
 *
 * Function prototypes, macros, etc. for related 'C' files
 *
 * Copyright (C) 2004,2009 Oracle.  All rights reserved.
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

#ifndef __DUMP_H__
#define __DUMP_H__

struct list_dir_opts {
	cmfs_filesys *fs;
	FILE *out;
	int long_opt;
	char *buf;
};

struct dirblocks_walk {
	cmfs_filesys *fs;
	FILE *out;
	struct cmfs_dinode *di;
	char *buf;
};

void dump_super_block (FILE *out, struct cmfs_super_block *sb);
void dump_local_alloc (FILE *out, struct cmfs_local_alloc *loc);
void dump_truncate_log (FILE *out, struct cmfs_truncate_log *tl);
void dump_inode (FILE *out, struct cmfs_dinode *in);
void dump_extent_list (FILE *out, struct cmfs_extent_list *ext);
void dump_chain_list (FILE *out, struct cmfs_chain_list *cl);
void dump_extent_block (FILE *out, struct cmfs_extent_block *blk);
void dump_group_descriptor (FILE *out, struct cmfs_group_desc *grp, int index);
int  dump_dir_entry (struct cmfs_dir_entry *rec, uint64_t blocknr, int offset, int blocksize,
		     char *buf, void *priv_data);
void dump_dir_block(FILE *out, char *buf);
void dump_jbd_header (FILE *out, journal_header_t *header);
void dump_jbd_superblock (FILE *out, journal_superblock_t *jsb);
void dump_jbd_block (FILE *out, journal_superblock_t *jsb,
		     journal_header_t *header, uint64_t blknum);
void dump_jbd_metadata (FILE *out, enum cmfs_block_type type, char *buf,
			uint64_t blknum);
void dump_jbd_unknown (FILE *out, uint64_t start, uint64_t end);
void dump_fast_symlink (FILE *out, char *link);
void dump_inode_path (FILE *out, uint64_t blkno, char *path);
void dump_logical_blkno(FILE *out, uint64_t blkno);
void dump_icheck(FILE *out, int hdr, uint64_t blkno, uint64_t inode,
		 int validoffset, uint64_t offset, int status);
void dump_block_check(FILE *out, struct cmfs_block_check *bc, void *block);
void dump_frag(FILE *out, uint64_t ino, uint32_t clusters,
	       uint32_t extents);
#endif		/* __DUMP_H__ */
