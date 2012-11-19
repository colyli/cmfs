#include <et/com_err.h>
#include <stdio.h>
#include <mntent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

#include <cmfs/cmfs.h>



/*
 * Helper function which checks /proc/mounts to see
 * if a file system is mounted. Returns an error if
 * the file does not exist or can't be opened.
 */
static errcode_t check_mntent_file(const char *mounts_file,
				   const char *file,
				   int *mount_flags,
				   char *mtpt,
				   int mtlen)
{
	struct mntent	*mnt;
	struct stat	st_buf;
	errcode_t	retval = ENOENT;
	dev_t		file_dev = 0, file_rdev = 0;
	ino_t		file_ino = 0;
	FILE		*f;
	int		fd;

	*mount_flags = 0;
	if ((f = setmntent(mounts_file, "r")) == NULL)
		return errno;
	if (stat(file, &st_buf) != 0)
		return errno;

	if (S_ISBLK(st_buf.st_mode)) {
		file_dev = st_buf.st_dev;
		file_ino = st_buf.st_ino;
	}
	while((mnt = getmntent(f)) != NULL) {
		/* matche name */
		if (strcmp(file, mnt->mnt_fsname) == 0)
			break;

		/* match dev numbers */
		if (file_dev == 0 ||
		    file_ino == 0)
			continue;
		if (stat(mnt->mnt_fsname, &st_buf))
			continue;
		if (!S_ISBLK(st_buf.st_mode))
			continue;
		if ((file_dev == st_buf.st_dev) &&
		    (file_ino == st_buf.st_ino))
			break;
	}

	if (mnt == NULL)
		goto errout;

	*mount_flags = CMFS_MF_MOUNTED;

	/* Check to see if the ro option is set */
	if (hasmntopt(mnt, MNTOPT_RO))
		*mount_flags |= CMFS_MF_READONLY;

	/* Check to see if the mount entry is root */
	if (mtpt)
		strncpy(mtpt, mnt->mnt_dir, mtlen);

	if (!strcmp(mnt->mnt_dir, "/"))
		*mount_flags |= CMFS_MF_ISROOT;

	retval = 0;
errout:
	endmntent(f);
	return retval;
}

/*
 * Check to see if we deal with the swap device
 */
static int is_swap_device(const char *file)
{
	FILE	*f;
	char	buf[1024], *cp;
	dev_t	file_dev;
	struct stat st_buf;
	int	ret = 0;

	file_dev = 0;

	if ((f = fopen("/proc/swaps", "r")) == NULL)
		return 0;

	/* Read and skip first (header) line */
	fgets(buf, sizeof(buf), f);
	while(!feof(f)) {
		if (!fgets(buf, sizeof(buf), f))
			break;
		if ((cp = strchr(buf, ' ')) != NULL)
			*cp = 0;
		if ((cp = strchr(buf, '\t')) != NULL)
			*cp = 0;
		if (strcmp(buf, file) == 0) {
			ret ++;
			break;
		}
	}
	fclose(f);
	return ret;
}

static errcode_t check_mntent(const char *file,
			      int *mount_flags,
			      char *mtpt,
			      int mtlen)
{
	errcode_t retval;

	retval = check_mntent_file("/proc/mounts",
				   file,
				   mount_flags,
				   mtpt,
				   mtlen);

	if (retval != 0)
		*mount_flags = 0;

	return 0;
}

/*
 * cmfs_check_mount_point() fills determines if the device
 * is mounted or otherwise busy, and fills in mount_flags
 * with one or more of the following flags:
 * - CMFS_MF_MOUNTED
 * - CMFS_MF_ISROOT
 * - CMFS_MF_READONLY
 * - CMFS_MF_SWAP
 * - CMFS_MF_BUSY
 *   If mtpt is non-NULL, the directory where the device
 *   is mounted is copied to where mtpt is pointing, up to
 *   mtlen characters.
 */
errcode_t cmfs_check_mount_point(const char *device,
				 int *mount_flags,
				 char *mtpt,
				 int mtlen)
{
	struct stat st_buf;
	errcode_t retval = 0;
	int fd;

	if (is_swap_device(device)) {
		*mount_flags = CMFS_MF_MOUNTED | CMFS_MF_SWAP;
		strncpy(mtpt, "<swap>", mtlen);
	} else {
		retval = check_mntent(device, mount_flags, mtpt, mtlen);
	}
	if (retval)
		return retval;

	/* Only works on Linux 2.6+ kernel */
	if ((stat(device, &st_buf) != 0) ||
	    (!S_ISBLK(st_buf.st_mode)))
		return 0;

	fd = open(device, O_RDONLY | O_EXCL);
	if (fd < 0) {
		if (errno == EBUSY)
			*mount_flags |= CMFS_MF_BUSY;
	} else {
		close(fd);
	}

	return 0;
}


/*
 * cmfs_check_if_mounted() sets the mount_flags,
 * CMFS_MF_MOUNTED, CMFS_MF_READONLY, CMFS_MF_ROOT
 */
errcode_t cmfs_check_if_mounted(const char *file, int *mount_flags)
{
	return cmfs_check_mount_point(file, mount_flags, NULL, 0);
}
