/* -*- mode: c; c-basic-offset: 8; -*-                                                                                            
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * check.c
 *
 * CMFS format check utility
 * Originally copy and modify from ocfs2-tools code.
 *
 * Copyright (C) 2012, Coly Li <i@coly.li>
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
 */

#include <stdio.h>


#include <cmfs/cmfs.h>
#include "mkfs.h"
#define WHOAMI	"mkfs.cmfs"


int cmfs_check_volume(State *s)
{
	cmfs_filesys *fs = NULL;
	errcode_t ret;
	int mount_flags;

	if (s->dry_run) {
		fprintf(stdout, "Dry run\n");
		return 0;
	}

	ret = cmfs_check_if_mounted(s->device_name, &mount_flags);
	if (ret) {
		com_err(s->progname, ret,
			"while determing whether %s is mounted.",
			s->device_name);
		return -1;
	}

	if (mount_flags & CMFS_MF_MOUNTED) {
		fprintf(stderr, "%s is mounted; ", s->device_name);
		if (s->force) {
			fprintf(stderr, "overwriting anyway. Hope /etc/mtab is "
					"incorrect.\n");
			return 1;
		}
		fprintf(stderr, "will not make a cmfs volume here!\n");
		return -1;
	}

	if (mount_flags & CMFS_MF_BUSY) {
		fprintf(stderr, "%s is apparently in use by the system; ",
				s->device_name);
		if (s->force) {
			fprintf(stderr, "format force anyway.\n");
			return 1;
		}
		fprintf(stderr, "will not make a cmfs volume here!\n");
		return -1;
	}

	ret = cmfs_open(s->device_name, CMFS_FLAG_RW, 0, 0, &fs);

	/* no existing cmfs file system to open */
	if (ret)
		return 0;

	/* a cmfs file system existing */
	fprintf(stderr, "Overwriting existing cmfs partition\n");
	cmfs_close(fs);

	return 1;
}
