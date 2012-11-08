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

#include <cmfs/cmfs.h>
#include <unistd.h>
#include <sys/types.h>

#define CMFS_DFL_MAX_MNT_COUNT		48
#define CMFS_DFL_CHECKINTERVAL		0

#define CMFS_OS_LINUX			0
#define CMFS_OS_HURD			1
#define CMFS_OS_FREEBSD			2
#define CMFS_OS_MINIX			3
#define CMFS_OS_MLXOS			4





/*
 * backup superblock flag is used to indicate that this volume
 * has backup superblocks
 */
#define CMFS_FEATURE_COMPAT_BACKUP_SB		0x0001




/* Journal limits (in bytes) */
#define CMFS_MIN_JOURNAL_SIZE	(4 * (1<<20))

#define SYSTEM_FILE_NAME_MAX	40
enum {
	SFI_JOURNAL,
	SFI_CLUSTER,
	SFI_LOCAL_ALLOC,
	SFI_CHAIN,
	SFI_TRUNCATE_LOG,
	SFI_OTHER
};

typedef struct _SystemFileInfo SystemFileInfo;

struct _SystemFileInfo {
	char *name;
	int type;
	int global;
	int mode;
};

typedef struct _State State;
typedef struct _AllocGroup AllocGroup;
typedef struct _SystemFileDiskRecord SystemFileDiskRecord;
typedef struct _DirData DirData;
typedef struct _AllocBitmap AllocBitmap;


struct _AllocBitmap {
	AllocGroup **groups;

	uint32_t valid_bits;
	uint32_t unit;
	uint32_t unit_bits;

	char *name;

	uint64_t fe_disk_off;

	SystemFileDiskRecord *bm_record;
	SystemFileDiskRecord *alloc_record;
	int num_chains;
};

struct _State {
	char *progname;

	int verbose;
	int quiet;
	int force;
	int prompt;
	int mount;
	int no_backup_super;
	int inline_data;
	int dx_dirs;
	int dry_run;

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

	AllocBitmap *global_bm;
	AllocGroup *system_group;

	uint32_t nr_cluster_groups;
	uint16_t global_cpg;
	uint16_t tail_group_bits;
	uint32_t first_cluster_group;
	uint64_t first_cluster_group_blkno;

	cmfs_fs_options feature_flags;
};

struct _AllocGroup {
	char *name;
	struct cmfs_group_desc *gd;
	SystemFileDiskRecord *alloc_inode;
	uint32_t chain_free;
	uint32_t chain_total;
	struct _AllocGroup *next;
};

struct BitInfo {
	uint32_t used_bits;
	uint32_t total_bits;
};

struct _SystemFileDiskRecord {
	uint64_t fe_off;
	uint16_t suballoc_bit;
	uint64_t extent_off;
	uint64_t extent_len;
	uint64_t file_size;

	uint64_t chain_off;
	AllocGroup *group;

	struct BitInfo bi;
	struct _AllocBitmap *bitmap;

	int flags;
	int links;
	int mode;

	/* record the dir entry so that inline dir can be sotred with file */
	DirData *dir_data;
};

struct _DirData {
	void *buf;
	int last_off;

	SystemFileDiskRecord *record;
};
