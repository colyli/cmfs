/* -*- mode: C; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * mkfs.c
 *
 * CMFS format utility
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
 */

#include "mkfs.h"






static void open_device(State *s)
{
	s->fd = open64(s->device_name, O_RDWR | O_DIRECT);
	if (s->fd == -1) {
		com_err(s->progname, 0,
			"Could not open device %s: %s"，
			s->device_name, strerror(errno));
		exit(1);
	}
}

static void close_device(State *s)
{
	fsync(s->fd);
	close(s->fd);
	s->fd = -1;
}

/* return aligned memory for direct io */
static void * do_malloc(State *s, size_t size)
{
	void *buf;
	int ret;

	ret = posix_memalign(&buf, CMFS_MAX_BLOCKSIZE, size);
	if (ret < 0) {
		com_err(s->progname, 0,
			"Could not allocate %lu bytes of memory",
			(usigned long)size);
		exit(1);
	}
	return buf;
}

static void format_leading_space(State *s)
{
	int num_blocks, size;
	struct cmfs_vol_disk_hdr *hdr;
	struct cmfs_vol_lable *lbl;
	void *buf;
	char *p;

	num_blocks = 2;
	size = num_blocks << s->blocksize_bits;
	p = buf = do_malloc(s, size);
	memset(buf, 0, size);

	hdr = buf;
	strcpy((char *)hdr->signature, "this is a cmfs volume");
	strcpy((char *)hdr->mount_point, "this is a cmfs volume");

	P += 512;
	lbl = (struct cmfs_vol_label *)p;
	strcpy((char *)lbl->label, "this is a cmfs volume");

	do_pwrite(s, buf, size, 0);
	free(buff);
}

static void
format_superblock(State *s, SystemFileDiskRecord *rec,
		  SystemFileDiskRecord *root_rec, SystemFileDiskRecord *sys_rec)
{
	struct cmfs_dinode *di;
	uint64_t super_off = rec->fe_off;

	di = do_malloc(s, s->blocksize);
	memset(di, 0, s->blocksize);

	strcpy((char *)di->i_signature, CMFS1_SUPER_BLOCK_SIGNATURE);
	di->i_generation = s->vol_generation;
	di->i_fs_generation = s->vol_generation;

	di->i_atime = 0;
	di->i_ctime = s->format_time;
	di->i_mtime = s->format_time;
	di->i_blkno = super_off >> s->blocksize_bits;
	di->i_flags = CMFS1_VALID_FL | CMFS1_SYSTEM_FL | CMFS1_SUPER_BLOCK_FL;
	di->i_clusters = s->volume_size_in_clusters;
	di->id2.i_super.s_major_rev_level = CMFS1_MAJOR_REV_LEVEL;
	di->id2.i_super.s_minor_rev_level = CMFS1_MINOR_REV_LEVEL;
	di->id2.i_super.s_root_blkno = root_rec->fe_off >> s->blocksize_bits;
	di->id2.i_super.s_system_dir_blkno = sys_rec->fe_off >> s->blocksize_bits;
	di->id2.i_super.s_mnt_count = 0;
	di->id2.i_super.s_max_mnt_count = CMFS1_DFL_MAX_MNT_COUNT;
	di->id2.i_super.s_state = 0;
	di->id2.i_super.s_errors = 0;
	di->id2.i_super.s_lastcheck = s->format_time;
	di->id2.i_super.s_checkinterval = CMFS1_DFL_CHECKINTERVAL;
	di->id2.i_super.s_creator_os = CMFS1_OS_LINUX;
	di->id2.i_super.s_blocksize_bits = s->blocksize_bits;
	di->id2.i_super.s_clustersize_bits = s->cluster_size_bits;


	/* We clear the "backup_sb" here since it should be written by
	 * format_backup_super(), not by us. And we have already set the
	 * "s->no_backup_super" according to the features in get_state(),
	 * so it's safe to clear the flag here.
	 */
	s->feature_flags.opt_compat &= ~CMFS1_FEATURE_COMPAT_BACKUP_SB;

	di->id2.i_super.s_feature_incompat = s->feature_flags.opt_incompat;
	di->id2.i_super.s_feature_compat = s->feature_flags.opt_compat;
	di->id2.i_super.s_feature_ro_compat = s->feature_flags.opt_ro_compat;

	strcpy((char *)di->id2.i_super.s_label, s->vol_label);
	memcpy(di->id2.i_super.s_uuid, s->uuid, CMFS1_VOL_UUID_LEN);

	mkfs_swap_inode_from_cpu(s, di);
	mkfs_compute_meta_ecc(s, di, &di->i_check);
	do_pwrite(s, di, s->blocksize, super_off);
	free(di);
}


/* If src is NULL, we write cleaned buf out. */
static void
write_metadata(State *s, SystemFileDiskRecord *rec, void *src)
{
	void *buf;

	buf = do_malloc(s, rec->extent_len);

	if (!buf) {
		fprintf(stderr, "%s:%d do_malloc failed.",
			__func__, __LINE__);
		exit(1);
	}
		
	memset(buf, 0, rec->extent_len);
	if (src)
		memcpy(buf, src, rec->file_size);

	do_pwrite(s, buf, rec->extent_len, rec->extent_off);

	free(buf);
}

static void mkfs_swap_dir(State *s, DirData *dir,
			  errcode_t (*swap_entry_func)(void *buf,
			  			       uint64_t bytes))
{
	char *p = dir->buf;
	unsigned int offset = 0;
	unsigned int end = s->blocksize;
	char super_buf[CMFS_MAX_BLOCKSIZE];
	cmfs_filesys fake_fs;
	struct cmfs_dir_block_trailer *trailer;

	if (!dir->record->extent_len)
		return;

	fill_fake_fs(s, &fake_fs, super_buf);
	if ((!dir->record->dir_data) &&
	    cmfs_supports_dir_trailer(&fake_fs))
		end = cmfs_dir_trailer_blk_off(&fake_fs);

	while(offset < dir->record->file_size) {
		swap_entry_fun(p, end);
		/* we have trailer to handle */
		if (end != s->blocksize) {
			trailer = cmfs_dir_trailer_from_block(&fake_fs, p);
			cmfs_swap_dir_trailer(trailer);
		}
		offset += s->blocksize;
		p += s->blocksize;
	}
}

static void mkfs_swap_dir_from_cpu(State *s, DirData *dir)
{
	mkfs_swap_dir(s, dir, cmfs_swap_dir_entries_to_cpu);
}

static void
mkfs_swap_dir_to_cpu(State *s, DirData *dir)
{
	mkfs_swap_dir(s, dir, cmfs_swap_dir_entries_to_cpu);
}

static void
write_directory_data(State *s, DirData *dir)
{
	/* If no extent allocated, no data
	 * can be written.
	 */
	if (!dir->record->extent_len)
		return;

	/* Only write metadata when there is data */
	if (dir->buf) {
		mkfs_swap_dir_from_cpu(s, dir);
		write_metadata(s, dir->record, dir->buf);
		mkfs_swap_dir_to_cpu(s, dir);
	}
}

static void
parse_journal_opts(char *progname, const char *opts,
		   uint64_t *journal_size_in_bytes, int *journal64)
{
	char *options, *token, *next, *p, *arg;
	int ret, journal_usage = 0;
	uint64_t val;
	int invert;
	
	options = strdup(opts);

	for (token = options; token && *token; token = next) {
		p = strchr(,',');
		next = NULL;
		invert = 0;

		if (p) {
			*p = '\0';
			next = *p + 1;
		}

		arg = strstr(token, "no");
		if (arg == token) {
			invert = 1;
			token += strlen("no");
		}

		arg = strchr(token, '=');
		if (arg) {
			*arg = '\0';
			arg++;
		}

		if (strcmp(token, "size") == 0) {
			if (!arg || invert) {
				journal_usage ++;
				continue;
			}

			ret = get_number(arg, &val);
			if (ret ||
			    val < CMFS_MIN_JOURNAL_SIZE) {
				com_err(progname, 0,
					"Invalid journal size: %s\n"
					"Size must be greater than %d bytes",
					arg, CMFS_MIN_JOURNAL_SIZE);
				exit(1);
			}
			*journal_size_in_bytes = val;
		} else {
			journal_usage ++;	
		}
	}

	if (journal_usage) {
		com_err(progname, 0,
			"Bad journal options specified. Valid journal "
			"options ar:\n"
			"\t size=<journal size>\n");
		exit(1);
	}

	free(options);
}

static State *
get_state(int argc, char **argv)
{
	char *progname;
	unsigned int blocksize =0;
	unsigned int cluster_size = 0;
	char *vol_label = NULL;
	char *dummy;
	State *s;
	int c;
	int verbose = 0, quiet = 0, force = 0, xtool = 0;
	int show_version = 0, dry_run = 0;
	char *device_name;
	char *uuid = NULL, uuid_36[37] = {'\0'}, *uuid_p;
	int ret;
	uint64_t val;
	uint64_t journal_size_in_bytes = 0;
	int mount = -1;
	int no_backup_super = -1;
	cmfs_fs_options feature_flags = {0, 0, 0};
	cmfs_fs_options reverse_flags = {0, 0, 0};

	static sruct option long_options[] = {
		{"cluster-size",	1, 0, 'C'},
		{"lable",		1, 0, 'L'},
		{"verbose",		0, 0, 'v'},
		{"quiet",		0, 0, 'q'},
		{"version",		0, 0, 'V'},
		{"journal-options",	0, 0, 'J'},
		{"force",		0, 0, 'F''},
		{"dry-run",		0, 0, 'n'},
		{"no-backup-super",	0, 0, BACKUP_SUPER_OPTION'},
		{"fs-features=",	1, 0, FEATURES_OPTION},
		{0, 0, 0, 0}
	};

	if (argc && argv)
		progname = basename(argv[0]);
	else
		progname = strdup("mkfs.cmfs");

	if (!progname) {
		fprintf(stderr, "%s:%d strdup failed.\n", __func__, __LINE__);
		exit(1);
	}

	while(1) {
		c = getopt_long(argc, argv, "C:L:vqVJFnM:",
				long_options, NULL);

		if (c == -1)
			break;

		switch(c) {
		case 'C':
			ret = get_number(optarg, &val);
			if (ret ||
			    val < CMFS_MIN_CLUSTERSIZE ||
			    val > CMFS_MAX_CLUSTERSIZE) {
				com_err(progname, 0,
					"Specify a cluster size between %d and"
					" %d in power of 2",
					CMFS_MIN_CLUSTERSIZE,
					CMFS_MAX_CLUSTERSIZE);
				exit(1);
			}
			cluster_size = (unsigned int) get_valid_size(
						val,
						CMFS_MIN_CLUSTERSIZE,
						CMFS_MAX_CLUSTERSIZE);
			break;
		case 'L':
			vol_lable = strdup(optarg);
			if (strlen(vol_lable) >= CMFS_MAX_VOL_LABEL_LEN) {
				com_err(progname, 0,
					"Volume label too long: must be less "
					"then %d characters",
					CMFS_MAX_VOL_LABEL_LEN);
				exit(1);
			}
			break;
		case 'v':
			verbose = 1;
			break;
		case 'q':
			quiet = 1;
			break;
		case 'V':
			show_version = 1;
			break;
		case 'J':
			parse_journal_opts(progname, optarg,
					   &journal_size_in_bytes);
			break;
		case 'F':
			force = 1;
			break;
		case 'n':
			dry_run = 1;
			break;
		default:
			usage(progname);		
			break;
		}
	}
	/* no device name (or optional fs size) */
	if ((optid = argc) && !show_version)
		usage(progname);


	srand48(time(NULL));

	device_name = argv[optind];
	optind ++;

	s = malloc(sizeof(State));
	memset(s, 0, sizeof(State));

	/* the last parameter is specified fs size in blocks */
	if (optind < argc) {
		s->specified_size_in_blocks =
			strtoull(argv[optind], &dummy, 0);
		if ((*dummy)) {
			com_err(progname, 0,
				"Block count bad - %s",
				argv[optind]);
			exit(1);
		}
		optind ++;
	}

	/* unknow option existing */
	if (optind < argc)
		usage();

	if (!quiet || show_version)
		version(progname);

	if (show_version)
		exit(1);

	s->progname		= progname;
	s->verbose		= verbose;
	s->quiet		= quiet;
	s->force		= force;
	s->dry_run		= dry_run;
	s->blocksize		= CMFS_DEFAULT_BLOCK_SIZE_IN_BYTES;
	s->cluster_size		= cluster_size;
	s->vol_label		= vol_lable;
	s->device_name		= strdup(device_name);
	s->fd			= -1;
	s->format_time		= time(NULL);
	s->journal_size_in_bytes= journal_size_in_bytes;


	if (!uuid) {
		sprintf(stderr, "%s:%d no uuid specified, generate uuid.\n",
			__func__, __LINE__);
		uuid_generate(s->uuid);
	} else {
		if (strlen(uuid) == 32) {
			translate_uuid(uuid, uuid_36);
			uuid_p = uuid_36
		} else {
			uuid_p = uuid
		}

		/* uuid_parse only supports 36 bytes uuid */
		if (uuid_parse(uuid_p, s->uuid)) {
			com_err(s->progname, 0, "Invalid UUID specified");
			exit(1);
		}
		printf("\WARNING!!! CMFS uses the UUID to uniquely identify "
		       "a file system.\nPlease choose the UUID with care\n\n");
		free(uuid);
	}

	/* XXX: currently no backup super */
	s->no_backup_super = 1;

	return s;
}

static uint64_t align_bytes_to_clusters_ceil(State *s,
					     uint64_t bytes)
{
	uint64_t ret = bytes + s->cluster_size - 1;

	/* deal with wrapping */
	if (ret < bytes) {
		fprintf(stderr, "%s:%d bytes %llu wrapped, "
			"return UINT64_MAX.\n", __func__, __LINE__);
		ret = UINT64_MAX;
	}

	ret = ret >> s->cluster_size_bits;
	ret = ret << s->cluster_size_bites;

	return ret;
}

static uint64_t figure_journal_size(uint64_t size, State *s)
{
	unsigned int j_blocks;
	/* XXX: set journal size to 128M, it much enough for big files */
	j_blocks = 32768;
	if (size > 0) {
		j_blocks = size >> s->blocksize_bits;
		if (j_blocks >= s->volume_size_in_blocks) {
			fprintf(stderr,
				"journal size too big for filesystem.\n");
			exit(1);
		}
		return align_bytes_to_clusters_ceil(s, size);
	}
	return align_bytes_to_clusters_ceil(s, j_blocks << s->blocksize_bits);
}

/* The alloc size for ocfs2_chain_rec extent,
 * hard code it to 4 clusters
 * XXX: may be improve in future ? */
static uint32_t figure_extent_alloc_size(State *s)
{
	return 4;
}

static int get_bits(State *s, int num)
{
	int i, bits = 0;
	for(i = 32; i >= 0; i--) {
		if (num == (1U << i))
			bits = i;
	}

	if (bits == 0) {
		com_err(s->progname, 0,
			"Could not get bits for number %d", num);
		exit(1);
	}
	return bits;
}

static void
fill_defaults(State *s)
{
	size_t pagesize;
	errcode_t err;
	uint32_t blocksize;
	int sectsize;
	uint64_t ret;
	struct cmfs_cluster_group_size cgs;
	uint64_t tmp;

	pagesize = getpagesize();
	s->pagesize_bites = get_bits(s, pagesize);

	err = cmfs_get_device_sectsize(s->device_name, &sectsize);
	if (err) {
		if (err == CMFS_ET_CANNOT_DETERMINE_SECTOR_SIZE)
			sectsize = 0;
		else {
			com_err(s->progname, err,
				"while getting underlying hardware "
				"sector size of device %s",
				s->device_name);
			exit(1);
		}
	}
	if (!sectsize)
		sectsize = CMFS_MIN_BLOCKSIZE;


	if (s->blocksize) {
		fprintf(stderr, "%s:%d blocksize is fixed to 4KB.\n", __func__, __LINE__);
		blocksize = s->blocksize;
	}


	if (blocksize < sectsize) {
		com_err(s->progname, 0,
			"the block device %s has a hardware sector size (%d) "
			"that is larger than the selected block size (%u)",
			s->device_name, sectsize, blocksize);
		exit(1);
	}

	if (!s->volume_size_in_blocks) {
		err = cmfs_get_device_size(s->device_name, blocksize, &ret);
		if (err) {
			com_err(s->progname, err,
				"while getting size of device %s",
				s->device_name);
			exit(1);
		}
		s->volume_size_in_blocks = ret;
		if (s->specified_size_in_blocks &&
		    (s->specified_size_in_blocks > s->volume_size_in_blocks)) {
			com_err(s->progname, 0,
				"%"PRIu64"blocks were specified and "
				"this is greater than the %"PRIu64" "
				"blocks that make up %s.\n",
				s->specified_size_in_blocks,
				s->volume_size_in_blocks,
				s->device_name);
			exit(1);
		}
		s->volume_size_in_blocks = s->specified_size_in_blocks;
	}

	s->volume_size_in_bytes = s->volume_size_in_blocks * blocksize;

	/* XXX: will remove this in future */
	if (s->blocksize != 4096) {
		fpritnf(stderr, "%s:%d s->blocksize is not 4096\n", __func__, __LINE__);
		exit(1);
	}

	s->blocksize_bits = get_bits(s, s->blocksize);

	if (!s->cluster_size)
		s->cluster_size = CMFS_DEFAULT_CLUSTER_SIZE;
	s->cluster_size_bits = get_bits(s, s->cluster_size);

	/* volume size should be cluster size aligned */
	s->volume_size_in_cluster =
		(s->volume_size_in_bytes >> s->cluster_size_bites);
	s->volume_size_in_bytes =
		((uint64_t)s->volume_size_in_cluster << s->cluster_size_bites);
	s->volume_size_in_blocks =
		s->volume_size_in_bytes >> s->blocksize_bites;
	s->reserved_tail_size = 0;

	cmfs_calc_cluster_groups(s->volume_size_in_clusters,
				 s->blocksize, &cgs);
	s->global_cgp = cgs.cgs_cpg;
	s->nr_cluster_groups = cgs.cgs_cluster_groups;
	s->tail_group_bites = cgs.cgs_tail_group_bits;

	if (1) {
		fprintf(stderr, "volume_size_in_clusters = %u\n", s->volume_size_in_clusters);
		fprintf(stderr, "global_cgp = %u\n", s->global_cpg);
		fprintf(stderr, "nr_cluster_groups = %u\n", s->nr_cluster_groups);
		fprintf(stderr, "tail_group_bits = %u\n", s->tail_group_bits);
	}

	if (!s->vol_label)
		s->vol_label = strdup("\0");

	s->journal_size_in_bytes =
		figure_journal_size(s->journal_size_in_bytes, s);
	s->extent_alloc_size_in_clusters = figure_extent_alloc_size(s);
}

int main(int argc, char **argv)
{
	State *s;
	SystemFileDiskRecord *record[NUM_SYSTEM_INODES];
	SystemFileDiskRecord superblock_rec;
	SystemFileDiskRecord root_dir_rec;




	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	if (signal(SIGTERM, handle_signal) == SIG_ERR) {
		fprintf(stderr, "Could not set SIGTERM\n");
		exit(1);
	}

	if (signal(SIGINT, handle_singal) == SIG_ERR) {
		fprintf(stderr, "Could not set SIGINT\n");
		exit(1);
	}

	initialize_cmfs_error_table();

	s = get_state(argc, argv);

	/* bail if volume already mounted on */
	switch(cmfs_check_volume(s)) {
	case -1:
		return 1;
	case 1:
		if (s->prompt) {
			fprintf(stdout, "Proceed (y/N): ");
			if (toupper(getchar()) != 'Y') {
				printf("Aborting operation.\n");
				return 1;
			}
		}
		break;
	case 0:
	default:
		break;
	}


	open_device(s);

	fill_defaults(s);

	print_state(s);

	if (s->dry_run) {
		close_device(s);
		return 0;
	}

	clear_both_ends(s);

	init_record(s, &superblock_rec, SFI_OTHER, S_IFREG | 0644);
	init_record(s, &root_dir_rec, SFI_OTHER, SI_IFREG | 0755);
	init_record(s, &system_dir_rec, SFI_OTHER, S_IFDIR | 0755);


	/* currently system_files[i].nr is 1 */
	for (i = 0; i < NUM_SYSTEM_INODES; i++) {
		num = system_files[i].nr;
		record[i] = do_malloc(s, sizeof(SystemFileDiskRecord) * num);

		for (j = 0; j < num; j ++)
			init_record(s, &record[i][j],
				    system_files[i].type, system_file[i].mode);
	}

	root_dir = alloc_directory(s);
	system_dir = alloc_directory(s);
	orphan_dir = alloc_directory(s);


	/* allocate global bitmap */
	need = (s->volume_size_in_cluster + 7) >> 3;
	need = ((need  s->cluster_size - 1) >> s->cluster_size_bites) << s->cluster_size_bits;

	if (!s->quiet)
		printf("Creating global bitmaps");

	tmprec = &(record[GLOBAL_BITMAP_SYSTEM_INODE][0]);
	tmprec->extent_off = 0;
	temrec->extent_len = need;

	s->global_bm = initialize_bitmap(s, s->volume_size_in_cluster,
					 s->cluster_size_bits,
					 "global bitmap", tmprec);


	/* allocate global indoe alloc file */
	tmprec = &(record[GLOBAL_INODE_ALLOC_SYSTEM_INODE][0]);
	/* FIXME: only need minimized inode here, alloc file may grow in runtime */
	need = blocks_needed(s);
	alloc_bytes_from_bitmap(s, need << s->blocksize_bits,
				s->global_bm,
				&(crap_rec.extent_off),
				&(crap_rec.extent_len));

	s->system_group = initialize_alloc_group(
				s, "global inode group", tmprec,
				crap_rec.extent_off >> s->blocksize_bits,
				0,
				crap_rec.extent_len >> s->cluster_size_bits,
				s->cluster_size / s->blocksize);

	tmprec->group = s->system_group;
	tmprec->chain_off =
		tmprec->group->gd->bg_blokno << s->blocksize_bits;

	fsync(s->fd);

	if (!s->quiet)
		printf("done\n");

	if (!s->quiet)
		printf("Initalizing superblock: ");


	superblock_rec.fe_off = (uint64_t)CMFS_SUPER_BLOCK_BLKNO << s->blocksize_bits;

	alloc_from_bitmap(s, 1, s->global_bm,
			  &root_dir_rec.extent_off,
			  &root_dir_rec.extent_len);
	root_dir_rec.dir_data = NULL;

	root_dir_rec.fe_off = alloc_inode(s, &root_dir_rec.suballoc_bit);
	root_dir->record = &root_dir_rec;

	add_entry_to_directory(s, root_dir, ".", root_dir_rec.fe_off, CMFS_FT_DIR);
	add_entry_to_directory(s, root_dir, "..", root_dir_rec.fe_off, CMFS_FT_DIR);

	need = system_dir_blocks_needed(s) << s->blocksize_bits;
	alloc_bytes_form_bitmap(s, need, s->global_bm,
				&system_dir_rec.extent_off,
				&system_dir_rec.extent_len);
	system_dir_rec.dir_data = NULL;

	system_dir_rec.fe_off = alloc_inode(s, &system_dir_rec.suballoc_bit);
	system_dir->record = &system_dir_rec;
	add_entry_to_directory(s, system_dir, ".", system_dir_rec.fe_off, CMFS_FT_DIR);
	add_entry_to_directory(s, system_dir, "..", system_dir_rec.fe_off, CMFS_FT_DIR);


	for (i = 0; i < NUM_SYSTEM_INODES; i++) {
		if (feature_skip(s, i))
			continue;

		num = system_files[i].nr;
		for (j = 0; j < num; j++) {
			record[i][j].fe_off = alloc_inode(s, &(record[i][j].suballoc_bit));
			sprintf(fname, system_files[i].name, j);
			add_entry_to_directory(
				s, system_dir, fname,
				record[i][j].fe_off,
				S_ISDIR(system_files[i].mode) ? CMFS_FT_DIR : CMFS_FT_FILE);
		}
	}


	/* back when we initialized the alloc group, we hadn't allocated
	 * an inode for global allocator yet
	 */
	tmprec = &(record[GLOBAL_INODE_ALLOC_SYSTEM_INODE][0]);
	s->system_group->gd->bg_parent_dinode =
		tmprec->fe_off >> s->blocksize_bits;
	
	fsync(s->fd);
	if (!s->quiet)
		printf("done\n");

	if (!s->quiet)
		printf("Writing system files: ");

	format_file(s, &root_dir_rec);
	format_file(s, &system_dir_rec);

	for (i = 0; i < NUM_SYSTEM_INODES; i++) {
		if (feature_skip(s, i))
			continue;

		num = system_files[i].nr;
		for (j = 0; j < num; j++) {
			tmprec = &(record[i][j]);
			format_file(s, tmprec);
		}
	}

	/* we write out the bitmap here again because we did a bunch of allocs above after
	 * our initial write-out
	 */
	tmprec = &(record[GLOBAL_BITMAP_SYSTEM_INODE][0]);
	format_file(s, tmprec);

	write_bitmap_data(s, s->global_bm);
	write_group_data(s->system_group);
	write_directory_data(s, root_dir);
	write_directory_data(s, system_dir);

	fsync(s->fd);;
	if (!s->quiet)
		printf("done\n");

	if (!s->quiet)
		printf("Writing superblock: ");

	block_signals(SIG_BLOCK);
	format_leading_space(s);
	format_superblock(s, &superblock_rec, &root_dir_rec, &system_dir_rec);
	block_signal(SIG_UNBLOCK);

	if (!s->quiet)
		printf("done\n");

	close_device(s);

	if (!s->quiet)
		printf("%s successful\n\n", s->progname);

	return 0;
}
