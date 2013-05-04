/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * dir_iterate.c
 *
 * Directory block routines for the OCFS2 userspace library.
 * (Copied and modified from ocfs2-tools/libocfs2/dir_iterate.c)
 *
 * Copyright (C) 2004 Oracle.  All rights reserved.
 * CMFS modification, by Coly Li <i@coly.li>
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

#define _XOPEN_SOURCE 600 /* Triggers magic in features.h */
#define _LARGEFILE64_SOURCE

#include <inttypes.h>
#include <com_err.h>
#include <string.h>

#include <cmfs/cmfs.h>
#include "cmfs_err.h"
#include "dir_iterate.h"

/*
 * This function checks to see whether or not a potential deleted
 * directory entry looks valid. What we do is check the deleted
 * entry and each successive entry to make sure that they all look
 * valid and that the last deleted etnry ends at the beginning of
 * the next undeleted entry. Returns 1 if the deleted entry looks
 * valid, zero if not valid.
 */
static int cmfs_validate_entry(char *buf,
			       int offset,
			       int final_offset)
{
	struct cmfs_dir_entry *dirent;

	while (offset < final_offset) {
		dirent = (struct cmfs_dir_entry *)(buf + offset);
		offset += dirent->rec_len;
		if ((dirent->rec_len < 8) ||
		    ((dirent->rec_len % 4) != 0) ||
		    (((dirent->name_len & 0xFF)+8) > dirent->rec_len))
			return 0;
	}
	return (offset == final_offset);
}

static int cmfs_process_dir_entry(cmfs_filesys *fs,
				  uint64_t blocknr,
				  unsigned int offset,
				  int entry,
				  int *changed,
				  int *do_abort,
				  struct dir_context *ctx)
{
	errcode_t ret;
	struct cmfs_dir_entry *dirent;
	unsigned int next_real_entry = 0;
	int size;

	while (offset < fs->fs_blocksize) {
		dirent = (struct cmfs_dir_entry *)(ctx->buf + offset);
		if (((offset + dirent->rec_len) > fs->fs_blocksize) ||
		    (dirent->rec_len < 8) ||
		    ((dirent->rec_len %4) != 0) ||
		    (((dirent->name_len & 0xFF)+8) > dirent->rec_len)) {
			ctx->errcode = CMFS_ET_DIR_CORRUPTED;
			return CMFS_BLOCK_ABORT;
		}

		if (!dirent->inode &&
		    !(ctx->flags & CMFS_DIRENT_FLAG_INCLUDE_EMPTY))
			goto next;
		else if ((ctx->flags & CMFS_DIRENT_FLAG_EXCLUDE_DOTS) &&
			 is_dots(dirent->name, dirent->name_len))
			goto next;

		ret = (ctx->func)(ctx->dir,
				  (next_real_entry > offset) ?
				  CMFS_DIRENT_DELETED_FILE : entry,
				  dirent,
				  blocknr,
				  offset,
				  fs->fs_blocksize,
				  ctx->buf,
				  ctx->priv_data);
		if (entry < CMFS_DIRENT_OTHER_FILE)
			entry ++;

		if (ret & CMFS_DIRENT_CHANGED)
			(*changed) += 1;
		if (ret & CMFS_DIRENT_ABORT) {
			(*do_abort) += 1;
			break;
		}
next:
		if (next_real_entry == offset)
			next_real_entry += dirent->rec_len;

		if (ctx->flags & CMFS_DIRENT_FLAG_INCLUDE_REMOVED) {
			size = ((dirent->name_len & 0xFF) + 11) & (~3);
			if (dirent->rec_len != size) {
				unsigned int final_offset;
				final_offset = offset + dirent->rec_len;
				offset += size;
				while ((offset < final_offset) &&
					!cmfs_validate_entry(ctx->buf,
							  offset,
							  final_offset))
					offset += 4;
				continue;
			}
		}
		offset += dirent->rec_len;
	}

	return 0;
}

int cmfs_process_dir_block(cmfs_filesys *fs,
			   uint64_t blocknr,
			   uint64_t blockcnt,
			   uint16_t ext_flags,
			   void *priv_data)
{
	struct dir_context *ctx = (struct dir_context *)priv_data;
	unsigned int offset = 0;
	int ret = 0;
	int changed = 0;
	int do_abort = 0;
	int entry;

	if (blockcnt < 0)
		return 0;

	entry = blockcnt ? CMFS_DIRENT_OTHER_FILE : CMFS_DIRENT_DOT_FILE;

	ctx->errcode = cmfs_read_dir_block(fs, ctx->di, blocknr, ctx->buf);
	if (ctx->errcode)
		return CMFS_BLOCK_ABORT;

	ret = cmfs_process_dir_entry(fs,
				     blocknr,
				     offset,
				     entry,
				     &changed,
				     &do_abort,
				     ctx);
	if (ret)
		return ret;

	if (changed) {
		ctx->errcode = cmfs_write_dir_block(fs,
						    ctx->di,
						    blocknr,
						    ctx->buf);
		if (ctx->errcode)
			return CMFS_BLOCK_ABORT;
	}
	if (do_abort)
		return CMFS_BLOCK_ABORT;
	return 0;
}

errcode_t cmfs_dir_iterate2(cmfs_filesys *fs,
			    uint64_t dir,
			    int flags,
			    char *block_buf,
			    int (*func)(uint64_t dir,
				        int entry,
					struct cmfs_dir_entry *dirent,
					uint64_t blocknr,
					int offset,
					int blocksieze,
					char *buf,
					void *priv_data),
			    void *priv_data)
{
	struct dir_context ctx;
	errcode_t ret;

	ret = cmfs_check_directory(fs, dir);
	if (ret)
		return ret;

	ctx.dir = dir;
	ctx.flags = flags;
	if (block_buf)
		ctx.buf = block_buf;
	else {
		ret = cmfs_malloc_block(fs->fs_io, &ctx.buf);
		if (ret)
			return ret;
	}

	ctx.func = func;
	ctx.priv_data = priv_data;
	ctx.errcode = 0;

	ret = cmfs_malloc_block(fs->fs_io, &ctx.di);
	if (ret)
		goto out;


	ret = cmfs_read_inode(fs, dir, ctx.buf);
	if (ret)
		goto out;

	/*
	 * Save off the inode - some paths use the buffer
	 * for dirent data.
	 */
	memcpy(ctx.di, ctx.buf, fs->fs_blocksize);

	ret = cmfs_block_iterate(fs, dir, 0,
				 cmfs_process_dir_block,
				 &ctx);

out:
	if (!block_buf)
		cmfs_free(&ctx.buf);
	if (ctx.di)
		cmfs_free(&ctx.di);
	if (ret)
		return ret;
	return ctx.errcode;
}

struct xlate {
	int (*func)(struct cmfs_dir_entry *dirent,
		    uint64_t blocknr,
		    int offset,
		    int blocksize,
		    char *buf,
		    void *priv_data);
	void *real_private;
};

static int xlate_func(uint64_t dir,
		int entry,
		struct cmfs_dir_entry *dirent,
		uint64_t blocknr,
		int offset,
		int blocksize,
		char *buf,
		void *priv_data)
{
	struct xlate *xl = (struct xlate *) priv_data;
	return (*xl->func)(dirent, blocknr, offset, blocksize, buf, xl->real_private);

}

errcode_t cmfs_dir_iterate(cmfs_filesys *fs,
			   uint64_t dir,
			   int flags,
			   char *block_buf,
			   int (*func)(struct cmfs_dir_entry *dirent,
				       uint64_t blocknr,
				       int offset,
				       int blocksize,
				       char *buf,
				       void *prov_data),
			   void *priv_data)
{
	struct xlate xl;

	xl.real_private = priv_data;
	xl.func = func;

	return cmfs_dir_iterate2(fs,
				 dir,
				 flags,
				 block_buf,
				 xlate_func,
				 &xl);
}

