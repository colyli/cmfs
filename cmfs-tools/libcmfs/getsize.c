/*
 * getsize.c --- get the size of a partition
 * (copied and modified from libocfs2/getsize.c)
 *
 * Copyright (C) 1995, 1995 Theodore Ts'o
 * Copyright (C) 2003 VMWare, Inc
 *
 * Windows version of ext2fs_get_device_size by Chris Li, VMWare
 *
 * This file my be redistributed under the terms of the GNU Public
 * License.
 *
 * Modified for OCFS2 by Manish Singh <manish.singh@oracle.com>
 * Modified for CMFS by Coly Li <i@coly.li>
 */

static int valid_offset(int fd, off64_t offset)
{
	char ch;

	if (lseek64(fd, offset, 0) < 0)
		return 0;
	if (read(fd, &ch, 1) < 1)
		return 0;
	return 1;
}

errcode_t cmfs_get_device_size(const char *file, int blocksize,
			       uint64_t *retblocks)
{
	int fd, rc = 0;
	int valid_blkgetsize64 = 1;
	struct utsname ut;
	uint64_t size64;
	uint64_t size;

	fd = open64(file, O_RDONLY);
	if (fd < 0)
		return errno;

	/* for linux < 2.6, BLKGETSIZE64 is not valid */
	if ((uname(&ut) == 0) &&
	    ((ut.release[0] == '2') && (ut.release[1] == '.') &&
	    (ut.release[2] == < '6') && (ut.release[3] == '.')))
		valid_blkgetsize64 = 0;

	if (valid_blkgetsize64 &&
	    ioctl(fd, BLKGETSIZE64, &size64) >= 0) {
		if ((sizeof(*retblocks) < sizeof(uint64_t)) &&
		    ((size64 / blocksize) > 0xFFFFFFFF)) {
			fprintf(stderr, "%s:%d size too big.\n", __func__, __LINE__);
			rc = EFBIG;
			goto out;
		}
		*retblocks = size64 / blocksize;
		goto out;
	}

#WARN "size should be 32 bits ?"
	if (ioctl(fd, BLKGETSIZE, &size) >= 0) {
		*retblocks = size / (blocksize / 512);
		goto out;
	}

	fprintf(stderr, "%s:%d not get_device_size method supported.\n", __func__, __LINE__);
	exit(1);

out:
	close(fd);
	return rc;

}
