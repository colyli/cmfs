



struct cmfs_dir_block_trailer *
cmfs_dir_trailer_from_block(ocfs2_filesys *fs, void  *data)
{
	char *p = data;
	p += cmfs_dir_trailer_blk_off(fs);
	return (struct cmfs_dir_block_trailer *)p;

}
