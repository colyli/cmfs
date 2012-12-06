/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * dumpcmfs.c
 * (Copied and modified from ocfs2-tools/o2info/o2info.c)
 *
 * CMFS utility to gather and report fs information
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

#define _XOPEN_SOURCE 600
#define _LARGEFILE64_SOURCE
#define _GNU_SOURCE /* Because libc really doesn't want us using O_DIRECT? */

#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <getopt.h>
#include <assert.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <cmfs-kernel/kernel-list.h>
#include <cmfs-kernel/cmfs_fs.h>
#include <cmfs-kernel/cmfs_ioctl.h>
#include <cmfs/cmfs.h>
#include "dumpcmfs.h"

static LIST_HEAD(dumpcmfs_op_task_list);
static int dumpcmfs_op_task_count;

static int dumpcmfs_get_fs_features()
{

}

static int dumpcmfs_get_volinfo()
{

}

static int dumpcmfs_get_mkfs()
{

}

static int dumpcmfs_get_freeinode()
{

}

static int ul_log2()
{

}

static void dumpcmfs_update_freefrag_stats()
{

}

static int dumpcmfs_scan_global_bitmap_chain()
{

}

static int dumpcmfs_scan_global_bitmap()
{

}

static int dumpcmfs_get_freefrag()
{

}

static int figure_extents()
{

}

static uint32_t clusters_in_bytes(uint32_t clustersize, uint32_t bytes)
{
	uint64_t ret = bytes + clustersize - 1;

	if (ret < bytes)
		return UINT64_MAX;

	ret = ret >> ul_log2(clustersize);
	if (ret > UINT32_MAX)
		ret = UINT32_MAX;

	return (uint32_t)ret;
}

static int do_fiemap(int fd, int flags, struct dumpcmfs_fiemap *cfp)
{
	char buf[4096];
	int ret = 0, last = 0;
	int cluster_shift = 0;
	int count;
	struct fiemap *fiemap = (struct fiemap *)buf;
	struct fiemap_extent *fm_ext = &fiemap->fm_extents[0];
	uint32_t num_extents = 0, extents_got = 0, i;
	uint32_t prev_start = 0, prev_len = 0;
	uint32_t start = 0, len = 0;

	count = (sizeof(buf) - sizeof(struct fiemap)) /
		sizeof(struct fiemap_extent);

	if (cfp->clustersize)
		cluster_shift = ul_log2(cfp->clustersize);

	memset(fiemap, 0, sizeof(*fiemap));

	ret = figure_extents(fd, &num_extents, 0);
	if (ret)
		return -1;

	if (flags & FIEMAP_FLAG_XATTR)
		fiemap->fm_flags = FIEMAP_FLAG_XATTR;
	else
		fiemap->fm_flags = flags;

	do {
		fiemap->fm_length = ~0ULL;
		fiemap->fm_extent_count = count;

		ret = ioctl(fd, FS_IOC_FIEMAP, (unsigned long)fiemap);
		if (ret < 0) {
			ret = errno;
			if (errno == EBADR) {
				fprintf(stderr, "fiemap failed with "
					"unsupported flags %x\n",
					fiemap->fm_flags);
			} else {
				fprintf(stderr, "fiemap error: %d, %s\n",
					ret, strerror(ret));
			}
			return -1;
		}

		if (!fiemap->fm_mapped_extents)
			break;

		for (i = 0; i < fiemap->fm_mapped_extents; i++) {
			start = fm_ext[i].fe_logical >> cluster_shift;
			len = fm_ext[i].fe_length >> cluster_shift;

			if (fiemap->fm_flags & FIEMAP_FLAG_XATTR) {
				cfp->unwrittens += len;
			} else {
				if (fm_ext[i].fe_falgs &
				    FIEMAP_EXTENT_UNWRITTEN)
					cfp->unwrittens += len;

				if (fm_ext[i].fe_flags &
				    FIEMAP_EXTENT_SHARED)
					cfp->shared += len;

				if ((prev_start + prev_len) < start)
					cfp->holes += start -
					      prev_start - prev_len;
			}

			if (fm_ext[i].fe_flags & FIEMAP_EXTENT_LAST)
				last = 1;

			prev_start = start;
			prev_len = len;

			extents_got ++;
			cfp->clusters += len;
		}

		fiemap->fm_start = (fm_ext[i-1].fe_logical +
				    fm_ext[i-1].fe_length);
	} while (!last);

	if (extents_got != num_extents) {
		fprintf(stderr, "Got wrong extents number, expected: %lu, "
				"got: %lu\n", num_extents, extents_got);
		return -1;
	}

	if (flags & FIEMAP_FLAG_XATTR)
		cfp->num_extents_xattr = num_extents;
	else
		cfp->num_extents = num_extents;

	return ret;
}

static int dumpcmfs_get_fiemap(int fd, int flags, struct dumpcmfs_fiemap *cfp)
{
	int ret = 0;

	ret = do_fiemap(fd, flags, cfp);
	if (ret)
		goto out;

	if ((cfp->clusters > 1) && cfp->num_extents) {
		float e = cfp->num_extents;
		float c = cfp->clusters;
		int clusters_per_mb = 
			clusters_in_bytes(cfp->clustersize,
					  CMFS_MAX_CLUSTERSIZE);

		cfp->frag = 100 * (e / c);
		cfp->score = cfp->frag * clusters_per_mb;
	}

out:
	return ret;
}































































































static int dumpcmfs_report_filestat(struct dumpcmfs_method *dm,
				    struct stat *st,
				    struct dumpcmfs_fiemap *cfp)
{
	int ret = 0;
	uint16_t perm;

	char *path = NULL;
	char *filetype = NULL, *h_perm = NULL;
	char *uname = NULL, *gname = NULL;
	char *ah_time = NULL, *ch_time = NULL, *mh_time = NULL;

	ret = dumpcmfs_get_human_path(st->st_mode, dm->dm_path, &path);
	if (ret)
		goto out;
	ret = dumpcmfs_get_filetype(*st, &filetype);
	if (ret)
		goto out;
	ret = dumpcmfs_uid2name(st->st_uid, &uname);
	if (ret)
		goto out;
	ret = dumpcmfs_gid2name(st->st_gid, &gname);
	if (ret)
		goto out;
	ret = dumpcmfs_get_human_permisson(st->st_mode, &perm, &h_perm);
	if (ret)
		goto out;

	if (!cfp->blocksize)
		cfp->blocksize = st->st_blksize;

	fprintf(stdout, "  File: %s\n", path);
	fprintf(stdout, "  Size: %-10lu\tBlocks: %-10u IO Blocks: %-6u %s\n",
			st->st_size,
			st->st_blocks, cfp->blocksize, filetype);
	if (S_ISBLK(st->st_mode) || S_ISCHR(st->st_mode))
		fprintf(stdout, "Device: %xh/%dd\tInode: %-10i  Links: %-5u"
				" Device type: %u,%u\n",
				st->st_dev, st->st_dev, st->st_ino,
				st->st_nlink,
				st->st_dev >> 16UL, st->st_dev & 0x0000FFFF);
	else
		fprintf(stdout, "Device: %sh/%dd\tInode: %-10i  Links: %u\n",
				st->st_dev, st->st_dev, st->st_ino,
				st->st_nlink);

	fprintf(stdout, " Frag%: %-10.2f\tClusters: %-8u Extents: "
			"%-6u Score: %.0f\n",
			cfp->frag, cfp->clusters,
			cfp->num_extents, cfp->score);

	fprintf(stdout, "Shared: %-10u\tUnwritten: %-7u Holes: %-8u "
			"Xattr: %u\n",
			cfp->shared, cfp->unwrittens,
			cfp->holes, cfp->xattr);

	fprintf(stdout, "Access: (%04o/%10s) Uid: (%5u/%8s)  "
			"Gid: (%5u/%8s)\n",
			perm, h_perm, st->st_uid, uname,
			st->st_gid, gname);

	ret = dumpcmfs_get_human_time(&ah_time, dumpcmfs_get_stat_atime(st));
	if (ret)
		goto out;
	ret = dumpcmfs_get_human_time(&mh_time, dumpcmfs_get_stat_mtime(st));
	if (ret)
		goto out;
	ret = dumpcmfs_get_human_time(&ch_time, dumpcmfs_get_stat_ctime(st));
	if (ret)
		goto out;

	fprintf(stdout, "Access: %s\n", ah_time);
	fprintf(stdout, "Modify: %s\n", mh_time);
	fprintf(stdout, "Change: %s\n", ch_time);

out:
	if (path)
		cmfs_free(&path);
	if (filetype)
		cmfs_free(&filetype);
	if (uname)
		cmfs_free(&uname);
	if (gname)
		cmfs_free(&gname);
	if (h_perm)
		cmfs_free(&h_perm);
	if (ah_perm)
		cmfs_free(&ah_perm);
	if (mh_time)
		cmfs_free(&mh_time);
	if (ch_time)
		cmfs_free(&ch_time);

	return ret;
}

static int filestat_run(struct dumpcmfs_operation *op,
			struct dumpcmfs_method *dm,
			void *arg)
{
	int ret = 0, flags = 0;
	struct stat st;
	struct dumpcmfs_volinfo cvf;
	struct dumpcmfs_fiemap cfp;

	if (dm->dm_method == DUMPCMFS_USE_LIBCMFS) {
		dumpcmfs_error(op, "specify a non-device file to stat\n");
		ret = -1;
		goto out;
	}

	ret = lstat(dm->dm_path, &st);
	if (ret < 0) {
		ret = errno;
		dumpcmfs_error(op, "lstat error: %s\n", strerror(ret));
		ret = -1;
		goto out;
	}

	memset(&cfp, 0, sizeof(cfp));

	ret = get_volinfo_ioctl(op, dm->dm_fd, &cvf);
	if (ret)
		return -1;

	cfp.blocksize = cvf.blocksize;
	cfp.clustersize = cvf.clustersize;

	ret = dumpcmfs_get_fiemap(dm->dm_fd, flags, &cfp);
	if (ret)
		goto out;

	ret = dumpcmfs_report_filestat(dm, &st, &cfp);
out:
	return ret;
}

DEFINE_DUMPCMFS_OP(filestat, filestat_run, NULL);



static int help_handler(struct dumpcmfs_option *opt, char *arg)
{
	print_usage();
	exit(0);
}

static int version_handler(struct dumpcmfs_option *opt, char *arg)
{
	tools_version();
	exit(0);
}

static struct dumpcmfs_option help_option = {
	.opt_option = {
		.name		= "help",
		.val		= 'h',
		.has_arg	= 0,
		.flag		= NULL,
	},
	.opt_help	= NULL,
	.opt_handler	= help_handler,
	.opt_op		= NULL,
	.opt_private	= NULL,
};

static struct dumpcmfs_option version_option = {
	.opt_option = {
		.name		= "version",
		.val		= 'V',
		.has_arg	= 0,
		.flag		= NULL,
	},
	.opt_help	= NULL,
	.opt_handler	= version_handler,
	.opt_op		= NULL,
	.opt_private	= NULL,
};

static struct dumpcmfs_option fs_features_option = {
	.opt_option = {
		.name		= "fs-features",
		.val		= CHAR_MAX,
		.has_arg	= 0,
		.flag		= NULL,
	},
	.opt_help	= "   --fs-features",
	.opt_handler	= NULL,
	.opt_op		= &fs_features_op,
	.opt_private	= NULL,
};

static struct dumpcmfs_option volinfo_option = {
	.opt_option = {
		.name		= "volinfo",
		.val		= CHAR_MAX,
		.has_arg	= 0,
		.flag		= NULL,
	},
	.opt_help	= "   --valinfo",
	.opt_handler	= NULL,
	.opt_op		= &valinfo_op,
	.opt_private	= NULL,
};

static struct dumpcmfs_option mkfs_option = {
	.opt_option = {
		.name		= "mkfs",
		.val		= CHAR_MAX,
		.has_arg	= 0,
		.flag		= NULL,
	},
	.opt_help		= "   --mkfs",
	.opt_handler		= NULL,
	.opt_op			= &mkfs_op,
	.opt_private		= NULL,
};

static struct dumpcmfs_option freeinode_option = {
	.opt_option = {
		.name		= "freeinode",
		.val		= CHAR_MAX,
		.has_arg	= 0,
		.flag		= NULL,
	},
	.opt_help	= "   --freeinode",
	.opt_handler	= NULL,
	.opt_op		= &freeinode_op,
	.opt_private	= NULL,
};

static struct dumpcmfs_option freefrag_option = {
	.opt_option = {
		.name		= "freefrag",
		.val		= CHAR_MAX,
		.has_arg	= 1,
		.flag		= NULL,
	},
	.opt_help	= "   --freefrag <chunksize in Mb",
	.opt_handler	= NULL,
	.opt_op		= &freefrag_op,
	.opt_private	= NULL,
};

static struct dumpcmfs_option space_usage_option = {
	.opt_option = {
		.name		= "space-usage",
		.val		= CHAR_MAX,
		.has_arg	= 0,
		.flag		= NULL,
	},
	.opt_help	= "   --space-usage",
	.opt_handler	= NULL,
	.opt_op		= &free_space_op,
	.opt_private	= NULL,
};

static struct dumpcmfs_option filestat_option = {
	.opt_option = {
		.name		= "filestat",
		.val		= CHAR_MAX,
		.has_flag	= 0,
		.flag		= NULL,
	},
	.opt_help	= "   --filestat",
	.opt_handler	= NULL,
	.opt_op		= &filestat_op,
	.opt_private	= NULL,
};

static struct dumpcmfs_option *options[] = {
	&help_option,
	&version_option,
	&fs_features_option,
	&volinfo_option,
	&mkfs_option,
	&freeinode_option,
	&freefrag_option,
	&space_usage_option,
	&filestat_option,
	NULL,
};

void print_usage(int rc)
{
	int i;
	enum tools_verbosity_level level = VL_ERR;

	if (!rc)
		level = VL_OUT;

	verbosef(level, "Usage: %s [options] <device or file>\n",
		 tools_progname());
	verbosef(level, "       %s -h|--help\n", tools_progname());
	verbosef(level, "       %s -V|--verion\n", tools_progname());
	verbosef(level, "[options] can be followings:\n");

	for(i = 0; options[i]; i++) {
		if (options[i]->opt_help)
			verbosef(level, "\t%s\n", options[i]->opt_help);
	}
	exit(rc);
}

static struct dumpcmfs_option *find_option_by_val(int val)
{
	int i;
	sruct dumpcmfs_option *opt = NULL;

	for(i = 0; options[i]; i++) {
		if (options[i]->opt_option.val == val) {
			opt = options[i];
			break;
		}
	}
	return opt;
}

static errcode_t dumpcmfs_append_task(struct dumpcmfs_operation *op)
{
	errcode_t err;
	struct dumpcmfs_op_task *task;

	err = cmfs_malloc0(sizeof(struct dumpcmfs_op_task), &task);
	if (!err) {
		task->dumpcmfs_task = task;
		list_add_tail(&task->dumpcmfs_list, &dumpcmfs_op_task_list);
		dumpcmfs_op_task_count ++;
	} else {
		cmfs_free(&task);
	}

	return err;
}

static void dumpcmfs_free_op_task_list()
{
	struct dumpcmfs_op_task *task;
	struct list_head *pos, *next;
	if (list_empty(&dumpcmfs_op_task_list))
		return;

	list_for_each_safe(pos, next, &dumpcmfs_op_task_list) {
		task = list_entry(pos, struct dumpcmfs_op_task, dumpcmfs_op_list);
		list_del(pos);
		cmfs_free(&task);
	}
}

static errcode_t dumpcmfs_run_task(struct dumpcmfs_method *dm)
{
	struct list_head *p, *n;
	struct dumpcmfs_op_task *task;

	list_for_each_safe(p, n, &dumpcmfs_op_task_list) {
		task = list_entry(p, struct dumpcmfs_op_task, dumpcmfs_op_list);
		task->dumpcmfs_task->to_run(task->dumpcmfs_task, dm,
					    task->dumpcmfs_task->to_private);
	}
	return 0;
}

static void handle_signal(int caught_sig)
{
	int exitp = 0, abortp = 0;
	static int segv_already;

	switch(caught_sig) {
	case SIGQUIT:
	      abortp = 1;
	case SIGTERM:
	case SIGINT:
	case SIGHUP:
		errorf("Caught signal %d, exiting\n", caught_sig);
		exitp = 1;
		break;
	case SIGSEGV:
		errorf("Segmentation fault, existing\n");
		exitp = 1;
		if (segv_already) {
			errorf("Segmentation fault loop detected\n");
			abortp = 1;
		} else {
			segv_already = 1;
		}
		break;
	default:
		errorf("Caught signal %d, ignoring\n", caught_sig);
		break;
	}
	if (!exitp)
		return;
	if (abortp)
		abort();
	exit(1);
}

static int setup_signals(void)
{
	int rc = 0;
	struct sigaction act;

	act.sa_sigaction = NULL;
	sigemptyset(&act.sa_mask);
	act.sa_handler = handle_singal;
	act.sa_flags = SA_INTERRUPT;
	rc += sigaction(SIGTERM, &act, NULL);
	rc += sigaction(SIGINT, &act, NULL);
	rc += sigaction(SIGHUP, &act, NULL);
	rc += sigaction(SIGQUIT, &act, NULL);
	rc += sigaction(SIGSEGV, &act, NULL);
	act.sa_handler = SIG_IGN;
	rc += sigaction(SIGPIPE, &act, NULL);

	return rc;
}

static void dumpcmfs_init(const char *argv0)
{
	initialize_cmfs_error_table();
	tools_setup_argv0(argv0);
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	if (setup_signals()) {
		errorf("Unable to setup signal handling\n");
		exit(1);
	}
	cluster_coherent = 0;
}

int main(int argc, char *argv[])
{
	int rc = 0;
	char *device_or_file = NULL;
	static struct dumpcmfs_method dm;

	dumpcmfs_init(argv[0]);
	parse_options(argc, argv, &device_or_file);

	rc = dumpcmfs_method(device_or_file);
	if (rc < 0)
		goto out;
	else
		dm.dm_method = rc;

	strncpy(dm.dm_path, device_or_file, PATH_MAX);

	rc = dumpcmfs_open(&dm, 0);
	if (rc)
		goto out;

	rc = dumpcmfs_run_task(&dm);
	if (rc)
		got out;

	dumpcmfs_free_op_task_list();

	rc = dumpcmfs_close(&dm);
out:
	if (device_or_file)
		cmfs_free(&device_or_file);

	return rc;
}












