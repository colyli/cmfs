/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * dumpcmfs.h
 * (Copied and modified from ocfs2-tools/o2info/o2info.h)
 *
 * dumpcmfs operation prototypes.
 *
 * Copyright (C) 2010 Oracle.  All rights reserved.
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

#ifndef __O2INFO_H__
#define __O2INFO_H__

#include <getopt.h>
#include <linux/limits.h>


#include <cmfs/cmfs.h>
#include <cmfs-kernel/cmfs_fs.h>
#include <cmfs-kernel/kernel-list.h>


struct dumpcmfs_fs_features {
	uint32_t compat;
	uint32_t incompat;
	uint32_t rocompat;
};

struct dumpcmfs_volinfo {
	uint32_t blocksize;
	uint32_t clustersize;
	uint32_t maxslots;
	uint8_t label[CMFS_MAX_VOL_LABEL_LEN];
	uint8_t uuid_str[CMFS_TEXT_UUID_LEN + 1];
	struct dumpcmfs_fs_features cfs;
};

struct dumpcmfs_mkfs {
	struct dumpcmfs_volinfo cvf;
	uint64_t journal_size;
};

struct dumpcmfs_local_freeinode {
	unsigned long total;
	unsigned long free;
};

struct dumpcmfs_freeinode {
	int slotnum;
	struct dumpcmfs_local_freeinode fi[CMFS_MAX_SLOTS];
};

struct free_chunk_histogram {
	uint32_t fc_chunks[CMFS_INFO_MAX_HIST];
	uint32_t fc_clusters[CMFS_INFO_MAX_HIST];
};

struct dumpcmfs_freefrag {
	unsigned long chunkbytes;
	uint32_t clusters;
	uint32_t total_chunks;
	uint32_t free_chunks;
	uint32_t free_chunks_real;
	int clustersize_bits;
	int blksize_bits;
	int chunkbits;
	uint32_t clusters_in_chunk;
	uint32_t chunks_in_group;
	uint32_t min, max, avg;	/* chunksize in clusters */
	struct free_chunk_histogram histogram;
};

struct dumpcmfs_fiemap {
	uint32_t blocksize;
	uint32_t clustersize;
	uint32_t num_extents;
	uint32_t num_extents_xattr;
	uint32_t clusters;
	uint32_t shared;
	uint32_t holes;
	uint32_t unwrittens;
	uint32_t xattr;
	float frag;	/*  extents/clusters ratio */
	float score;
};

enum dumpcmfs_method_type {
	DUMPCMFS_USE_LIBCMFS = 1,
	DUMPCMFS_USE_IOCTL,
	DUMPCMFS_USE_NUMTYPES
};

struct dumpcmfs_method {
	enum dumpcmfs_method_type dm_method;
	char dm_path[PATH_MAX];
	union {
		cmfs_filesys *dm_fs;
		int dm_fd;
	};
};

struct dumpcmfs_operation {
	char *to_name;
	int (*to_run)(struct dumpcmfs_operation *op,
		      struct dumpcmfs_method *dm,
		      void *arg);
	void *to_private;
};

struct dumpcmfs_option {
	/*
	 * For getopt_long().
	 * If there is not short option, set .val to CHAR_MAX.
	 * A unique value will be inserted by the code.
	 */
	struct option opt_option;
	struct dumpcmfs_operation *opt_op;
	char *opt_help;	/* Help string */
	int opt_set;	/* Was this option seen */
	int (*opt_handler)(struct dumpcmfs_option *opt, char *arg);
	void *opt_private;
};

struct dumpcmfs_op_task {
	struct list_head dumpcmfs_list;
	struct dumpcmfs_operation *dumpcmfs_task;
};

#define __DUMPCMFS_OP(_name, _run, _private)	\
{						\
	.to_name	= #_name,		\
	.to_run		= _run,			\
	.to_private	= _to_private		\
}

#define DEFINE_DUMPCMFS_OP(_name, _run, _private)	\
struct dumpcmfs_operation _name##_op =			\
	__DUMPCMFS_OP(_name, _run, _private)


#endif
