/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * cmfs_fs.h
 * (copied and modified from ocfs2_fs.h from ocfs2-tools)
 *
 * On-disk structures for CMFS.
 *
 * Copyright (C) 2002, 2004 Oracle.All rights reserved.
 * CMFS modification, Copyright (C) 2012, Coly Li <i@coly.li>
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License, version 2,as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#ifndef _CMFS_FS_H
#define _CMFS_FS_H


#define CMFS_VOL_UUID_LEN		16
//#define CMFS_MAX_VOL_LABEL_LEN	64


#define CMFS_MIN_CLUSTERSIZE	4096
#define CMFS_MAX_CLUSTERSIZE	(32*(1<<20))
#define CMFS_MAX_BLOCKSIZE	CMFS_MIN_CLUSTERSIZE
#define CMFS_MIN_BLOCKSIZE	CMFS_MAX_BLOCKSIZE

#endif
