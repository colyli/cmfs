#include <string.h>

#include <cmfs-kernel/cmfs_fs.h>
#include <cmfs/bitmap.h>
#include <cmfs/cmfs.h>
#include <cmfs/byteorder.h>

#include "cmfs_err.h"


unsigned int cmfs_dir_trailer_blk_off(cmfs_filesys *fs)
{
	return (fs->fs_blocksize - sizeof(struct cmfs_dir_block_trailer));
}

int cmfs_supports_dir_trailer(cmfs_filesys *fs)
{
	return cmfs_meta_ecc(CMFS_RAW_SB(fs->fs_super)) ||
		cmfs_supports_indexed_dirs(CMFS_RAW_SB(fs->fs_super));
}

struct cmfs_dir_block_trailer *
cmfs_dir_trailer_from_block(cmfs_filesys *fs, void  *data)
{
	char *p = data;
	p += cmfs_dir_trailer_blk_off(fs);
	return (struct cmfs_dir_block_trailer *)p;
}

void cmfs_swap_dir_trailer(struct cmfs_dir_block_trailer *trailer)
{
	if (cpu_is_little_endian)
		return;

	trailer->db_compat_inode = bswap_64(trailer->db_compat_inode);
	trailer->db_compat_rec_len = bswap_16(trailer->db_compat_rec_len);
	trailer->db_blkno = bswap_64(trailer->db_blkno);
	trailer->db_parent_dinode = bswap_64(trailer->db_parent_dinode);
	trailer->db_free_rec_len = bswap_16(trailer->db_free_rec_len);
	trailer->db_free_next = bswap_64(trailer->db_free_next);
}

/*
 * We are sure there is prepared space for the trailer, no directory
 * entry will overlap with the trailer:
 * - if we rebuild the indexed tree for a directory, no dir entry
 *   will overwrite the trailer's space
 * - if we build the indexed tree by tunefs.ocfs2, it will enable
 *   meta ecc feature before enable indexed dirs feature. Which
 *   means space for each trailer is well prepared already.
 */
void cmfs_init_dir_trailer(cmfs_filesys *fs,
			   struct cmfs_dinode *di,
			   uint64_t blkno,
			   void *buf)
{
	struct cmfs_dir_block_trailer *trailer;

	trailer = cmfs_dir_trailer_from_block(fs, buf);

	memset(trailer, 0, sizeof(struct cmfs_dir_block_trailer));
	memcpy(trailer->db_signature, CMFS_DIR_TRAILER_SIGNATURE,
	       strlen(CMFS_DIR_TRAILER_SIGNATURE));
	trailer->db_compat_rec_len = sizeof(struct cmfs_dir_block_trailer);
	trailer->db_blkno = blkno;
	trailer->db_parent_dinode = di->i_blkno;
}

/*
 * No need to swap name_len and file_type
 * because they are only 1 byte
 */
static void cmfs_swap_dir_entry(struct cmfs_dir_entry *dirent)
{
	if (cpu_is_little_endian)
		return;

	dirent->inode = bswap_64(dirent->inode);
	dirent->rec_len = bswap_64(dirent->rec_len);
}

/*
 * DIRENT_HEADER_SIZE:
 * - sizeof (__le64 inode)	 8 bytes
 * - sizeof (__le16 rec_len)	 2 bytes
 * - sizeof (uint8_t name_len)	 1 byte
 * - sizeof (uint8_t file_type)	 1 bytes
 * total:			12 bytes
 */
#define DIRENT_HEADER_SIZE 12
static errcode_t cmfs_swap_dir_entries_direction(void *buf,
						uint64_t bytes,
						int to_cpu)
{
	char *p, *end;
	struct cmfs_dir_entry *dirent;
	unsigned int name_len, rec_len;
	errcode_t retval = 0;

	p = (char *)buf;
	end = (char *)(buf + bytes);
	while (p < (end - DIRENT_HEADER_SIZE)) {
		dirent =(struct cmfs_dir_entry *)p;

		if (to_cpu)
			cmfs_swap_dir_entry(dirent);
		name_len = dirent->name_len;
		rec_len = dirent->rec_len;
		if (!to_cpu)
			cmfs_swap_dir_entry(dirent);

		/*
		 * rec_len should >= DIRENT_HEADER_SIZE,
		 * and 4 bytes aligned
		 */
		if ((rec_len < DIRENT_HEADER_SIZE) ||
		    (rec_len % 4)) {
			rec_len = DIRENT_HEADER_SIZE;
			retval = CMFS_ET_DIR_CORRUPTED;
		}

		if (((name_len & 0xff) + DIRENT_HEADER_SIZE) > rec_len )
			retval = CMFS_ET_DIR_CORRUPTED;
		p += rec_len;
	}
	return retval;
}

errcode_t cmfs_swap_dir_entries_from_cpu(void *buf, uint64_t bytes)
{
	return cmfs_swap_dir_entries_direction(buf, bytes, 0);
}

errcode_t cmfs_swap_dir_entries_to_cpu(void *buf, uint64_t bytes)
{
	return cmfs_swap_dir_entries_direction(buf, bytes, 1);
}

errcode_t cmfs_read_dir_block(cmfs_filesys *fs,
			      struct cmfs_dinode *di,
			      uint64_t block,
			      void *buf)
{
	return -1;
}
