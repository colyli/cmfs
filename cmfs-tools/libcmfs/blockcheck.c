/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * blockcheck.c
 *
 * Checksum and ECC codes for the OCFS2 userspace library.
 *
 * Copyright (C) 2006, 2008 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License, version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 *   The 802.3 CRC32 algorithm is copied from the Linux kernel, lib/crc32.c.
 *   Code was from the public domain, is now GPL, so no real copyright
 *   attribution other than "The Linux Kernel".  XXX: better text, anyone?
 */

#define _XOPEN_SOURCE 600 /* Triggers magic in features.h */
#define _LARGEFILE64_SOURCE

#ifdef DEBUG_EXE
# define _BSD_SOURCE  /* For timersub() */
#endif

#include <inttypes.h>

#include <cmfs/cmfs.h>


/* XXX: not implemented yet */
errcode_t cmfs_validate_meta_ecc(cmfs_filesys *fs,
				 void *data,
				 struct cmfs_block_check *bc)
{
	return 0;
};
