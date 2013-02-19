/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * commands.c
 *
 * handles debugfs commands
 *
 * Copyright (C) 2004, 2011 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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
 */
#define _GNU_SOURCE
#define _XOPEN_SOURCE 600
#define _LARGEFILE64_SOURCE

#include <fcntl.h>

#include <cmfs/byteorder.h>
#include <cmfs/cmfs.h>
#include "main.h"
#include "stat_sysdir.h"



#define SYSTEM_FILE_NAME_MAX	40
#define MAX_BLOCKS		50

struct dbgfs_gbls gbls;

typedef void (*cmdfunc)(char **args);

struct command {
	char		*cmd_name;
	cmdfunc		cmd_func;
	char		*cmd_usage;
	char		*cmd_desc;
};

static struct command *find_command(char  *cmd);

static void do_bmap(char **args);
static void do_cat(char **args);
static void do_cd(char **args);
static void do_chroot(char **args);
static void do_close(char **args);
static void do_curdev(char **args);
static void do_dirblocks(char **args);
static void do_extent(char **args);
static void do_frag(char **args);
static void do_group(char **args);
static void do_help(char **args);
static void do_icheck(char **args);
static void do_lcd(char **args);
static void do_locate(char **args);
static void do_ls(char **args);
static void do_open(char **args);
static void do_quit(char **args);
static void do_stat(char **args);
static void do_stat_sysdir(char **args);
static void do_stats(char **args);

static struct command commands[] = {
	{ "bmap",
		do_bmap,
		"bmap <filespec> <logical_blk>",
		"Show the corresponding physical block# for the inode",
	},
	{ "cat",
		do_cat,
		"cat <filespec>",
		"Show file on stdout",
	},
	{ "cd",
		do_cd,
		"cd <filespec>",
		"Change directory",
	},
	{ "chroot",
		do_chroot,
		"chroot <filespec>",
		"Change root",
	},
	{ "close",
		do_close,
		"close",
		"Close a device",
	},
	{ "curdev",
		do_curdev,
		"curdev",
		"Show current device",
	},
	{ "dirblocks",
		do_dirblocks,
		"dirblocks <filespec>",
		"Dump directory blocks",
	},
	{ "extent",
		do_extent,
		"extent <block#>",
		"Show extent block",
	},
	{ "findpath",
		do_locate,
		"findpath <block#>",
		"List one pathname of the inode/lockname",
	},
	{ "frag",
		do_frag,
		"frag <filespec>",
		"Show inode extents / clusters ratio",
	},
	{ "group",
		do_group,
		"group <block#>",
		"Show chain group",
	},
	{ "help",
		do_help,
		"help, ?",
		"This information",
	},
	{ "?",
		do_help,
		NULL,
		NULL,
	},
	{ "icheck",
		do_icheck,
		"icheck block# ...",
		"List inode# that is using the block#",
	},
	{ "lcd",
		do_lcd,
		"lcd <directory>",
		"Change directory on a mounted flesystem",
	},
	{ "locate",
		do_locate,
		"locate <block#> ...",
		"List all pathnames of the inode(s)/lockname(s)",
	},
	{ "ls",
		do_ls,
		"ls [-l] <filespec>",
		"List directory",
	},
	{ "ncheck",
		do_locate,
		"ncheck <block#> ...",
		"List all pathnames of the inode(s)/lockname(s)",
	},
	{ "open",
		do_open,
		"open <device> [-i] [-s backup#]",
		"Open a device",
	},
	{ "quit",
		do_quit,
		"quit, q",
		"Exit the program",
	},
	{ "q",
		do_quit,
		NULL,
		NULL,
	},
	{ "stat",
		do_stat,
		"stat [-t|-T] <filespec>",
		"Show inode",
	},
	{ "stat_sysdir",
		do_stat_sysdir,
		"stat_sysdir",
		"Show all objects in the system directory",
	},
	{ "stats",
		do_stats,
		"stats [-h]",
		"Show superblock",
	},
};

void handle_signal(int sig)
{
	switch (sig) {
	case SIGTERM:
	case SIGINT:
		if (gbls.device)
			do_close(NULL);
		exit(1);
	}

	return ;
}

static struct command *find_command(char *cmd)
{
	unsigned int i;

	for (i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
		if (strcmp(cmd, commands[i].cmd_name) == 0)
			return &commands[i];

	return NULL;
}

void do_command(char *cmd)
{
	char    **args;
	struct command  *command;

	if (*cmd == '\0')
		return;

	args = g_strsplit(cmd, " ", -1);

	/* Move empty strings to the end */
	crunch_strsplit(args);

	/* ignore commented line */
	if (!strncmp(args[0], "#", 1))
		goto bail;

	command = find_command(args[0]);

	fflush(stdout);

	if (command) {
		gbls.cmd = command->cmd_name;
		command->cmd_func(args);
	} else
		fprintf(stderr, "%s: command not found\n", args[0]);

bail:
	g_strfreev(args);
}

static int check_device_open(void)
{
	if (!gbls.fs) {
		fprintf(stderr, "No device open\n");
		return -1;
	}

	return 0;
}

static int process_inode_args(char **args, uint64_t *blkno)
{
	errcode_t ret;
	char *opts = args[1];

	if (check_device_open())
		return -1;

	if (!opts) {
		fprintf(stderr, "usage: %s <filespec>\n", args[0]);
		return -1;
	}

	ret = string_to_inode(gbls.fs, gbls.root_blkno, gbls.cwd_blkno,
			      opts, blkno);
	if (ret) {
		com_err(args[0], ret, "'%s'", opts);
		return -1;
	}

	if (*blkno >= gbls.max_blocks) {
		com_err(args[0], CMFS_ET_BAD_BLKNO, "- %"PRIu64"", *blkno);
		return -1;
	}

	return 0;
}

static int process_ls_args(char **args, uint64_t *blkno, int *long_opt)
{
	errcode_t ret;
	char *def = ".";
	char *opts;
	int ind = 1;

	if (check_device_open())
		return -1;

	if (args[ind]) {
		if (!strcmp(args[1], "-l")) {
			*long_opt = 1;
			++ind;
		}
	}

	if (args[ind])
		opts = args[ind];
	else
		opts = def;

	ret = string_to_inode(gbls.fs, gbls.root_blkno, gbls.cwd_blkno,
			      opts, blkno);
	if (ret) {
		com_err(args[0], ret, "'%s'", opts);
		return -1;
	}

	if (*blkno >= gbls.max_blocks) {
		com_err(args[0], CMFS_ET_BAD_BLKNO, "- %"PRIu64"", *blkno);
		return -1;
	}

	return 0;
}

/*
 * process_inodestr_args()
 * 	args:	arguments starting from command
 * 	count:	max space available in blkno
 * 	blkno:	block nums extracted
 * 	RETURN:	number of blknos
 *
 */
static int process_inodestr_args(char **args, int count, uint64_t *blkno)
{
	uint64_t *p;
	int i;

	if (check_device_open())
		return -1;

	if (count < 1)
		return 0;

	for (i = 1, p = blkno; i < count + 1; ++i, ++p) {
		if (!args[i] || inodestr_to_inode(args[i], p))
			break;
		if (*p >= gbls.max_blocks) {
			com_err(args[0], CMFS_ET_BAD_BLKNO, "- %"PRIu64"", *p);
			return -1;
		}
	}

	--i;

	if (!i) {
		fprintf(stderr, "usage: %s <inode#>\n", args[0]);
		return -1;
	}

	return  i;
}

static errcode_t find_block_offset(cmfs_filesys *fs,
				   struct cmfs_extent_list *el,
				   uint64_t blkoff, FILE *out)
{
	struct cmfs_extent_block *eb;
	struct cmfs_extent_rec *rec;
	errcode_t ret = 0;
	char *buf = NULL;
	int i;
	uint32_t clstoff, clusters;
	uint32_t tmp;

	clstoff = cmfs_blocks_to_clusters(fs, blkoff);

	for (i = 0; i < el->l_next_free_rec; ++i) {
		rec = &(el->l_recs[i]);
		clusters = cmfs_rec_clusters(fs, el->l_tree_depth, rec);

		/*
		 * For a sparse file, we may find an empty record.
		 * Just skip it.
		 */
		if (!clusters)
			continue;

		if (clstoff >= (rec->e_cpos + clusters))
			continue;

		if (!el->l_tree_depth) {
			if (clstoff < rec->e_cpos) {
				dump_logical_blkno(out, 0);
			} else {
				tmp = blkoff -
					cmfs_clusters_to_blocks(fs,
								 rec->e_cpos);
				dump_logical_blkno(out, rec->e_blkno + tmp);
			}
			goto bail;
		}

		ret = cmfs_malloc_block(gbls.fs->fs_io, &buf);
		if (ret) {
			com_err(gbls.cmd, ret, "while allocating a block");
			goto bail;
		}

		ret = cmfs_read_extent_block(fs, rec->e_blkno, buf);
		if (ret) {
			com_err(gbls.cmd, ret, "while reading extent %"PRIu64,
				(uint64_t)rec->e_blkno);
			goto bail;
		}

		eb = (struct cmfs_extent_block *)buf;

		ret = find_block_offset(fs, &(eb->h_list), blkoff, out);
		goto bail;
	}

	dump_logical_blkno(out, 0);

bail:
	if (buf)
		cmfs_free(&buf);
	return ret;
}

static void do_open(char **args)
{
	char *dev = args[1];
	int flags;
	errcode_t ret = 0;
	char sysfile[SYSTEM_FILE_NAME_MAX];
	struct cmfs_super_block *sb;
	uint64_t superblock = 0, block_size = 0;

	if (gbls.device)
		do_close(NULL);

	if (dev == NULL) {
		fprintf(stderr, "usage: %s <device>\n", args[0]);
		return ;
	}

	flags = gbls.allow_write ? CMFS_FLAG_RW : CMFS_FLAG_RO;
	ret = cmfs_open(dev, flags, superblock, block_size, &gbls.fs);
	if (ret) {
		gbls.fs = NULL;
		com_err(args[0], ret, "while opening context for device %s",
			dev);
		return ;
	}

	/* allocate blocksize buffer */
	ret = cmfs_malloc_block(gbls.fs->fs_io, &gbls.blockbuf);
	if (ret) {
		com_err(args[0], ret, "while allocating a block");
		return ;
	}

	sb = CMFS_RAW_SB(gbls.fs->fs_super);

	/* set globals */
	gbls.device = g_strdup (dev);
	gbls.max_clusters = gbls.fs->fs_super->i_clusters;
	gbls.max_blocks = cmfs_clusters_to_blocks(gbls.fs, gbls.max_clusters);
	gbls.root_blkno = sb->s_root_blkno;
	gbls.sysdir_blkno = sb->s_system_dir_blkno;
	gbls.cwd_blkno = sb->s_root_blkno;
	if (gbls.cwd)
		free(gbls.cwd);
	gbls.cwd = strdup("/");

	snprintf(sysfile, sizeof(sysfile),
		  cmfs_system_inodes[JOURNAL_SYSTEM_INODE].si_name);
	ret = cmfs_lookup(gbls.fs, gbls.sysdir_blkno, sysfile,
			   strlen(sysfile), NULL, &gbls.jrnl_blkno);
	if (ret)
		gbls.jrnl_blkno = 0;

	return ;
}

static void do_close(char **args)
{
	errcode_t ret = 0;

	if (check_device_open())
		return ;

	ret = cmfs_close(gbls.fs);
	if (ret)
		com_err(args[0], ret, "while closing context");
	gbls.fs = NULL;

	if (gbls.blockbuf)
		cmfs_free(&gbls.blockbuf);

	g_free(gbls.device);
	gbls.device = NULL;

	return ;
}

static void do_cd(char **args)
{
	uint64_t blkno;
	errcode_t ret;

	if (process_inode_args(args, &blkno))
		return ;

	ret = cmfs_check_directory(gbls.fs, blkno);
	if (ret) {
		com_err(args[0], ret, "while checking directory at "
			"block %"PRIu64"", blkno);
		return ;
	}

	gbls.cwd_blkno = blkno;

	return ;
}

static void do_chroot(char **args)
{
	uint64_t blkno;
	errcode_t ret;

	if (process_inode_args(args, &blkno))
		return ;

	ret = cmfs_check_directory(gbls.fs, blkno);
	if (ret) {
		com_err(args[0], ret, "while checking directory at "
			"blkno %"PRIu64"", blkno);
		return ;
	}

	gbls.root_blkno = blkno;

	return ;
}

static void do_ls(char **args)
{
	uint64_t blkno;
	errcode_t ret = 0;
	struct list_dir_opts ls_opts = { gbls.fs, NULL, 0, NULL };

	if (process_ls_args(args, &blkno, &ls_opts.long_opt))
		return ;

	ret = cmfs_check_directory(gbls.fs, blkno);
	if (ret) {
		com_err(args[0], ret, "while checking directory at "
			"block %"PRIu64"", blkno);
		return ;
	}

	if (ls_opts.long_opt) {
		ret = cmfs_malloc_block(gbls.fs->fs_io, &ls_opts.buf);
		if (ret) {
			com_err(args[0], ret, "while allocating a block");
			return ;
		}
	}

	ls_opts.out = open_pager(gbls.interactive);
	ret = cmfs_dir_iterate(gbls.fs, blkno, 0, NULL,
				dump_dir_entry, (void *)&ls_opts);
	if (ret)
		com_err(args[0], ret, "while iterating directory at "
			"block %"PRIu64"", blkno);

	close_pager(ls_opts.out);

	if (ls_opts.buf)
		cmfs_free(&ls_opts.buf);

	return ;
}

static void do_help(char **args)
{
	int i, usagelen = 0;
	int numcmds = sizeof(commands) / sizeof(commands[0]);

	for (i = 0; i < numcmds; ++i) {
		if (commands[i].cmd_usage)
			usagelen = max(usagelen, strlen(commands[i].cmd_usage));
	}

#define MIN_USAGE_LEN	40
	usagelen = max(usagelen, MIN_USAGE_LEN);

	for (i = 0; i < numcmds; ++i) {
		if (commands[i].cmd_usage)
			fprintf(stdout, "%-*s  %s\n", usagelen,
				commands[i].cmd_usage, commands[i].cmd_desc);
	}
}

static void do_quit(char **args)
{
	if (gbls.device)
		do_close(NULL);
	exit(0);
}

static void do_lcd(char **args)
{
	char buf[PATH_MAX];

	if (check_device_open())
		return ;

	if (!args[1]) {
		/* show cwd */
		if (!getcwd(buf, sizeof(buf))) {
			com_err(args[0], errno, "while reading current "
				"working directory");
			return ;
		}
		fprintf(stdout, "%s\n", buf);
		return ;
	}

	if (chdir(args[1]) == -1) {
		com_err(args[0], errno, "'%s'", args[1]);
		return ;
	}

	return ;
}

static void do_curdev(char **args)
{
	printf("%s\n", gbls.device ? gbls.device : "No device");
}

static void do_stats(char **args)
{
	FILE *out;
	struct cmfs_super_block *sb;
	int c, argc;
	int only_super = 0;

	if (check_device_open())
		goto bail;

	for (argc = 0; (args[argc]); ++argc);
	optind = 0;

	while ((c = getopt(argc, args, "h")) != -1) {
		switch (c) {
		case 'h':
			only_super = 1;
			break;
		default:
			break;
		}
	}

	sb = CMFS_RAW_SB(gbls.fs->fs_super);
	out = open_pager(gbls.interactive);
	dump_super_block(out, sb);
	if (!only_super)
		dump_inode(out, gbls.fs->fs_super);
	close_pager(out);

bail:
	return ;
}

static void do_stat(char **args)
{
	struct cmfs_dinode *inode;
	uint64_t blkno;
	char *buf = NULL;
	FILE *out;
	errcode_t ret = 0;
	const char *stat_usage = "usage: stat [-t|-T] <filespec>";
	int index = 1, traverse = 1;

	if (check_device_open())
		return;

	if (!args[index]) {
		fprintf(stderr, "%s\n", stat_usage);
		return ;
	}

	if (!strncmp(args[index], "-t", 2)) {
		traverse = 1;
		index++;
	} else if (!strncmp(args[index], "-T", 2)) {
		traverse = 0;
		index++;
	}

	ret = string_to_inode(gbls.fs, gbls.root_blkno, gbls.cwd_blkno,
			      args[index], &blkno);
	if (ret) {
		com_err(args[0], ret, "'%s'", args[index]);
		return ;
	}

	buf = gbls.blockbuf;
	ret = cmfs_read_inode(gbls.fs, blkno, buf);
	if (ret) {
		com_err(args[0], ret, "while reading inode %"PRIu64"", blkno);
		return ;
	}

	inode = (struct cmfs_dinode *)buf;

	out = open_pager(gbls.interactive);
	dump_inode(out, inode);

	if (traverse) {
		if ((inode->i_flags & CMFS_LOCAL_ALLOC_FL))
			dump_local_alloc(out, &(inode->id2.i_lab));
		else if ((inode->i_flags & CMFS_CHAIN_FL))
			ret = traverse_chains(gbls.fs,
					      &(inode->id2.i_chain), out);
		else if (S_ISLNK(inode->i_mode) && !inode->i_clusters)
			dump_fast_symlink(out,
					  (char *)inode->id2.i_symlink);

		if (ret)
			com_err(args[0], ret,
				"while traversing inode at block "
				"%"PRIu64, blkno);
	}

	close_pager(out);

	return ;
}

static void do_cat(char **args)
{
	uint64_t blkno;
	errcode_t ret;
	char *buf;
	struct cmfs_dinode *di;

	if (process_inode_args(args, &blkno))
		return ;

	buf = gbls.blockbuf;
	ret = cmfs_read_inode(gbls.fs, blkno, buf);
	if (ret) {
		com_err(args[0], ret, "while reading inode %"PRIu64"", blkno);
		return ;
	}

	di = (struct cmfs_dinode *)buf;
	if (!S_ISREG(di->i_mode)) {
		fprintf(stderr, "%s: Not a regular file\n", args[0]);
		return ;
	}

	ret = dump_file(gbls.fs, blkno, fileno(stdout),  NULL, 0);
	if (ret)
		com_err(args[0], ret, "while reading file for inode %"PRIu64"",
			blkno);

	return ;
}

static void do_group(char **args)
{
	struct cmfs_group_desc *grp;
	uint64_t blkno;
	char *buf = NULL;
	FILE *out;
	errcode_t ret = 0;
	int index = 0;

	if (process_inodestr_args(args, 1, &blkno) != 1)
		return ;

	buf = gbls.blockbuf;
	out = open_pager(gbls.interactive);
	while (blkno) {
		ret = cmfs_read_group_desc(gbls.fs, blkno, buf);
		if (ret) {
			com_err(args[0], ret, "while reading block group "
				"descriptor %"PRIu64"", blkno);
			close_pager(out);
			return ;
		}

		grp = (struct cmfs_group_desc *)buf;
		dump_group_descriptor(out, grp, index);
		blkno = grp->bg_next_group;
		index++;
	}

	close_pager(out);

	return ;
}

static int dirblocks_proxy(cmfs_filesys *fs, uint64_t blkno,
			   uint64_t bcount, uint16_t ext_flags,
			   void *priv_data)
{
	errcode_t ret;
	struct dirblocks_walk *ctxt = priv_data;

	ret = cmfs_read_dir_block(fs, ctxt->di, blkno, ctxt->buf);
	if (!ret) {
		fprintf(ctxt->out, "\tDirblock: %"PRIu64"\n", blkno);
		dump_dir_block(ctxt->out, ctxt->buf);
	} else
		com_err(gbls.cmd, ret,
			"while reading dirblock %"PRIu64" on inode "
			"%"PRIu64"\n",
			blkno, (uint64_t)ctxt->di->i_blkno);
	return 0;
}

static void do_dirblocks(char **args)
{
	uint64_t ino_blkno;
	errcode_t ret = 0;
	struct dirblocks_walk ctxt = {
		.fs = gbls.fs,
	};

	if (process_inode_args(args, &ino_blkno))
		return;

	ret = cmfs_check_directory(gbls.fs, ino_blkno);
	if (ret) {
		com_err(args[0], ret, "while checking directory at "
			"block %"PRIu64"", ino_blkno);
		return;
	}

	ret = cmfs_read_inode(gbls.fs, ino_blkno, gbls.blockbuf);
	if (ret) {
		com_err(args[0], ret, "while reading inode %"PRIu64"",
			ino_blkno);
		return;
	}
	ctxt.di = (struct cmfs_dinode *)gbls.blockbuf;

	ret = cmfs_malloc_block(ctxt.fs->fs_io, &ctxt.buf);
	if (ret) {
		com_err(gbls.cmd, ret, "while allocating a block");
		return;
	}

	ctxt.out = open_pager(gbls.interactive);
	ret = cmfs_block_iterate_inode(gbls.fs, ctxt.di, 0,
					dirblocks_proxy, &ctxt);
	if (ret)
		com_err(args[0], ret, "while iterating directory at "
			"block %"PRIu64"", ino_blkno);
	close_pager(ctxt.out);

	cmfs_free(&ctxt.buf);
}

static void do_extent(char **args)
{
	struct cmfs_extent_block *eb;
	uint64_t blkno;
	char *buf = NULL;
	FILE *out;
	errcode_t ret = 0;

	if (process_inodestr_args(args, 1, &blkno) != 1)
		return ;

	buf = gbls.blockbuf;
	ret = cmfs_read_extent_block(gbls.fs, blkno, buf);
	if (ret) {
		com_err(args[0], ret, "while reading extent block %"PRIu64"",
			blkno);
		return ;
	}

	eb = (struct cmfs_extent_block *)buf;

	out = open_pager(gbls.interactive);
	dump_extent_block(out, eb);
	dump_extent_list(out, &eb->h_list);
	close_pager(out);

	return ;
}

static void do_locate(char **args)
{
	uint64_t blkno[MAX_BLOCKS];
	int count = 0;
	int findall = 1;
	
	count = process_inodestr_args(args, MAX_BLOCKS, blkno);
	if (count < 1)
		return ;

	if (!strncasecmp(args[0], "findpath", 8))
		findall = 0;

	find_inode_paths(gbls.fs, args, findall, count, blkno, stdout);
}

static void do_bmap(char **args)
{
	struct cmfs_dinode *inode;
	const char *bmap_usage = "usage: bmap <filespec> <logical_blk>";
	uint64_t blkno;
	uint64_t blkoff;
	char *endptr;
	char *buf = NULL;
	FILE *out;
	errcode_t ret = 1;
 
	if (check_device_open())
		return;

	if (!args[1]) {
		fprintf(stderr, "usage: %s\n", bmap_usage);
		return;
	}

	if (!args[1] || !args[2]) {
		fprintf(stderr, "usage: %s\n", bmap_usage);
		return;
	}

	ret = string_to_inode(gbls.fs, gbls.root_blkno, gbls.cwd_blkno,
			      args[1], &blkno);
	if (ret) {
		com_err(args[0], ret, "'%s'", args[1]);
		return;
	}

	if (blkno >= gbls.max_blocks) {
		com_err(args[0], CMFS_ET_BAD_BLKNO, "- %"PRIu64"", blkno);
		return;
	}

	blkoff = strtoull(args[2], &endptr, 0);
	if (*endptr) {
		com_err(args[0], CMFS_ET_BAD_BLKNO, "- %"PRIu64"", blkoff);
		return;
	}
 
	buf = gbls.blockbuf;
	ret = cmfs_read_inode(gbls.fs, blkno, buf);
	if (ret) {
		com_err(args[0], ret, "while reading inode %"PRIu64, blkno);
		return;
	}
 
	inode = (struct cmfs_dinode *)buf;
 
	out = open_pager(gbls.interactive);
 
	if (inode->i_flags & (CMFS_LOCAL_ALLOC_FL | CMFS_CHAIN_FL |
			      CMFS_DEALLOC_FL)) {
		dump_logical_blkno(out, 0);
		goto bail;
	}

	if (S_ISLNK(inode->i_mode) && !inode->i_clusters) {
		dump_logical_blkno(out, 0);
		goto bail;
	}

	if (blkoff > (inode->i_size >>
		      CMFS_RAW_SB(gbls.fs->fs_super)->s_blocksize_bits)) {
		dump_logical_blkno(out, 0);
		goto bail;
	}

	find_block_offset(gbls.fs, &(inode->id2.i_list), blkoff, out);

bail:
	close_pager(out);
 
	return;
}

static void do_icheck(char **args)
{
	const char *testb_usage = "usage: icheck block# ...";
	char *endptr;
	uint64_t blkno[MAX_BLOCKS];
	int i;
	FILE *out;

	if (check_device_open())
		return;

	if (!args[1]) {
		fprintf(stderr, "%s\n", testb_usage);
		return;
	}

	for (i = 0; i < MAX_BLOCKS && args[i + 1]; ++i) {
		blkno[i] = strtoull(args[i + 1], &endptr, 0);
		if (*endptr) {
			com_err(args[0], CMFS_ET_BAD_BLKNO, "- %s",
				args[i + 1]);
			return;
		}

		if (blkno[i] >= gbls.max_blocks) {
			com_err(args[0], CMFS_ET_BAD_BLKNO, "- %"PRIu64"",
				blkno[i]);
			return;
		}
	}

	out = open_pager(gbls.interactive);

	find_block_inode(gbls.fs, blkno, i, out);

	close_pager(out);

	return;
}

static errcode_t calc_num_extents(cmfs_filesys *fs,
				  struct cmfs_extent_list *el,
				  uint32_t *ne)
{
	struct cmfs_extent_block *eb;
	struct cmfs_extent_rec *rec;
	errcode_t ret = 0;
	char *buf = NULL;
	int i;
	uint32_t clusters;
	uint32_t extents = 0;

	*ne = 0;

	for (i = 0; i < el->l_next_free_rec; ++i) {
		rec = &(el->l_recs[i]);
		clusters = cmfs_rec_clusters(fs, el->l_tree_depth, rec);

		/*
		 * In a unsuccessful insertion, we may shift a tree
		 * add a new branch for it and do no insertion. So we
		 * may meet a extent block which have
		 * clusters == 0, this should only be happen
		 * in the last extent rec. */
		if (!clusters && i == el->l_next_free_rec - 1)
			break;

		extents = 1;

		if (el->l_tree_depth) {
			ret = cmfs_malloc_block(gbls.fs->fs_io, &buf);
			if (ret)
				goto bail;

			ret = cmfs_read_extent_block(fs, rec->e_blkno, buf);
			if (ret)
				goto bail;

			eb = (struct cmfs_extent_block *)buf;

			ret = calc_num_extents(fs, &(eb->h_list), &extents);
			if (ret)
				goto bail;

		}		

		*ne = *ne + extents;
	}

bail:
	if (buf)
		cmfs_free(&buf);
	return ret;
}

static void do_frag(char **args)
{
	struct cmfs_dinode *inode;
	uint64_t blkno;
	char *buf = NULL;
	FILE *out;
	errcode_t ret = 0;
	uint32_t clusters;
	uint32_t extents = 0;

	if (process_inode_args(args, &blkno))
		return;

	buf = gbls.blockbuf;
	ret = cmfs_read_inode(gbls.fs, blkno, buf);
	if (ret) {
		com_err(args[0], ret, "while reading inode %"PRIu64"", blkno);
		return ;
	}

	inode = (struct cmfs_dinode *)buf;

	out = open_pager(gbls.interactive);

	clusters = inode->i_clusters;
	if (!(inode->i_dyn_features & CMFS_INLINE_DATA_FL))
		ret = calc_num_extents(gbls.fs, &(inode->id2.i_list), &extents);

	if (ret)
		com_err(args[0], ret, "while traversing inode at block "
			"%"PRIu64, blkno);
	else
		dump_frag(out, inode->i_blkno, clusters, extents);


	close_pager(out);

	return ;
}

static void do_stat_sysdir(char **args)
{
	FILE *out;

	if (check_device_open())
		goto bail;

	out = open_pager(gbls.interactive);
	show_stat_sysdir(gbls.fs, out);
	close_pager(out);

bail:
	return ;
}
