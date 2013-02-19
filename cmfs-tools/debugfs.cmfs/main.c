/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * main.c
 *
 * entry point for debugfs.ocfs2
 *
 * Copyright (C) 2004, 2007 Oracle.  All rights reserved.
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
 * Authors: Sunil Mushran, Manish Singh
 */
#define _GNU_SOURCE
#define _XOPEN_SOURCE 600
#define _LARGEFILE64_SOURCE

#include "main.h"
#include <sys/types.h>
#include <dirent.h>

#define PROMPT "debugfs: "

extern struct dbgfs_gbls gbls;

struct log_entry {
	char *mask;
	char *action;
};

static void usage(char *progname)
{
	g_print("usage: %s [-f cmdfile] [-R request] [-i] [-s backup#] [-V] [-w] [-n] [-?] [device]\n", progname);
	g_print("\t-f, --file <cmdfile>\t\tExecute commands in cmdfile\n");
	g_print("\t-R, --request <command>\t\tExecute a single command\n");
	g_print("\t-s, --superblock <backup#>\tOpen the device using a backup superblock\n");
	g_print("\t-w, --write\t\t\tOpen in read-write mode instead of the default of read-only\n");
	g_print("\t-V, --version\t\t\tShow version\n");
	g_print("\t-n, --noprompt\t\t\tHide prompt\n");
	g_print("\t-?, --help\t\t\tShow this help\n");
	exit(0);
}

static void print_version(char *progname)
{
	fprintf(stderr, "%s %s\n", progname, VERSION);
}

static void get_options(int argc, char **argv, struct dbgfs_opts *opts)
{
	int c;
	char *ptr = NULL;
	static struct option long_options[] = {
		{ "file", 1, 0, 'f' },
		{ "request", 1, 0, 'R' },
		{ "version", 0, 0, 'V' },
		{ "help", 0, 0, '?' },
		{ "write", 0, 0, '?' },
		{ "log", 0, 0, 'l' },
		{ "noprompt", 0, 0, 'n' },
		{ "superblock", 0, 0, 's' },
		{ 0, 0, 0, 0}
	};

	while (1) {
		c = getopt_long(argc, argv, "f:R:V?wns:",
				long_options, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'f':
			opts->cmd_file = strdup(optarg);
			if (!strlen(opts->cmd_file)) {
				usage(gbls.progname);
				exit(1);
			}
			break;

		case 'R':
			opts->one_cmd = strdup(optarg);
			if (!strlen(opts->one_cmd)) {
				usage(gbls.progname);
				exit(1);
			}
			break;

		case 'w':
			opts->allow_write = 1;
			break;

		case 'n':
			opts->no_prompt = 1;
			break;

		case '?':
			print_version(gbls.progname);
			usage(gbls.progname);
			exit(0);
			break;

		case 'V':
			print_version(gbls.progname);
			exit(0);
			break;

		case 's':
			opts->sb_num = strtoul(optarg, &ptr, 0);
			break;

		default:
			usage(gbls.progname);
			break;
		}
	}

	if (optind < argc) {
		opts->device = strdup(argv[optind]);
	}

	return ;
}

static char *get_line(FILE *stream, int no_prompt)
{
	char *line;
	static char buf[1024];
	int i;

	if (stream) {
		while (1) {
			if (!fgets(buf, sizeof(buf), stream))
				return NULL;
			line = buf;
			i = strlen(line);
			if (i)
				buf[i - 1] = '\0';
			g_strchug(line);
			if (strlen(line))
				break;
		}
	} else {
		if (no_prompt)
			line = readline(NULL);
		else
			line = readline(PROMPT);

		if (line && *line) {
			g_strchug(line);
			add_history(line);
		}
	}

	return line;
}

int main(int argc, char **argv)
{
	char *line;
	struct dbgfs_opts opts;
	FILE *cmd = NULL;

	initialize_cmfs_error_table();

#define INSTALL_SIGNAL(sig)					\
	do {							\
		if (signal(sig, handle_signal) == SIG_ERR) {	\
		    printf("Could not set " #sig "\n");		\
		    goto bail;					\
		}						\
	} while (0)

	INSTALL_SIGNAL(SIGTERM);
	INSTALL_SIGNAL(SIGINT);

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	memset(&opts, 0, sizeof(opts));
	memset(&gbls, 0, sizeof(gbls));

	gbls.progname = basename(argv[0]);

	get_options(argc, argv, &opts);

	gbls.allow_write = opts.allow_write;
	if (!opts.cmd_file)
		gbls.interactive++;

	if (opts.device) {
		if (opts.sb_num)
			line = g_strdup_printf("open %s -s %u", opts.device, opts.sb_num);
		else
			line = g_strdup_printf("open %s", opts.device);
		do_command(line);
		g_free(line);
	}

	if (opts.one_cmd) {
		do_command(opts.one_cmd);
		goto bail;
	}

	if (opts.cmd_file) {
		cmd = fopen(opts.cmd_file, "r");
		if (!cmd) {
			com_err(argv[0], errno, "'%s'", opts.cmd_file);
			goto bail;
		}
	}

	if (!opts.no_prompt)
		print_version(gbls.progname);

	while (1) {
		line = get_line(cmd, opts.no_prompt);

		if (line) {
			if (!gbls.interactive && !opts.no_prompt)
				fprintf(stdout, "%s%s\n", PROMPT, line);
			do_command(line);
			if (gbls.interactive)
				free(line);
		} else {
			printf("\n");
			raise(SIGTERM);
			exit(0);
		}
	}

bail:
	if (cmd)
		fclose(cmd);
	if (opts.cmd_file)
		free(opts.cmd_file);
	if (opts.device)
		free(opts.device);
	return 0;
}
