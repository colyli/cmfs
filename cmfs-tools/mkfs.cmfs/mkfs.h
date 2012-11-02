/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * mkfs2.h
 *
 * Definitions, function prototypes, etc.
 *
 * Copyright (C) 2004, 2005 Oracle.All rights reserved.
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
 *
 */

#define _LARGEFILE64_SOURCE

typedef struct _State State;

struct _State {
	char *program;

	int verbose;
	int quiet;
	int force;
	int prompt;
	int mount;
	int no_backup_super;

	uint32_t blocksize;
	uint32_t blocksize_bits;

	uint32_t cluster_size;
	uint32_t cluster_size_bits;

	uint64_t specified_size_in_blocks;
	uint64_t volume_size_in_bytes;
	uint64_t volume_size_in_clusters;
	uint64_t volume_size_in_blocks;

	uint32_t pagesize_bits;

	uint64_t reserved_tail_size;

	uint64_t journal_size_in_bytes;

	uint32_t extent_alloc_size_in_clusters;

	char *vol_label;
	char *device_name;
	unsigned char uuid[CMFS_VOL_UUID_LEN];
	uint32_t vol_generation;

	int fd;

	time_t format_time;

	uint32_t nr_cluster_groups;
	uint16_t tail_group_bits;
	uint32_t first_cluster_group;
	uint64_t first_cluster_group_blkno;

	cmfs_fs_options feature_flags;
};
