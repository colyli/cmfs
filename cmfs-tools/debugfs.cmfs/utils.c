/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * utils.c
 *
 * utility functions
 * (Copied and modified from ocfs2-tools/debugfs.ocfs2/utils.c)
 *
 * Copyright (C) 2004, 2011 Oracle.  All rights reserved.
 * CMFS modification, by Coly Li <i@coly.li>
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

#include <unistd.h>

#include <cmfs/cmfs.h>
#include <cmfs/bitops.h>
#include "main.h"
#include "dump.h"

extern struct dbgfs_gbls gbls;

/*
 * open_pager() -- copied from e2fsprogs-1.32/debugfs/util.c
 *
 * Copyright (C) 1993, 1994 Theodore Ts'o. This file may be
 * redistributed under the terms of the GNU Public License.
 */
FILE *open_pager(int interactive)
{
	FILE *outfile = NULL;
	const char *pager = getenv("PAGER");

	if (interactive) {
		signal(SIGPIPE, SIG_IGN);
		if (pager) {
			if (strcmp(pager, "__none__") == 0)
				return stdout;
		} else {
			pager = "/bin/more";
		}
		outfile = popen(pager, "w");
	}
	return (outfile ? outfile : stdout);
}

/*
 * close_pager() - copied from e2fsprogs-1.3.2/debugfs/util.c
 *
 * Copyright (C) 1993, 1994 Theodore Ts'o. This file may be
 * redistributed under the terms of the GNU Public License.
 */
void close_pager(FILE *stream)
{
	if (stream && stream != stdout)
		pclose(stream);
}

/*
 * crunch_strsplit()
 *
 * Moves empty strings to the end in args returned by g_strsplit()
 *
 */
void crunch_strsplit(char **args)
{
	char *p;
	int i,j;

	i = j = 0;
	while(args[i]) {
		if (!strlen(args[i])) {
			j = max(j, i+1);
			while (args[j]) {
				if (strlen(args[j])) {
					p = args[i];
					args[i] = args[j];
					args[j] = p;
					break;
				}
				++j;
			}
			if (!args[j])
				break;
		}
		++i;
	}
	return;
}

/*
 * read in buflen bytes or whole file if buflen == 0
 *
 * buf allocated here should be freed by caller.
 */
errcode_t read_whole_file(cmfs_filesys *fs,
			  uint64_t ino,
			  char **buf,
			  uint32_t *buflen)
{
	errcode_t ret;
	uint32_t got;
	cmfs_cached_inode *ci = NULL;

	ret = cmfs_read_cached_inode(fs, ino, &ci);
	if (ret) {
		com_err(gbls.cmd, ret, "while reading inode %"PRIu64, ino);
		goto bail;
	}

	if (!(*buflen)) {
		*buflen = (((ci->ci_inode->i_size + fs->fs_blocksize - 1) >>
			    CMFS_RAW_SB(fs->fs_super)->s_blocksize_bits) <<
			   CMFS_RAW_SB(fs->fs_super)->s_blocksize_bits);
	}

	/* bail if file size is larger than reasonable */
	if (*buflen > 100 * 1024 * 1024) {
		ret = CMFS_ET_INTERNAL_FAILURE;
		goto bail;
	}

	ret = cmfs_malloc_blocks(fs->fs_io,
				 (*buflen >>
				  CMFS_RAW_SB(fs->fs_super)->s_blocksize_bits),
				 buf);
	if (ret) {
		com_err(gbls.cmd, ret, "while allocating %u bytes", *buflen);
		goto bail;
	}

	ret = cmfs_file_read(ci, *buf, *buflen, 0, &got);
	if (ret)
		com_err(gbls.cmd, ret,
			"while reading file at inode %"PRIu64,
			ci->ci_blkno);

bail:
	if (ci)
		cmfs_free_cached_inode(fs, ci);
	return ret;

}

/*
 * dump_symlink()
 *
 * Code based on similar function in e2fsprogs-1.32/debugfs/dump.c
 *
 * Copyright (C) 1994 Theodore Ts'o. This file may be redistributed
 * under the terms of the GNU Public License.
 */
static errcode_t dump_symlink(cmfs_filesys *fs,
			     uint64_t blkno,
			     char *name,
			     struct cmfs_dinode *inode)
{
	char *buf = NULL;
	uint32_t len = 0;
	errcode_t ret = 0;
	char *link = NULL;

	if (!inode->i_clusters)
		link = (char *)inode->id2.i_symlink;
	else {
		ret = read_whole_file(fs, blkno, &buf, &len);
		if (ret)
			goto bail;
		link = buf;
	}

	if (symlink(link, name) == -1)
		ret = errno;

bail:
	if (buf)
		cmfs_free(&buf);
	return ret;
}

/*
 * fix_perms()
 *
 * Code based on similar function in e2fsprogs-1.32/debugfs/dump.c
 *
 * Copyright (C) 1994 Theodore Ts'o. This file may be redistributed
 * under the terms of the GNU Public License.
 */
static errcode_t fix_perms(const struct cmfs_dinode *di,
			   int *fd,
			   char *name)
{
	struct utimbuf ut;
	int i;
	errcode_t ret = 0;

	if (*fd != -1)
		i = fchmod(*fd, di->i_mode);
	else
		i = chmod(name, di->i_mode);
	if (i == -1) {
		ret = errno;
		goto bail;
	}

	if (*fd != -1)
		i = fchown(*fd, di->i_uid, di->i_gid);
	else
		i = chown(name, di->i_uid, di->i_gid);
	if (i == -1) {
		ret = errno;
		goto bail;
	}

	if (*fd != -1) {
		close(*fd);
		*fd = -1;
	}

	ut.actime = di->i_atime;
	ut.modtime = di->i_mtime;
	if (utime(name, &ut) == -1)
		ret = errno;

bail:
	return ret;
}

errcode_t dump_file(cmfs_filesys *fs,
		    uint64_t ino,
		    int fd,
		    char *out_file,
		    int preserve)
{
	errcode_t ret;
	char *buf = NULL;
	int buflen;
	uint32_t got;
	uint32_t wrote;
	cmfs_cached_inode *ci = NULL;
	uint64_t offset = 0;

	ret = cmfs_read_cached_inode(fs, ino, &ci);
	if (!ret) {
		com_err(gbls.cmd, ret, "while reading inode %"PRIu64, ino);
		goto bail;
	}

	if (S_ISLNK(ci->ci_inode->i_mode)) {
		ret = unlink(out_file);
		if (ret)
			goto bail;

		ret = dump_symlink(fs, ino, out_file, ci->ci_inode);
		goto bail;
	}

	buflen = 1 << 20;
	ret = cmfs_malloc_blocks(fs->fs_io,
				 (buflen >>
				  CMFS_RAW_SB(fs->fs_super)->s_blocksize_bits),
				  &buf);
	if (ret) {
		com_err(gbls.cmd, ret, "while allocating %u bytes", buflen);
		goto bail;
	}

	while (1) {
		ret = cmfs_file_read(ci, buf, buflen, offset, &got);
		if (ret) {
			com_err(gbls.cmd, ret, "while reading file %"PRIu64
				" at offset %"PRIu64, ci->ci_blkno, offset);
			goto bail;
		}

		if (!got)
			break;

		wrote = write(fd, buf, got);
		if (wrote != got) {
			com_err(gbls.cmd, errno, "while writing file");
			ret = errno;
			goto bail;
		}

		if (got < buflen)
			break;
		else
			offset += got;
	}

	if (preserve)
		ret = fix_perms(ci->ci_inode, &fd, out_file);
bail:
	if (fd > 0 && fd != fileno(stdout))
		close(fd);
	if (buf)
		cmfs_free(&buf);
	if (ci)
		cmfs_free_cached_inode(fs, ci);
	return ret;
}

/*
 * returns ino if string is of the form <ino>
 */
int inodestr_to_inode(char *str, uint64_t *blkno)
{
	int len;
	char *buf = NULL;
	char *end;
	int ret = CMFS_ET_INVALID_ARGUMENT;

	len = strlen(str);
	if (!((len > 2) && (str[0] == '<') && (str[len - 1] == '>')))
		goto bail;

	ret = CMFS_ET_NO_MEMORY;
	buf = strndup(str + 1, len - 2);
	if (!buf)
		goto bail;

	ret = 0;
	*blkno = strtoull(buf, &end, 0);
	if (*end)
		ret = CMFS_ET_INVALID_ARGUMENT;

bail:
	if (buf)
		free(buf);
	return ret;
}

/*
 * This routine is used whenever a command needs to turn a string into
 * an inode.
 *
 * Code based on similar function in e2fsprogs-1.32/debugfs/util.c
 *
 * Copyright (C) 1993, 1994 Theodore Ts'o. This file may be
 * redistributed under the terms of the GNU Public License.
 */
errcode_t string_to_inode(cmfs_filesys *fs,
			  uint64_t root_blkno,
			  uint64_t cwd_blkno,
			  char *str,
			  uint64_t *blkno)
{
	uint64_t root = root_blkno;

	/*
	 * If the string is of the form <ino>, then treat it as an
	 * inode number.
	 */
	if (!inodestr_to_inode(str, blkno))
		return 0;

	/* // is short for system directory */
	if (!strncmp(str, "//", 2)) {
		root = fs->fs_sysdir_blkno;
		++str;
	}
	return cmfs_namei(fs, root, cwd_blkno, str, blkno);
}

errcode_t traverse_chains(cmfs_filesys *fs,
			  struct cmfs_chain_list *cl,
			  FILE *out)
{
	return -1;
}

void get_incompat_flag(struct cmfs_super_block *sb,
		       char *buf,
		       size_t count)
{
	errcode_t err;
	cmfs_fs_options flags = {
		.opt_incompat = sb->s_feature_incompat,
	};

	*buf = '\0';
	err = cmfs_snprint_feature_flags(buf, count, &flags);
	if (err)
		com_err(gbls.cmd, err, "while processing incompat flags");
}

void get_compat_flag(struct cmfs_super_block *sb,
		     char *buf,
		     size_t count)
{
	errcode_t err;
	cmfs_fs_options flags = {
		.opt_compat = sb->s_feature_compat,
	};

	*buf = '\0';
	err = cmfs_snprint_feature_flags(buf, count, &flags);
	if (err)
		com_err(gbls.cmd, err, "while processing compat flags");
}

void get_rocompat_flag(struct cmfs_super_block *sb,
		       char *buf,
		       size_t count)
{
	errcode_t err;
	cmfs_fs_options flags = {
		.opt_ro_compat = sb->s_feature_ro_compat,
	};

	*buf = '\0';
	err = cmfs_snprint_feature_flags(buf, count, &flags);
	if (err)
		com_err(gbls.cmd, err, "while processing ro compat flags");
}

void inode_perms_to_str(uint16_t mode, char *str, int len)
{
	if (len < 11)
		DBGFS_FATAL("internal error");

	if (S_ISREG(mode))
		str[0] = '-';
	else if (S_ISDIR(mode))
		str[0] = 'd';
	else if (S_ISLNK(mode))
		str[0] = 'l';
	else if (S_ISCHR(mode))
		str[0] = 'c';
	else if (S_ISBLK(mode))
		str[0] = 'b';
	else if (S_ISFIFO(mode))
		str[0] = 'f';
	else if (S_ISSOCK(mode))
		str[0] = 's';
	else
		str[0] = '-';

	str[1] = (mode & S_IRUSR) ? 'r' : '-';
	str[2] = (mode & S_IWUSR) ? 'w' : '-';
	if (mode & S_ISUID)
		str[3] = (mode & S_IXUSR) ? 's' : 'S';
	else
		str[3] = (mode & S_IXUSR) ? 'x' : '-';
	
	str[4] = (mode & S_IRGRP) ? 'r' : '-';
	str[5] = (mode & S_IWGRP) ? 'w' : '-';
	if (mode & S_ISGID)
		str[6] = (mode & S_IXGRP) ? 's' : 'S';
	else
		str[6] = (mode & S_IXGRP) ? 'x' : '-';
	
	str[7] = (mode & S_IROTH) ? 'r' : '-';
	str[8] = (mode & S_IWOTH) ? 'w' : '-';
	if (mode & S_ISVTX)
		str[9] = (mode & S_IXOTH) ? 't' : 'T';
	else
		str[9] = (mode & S_IXOTH) ? 'x' : '-';

	str[10] = '\0';

	return ;
}

void inode_time_to_str(uint64_t timeval, char *str, int len)
{
	time_t tt = (time_t) timeval;
	struct tm *tm;

	static const char *month_str[] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

	tm = localtime(&tt);

	snprintf(str, len, "%2d-%s-%4d %02d:%02d", tm->tm_mday,
		 month_str[tm->tm_mon], 1900 + tm->tm_year,
		 tm->tm_hour, tm->tm_min);

	return ;
}

void find_max_contig_free_bits(struct cmfs_group_desc *gd,
			       int *max_contig_free_bits)
{
	int end = 0;
	int start;
	int free_bits;

	*max_contig_free_bits = 0;

	while (end < gd->bg_bits) {
		start = cmfs_find_next_bit_set(gd->bg_bitmap,
					       gd->bg_bits,
					       end);
		if (start >= gd->bg_bits)
			break;

		end = cmfs_find_next_bit_set(gd->bg_bitmap,
					     gd->bg_bits,
					     start);
		free_bits = end - start;
		if (*max_contig_free_bits < free_bits)
			*max_contig_free_bits = free_bits;
	}
}

/*
 * Adds nanosec to the ctime output. eg. Jue Jul 19 13:36:52.123456 2011
 * On error, returns an empty string.
 */
void ctime_nano(struct timespec *t, char *buf, int buflen)
{
	time_t sec;
	char tmp[26], *p;

	sec = (time_t)t->tv_sec;
	if (!ctime_r(&sec, tmp))
		return;

	/* Find the last space where we will append the nanosec */
	if ((p = strrchr(tmp, ' '))) {
		*p = '\0';
		snprintf(buf, buflen, "%s.%ld %s", tmp, t->tv_nsec, ++p);
	}
}
