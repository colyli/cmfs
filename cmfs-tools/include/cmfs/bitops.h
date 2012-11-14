/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * bitops.h
 *
 * Bitmap frobbing routines for the CMFS userspace library.
 *
 * (copied and modified from ocfs2-tools/include/ocfs2/bitops.h)
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
 * CMFS modification: Coly Li <i@coly.li>
 *
 *  This code is a port of e2fsprogs/lib/ext2fs/bitops.h
 *  Copyright (C) 1993, 1994, 1995, 1996 Theodore Ts'o.
 */

#ifndef _BITOPS_H
#define _BITOPS_H

extern int cmfs_set_bit(int nr,void * addr);
extern int cmfs_clear_bit(int nr, void * addr);
extern int cmfs_test_bit(int nr, const void * addr);

extern int cmfs_find_first_bit_set(void *addr, int size);
extern int cmfs_find_first_bit_clear(void *addr, int size);
extern int cmfs_find_next_bit_set(void *addr, int size, int offset);
extern int cmfs_find_next_bit_clear(void *addr, int size, int offset);
extern int cmfs_get_bits_set(void *addr, int size, int offset);

#endif
