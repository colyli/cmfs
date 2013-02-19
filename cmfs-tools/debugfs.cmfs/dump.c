/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * dump.c
 *
 * dumps CMFS structures
 * (Copied and modified from ocfs2-tools/debugfs.ocfs2/dump.c)
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
 */

#include <stdint.h>

#include <cmfs/byteorder.h>
#include "main.h"
#include "dump.h"

extern struct dbgfs_gbls gbls;

void dump_super_block(FILE *out, struct cmfs_super_block *sb)
{
	int i;
	char *str;
	char buf[PATH_MAX];
	time_t lastcheck;

	fprintf(out, "\tRevision: %u.%u\n",
		sb->s_major_rev_level, sb->s_minor_rev_level);

	fprintf(out, "\tMount Count: %u   Max Mount Count: %u\n",
		sb->s_mnt_count, sb->s_max_mnt_count);

	fprintf(out, "\tState: %u   Errors: %u\n",
		sb->s_state, sb->s_errors);

	lastcheck = (time_t)sb->s_lastcheck;
	str = ctime(&lastcheck);
	fprintf(out, "\tCheck Interval: %u   Last Check: %s",
		sb->s_checkinterval, str);

	fprintf(out, "\tCreator OS: %u\n", sb->s_creator_os);

	get_compat_flag(sb, buf, sizeof(buf));
	fprintf(out, "\tFeature Compat: %u %s\n", sb->s_feature_compat, buf);

	get_incompat_flag(sb, buf, sizeof(buf));
	fprintf(out, "\tFeature Incompat: %u %s\n",
		sb->s_feature_incompat, buf);

	get_rocompat_flag(sb, buf, sizeof(buf));
	fprintf(out, "\tFeature RO compat: %u %s\n",
		sb->s_feature_ro_compat, buf);

	fprintf(out,
		"\tRoot Blknum: %"PRIu64"   System Dir Blknum: %"PRIu64"\n",
		(uint64_t)sb->s_root_blkno, (uint64_t)sb->s_system_dir_blkno);

	fprintf(out, "\tFirst Cluster Group Blknum: %"PRIu64"\n",
		(uint64_t)sb->s_first_cluster_group);

	fprintf(out, "\tBlock Size Bits: %u   Cluster Size Bits: %u\n",
	       sb->s_blocksize_bits, sb->s_clustersize_bits);

	fprintf(out, "\tLabel: %.*s\n",
		CMFS_MAX_VOL_LABEL_LEN, sb->s_label);

	fprintf(out, "\tUUID: ");
	for (i = 0; i < 16; i++)
		fprintf(out, "%02X", sb->s_uuid[i]);
	fprintf(out, "\n");
	fprintf(out, "\tHash: %u (0x%x)\n", sb->s_uuid_hash, sb->s_uuid_hash);

	return ;
}

int dump_dir_entry(struct cmfs_dir_entry *rec,
		   uint64_t blocknr,
		   int offset,
		   int blocksize,
		   char *buf,
		   void *priv_data)
{
	struct list_dir_opts *ls = (struct list_dir_opts *)priv_data;
	char tmp = rec->name[rec->name_len];
	struct cmfs_dinode *di;
	char perms[20];
	char timestr[40];

	rec->name[rec->name_len] = '\0';

	if (!ls->long_opt) {
		fprintf(ls->out, "\t%-15"PRIu64" %-4u %-4u %-2u %s\n",
			(uint64_t)rec->inode,
			rec->rec_len, rec->name_len,
			rec->file_type, rec->name);
	} else {
		memset(ls->buf, 0, ls->fs->fs_blocksize);
		cmfs_read_inode(ls->fs, rec->inode, ls->buf);
		di = (struct cmfs_dinode *)ls->buf;

		inode_perms_to_str(di->i_mode, perms, sizeof(perms));
		inode_time_to_str(di->i_mtime, timestr, sizeof(timestr));

		fprintf(ls->out,
			"\t%-15"PRIu64" %10s %3u %5u %5u %15"PRIu64" %s %s\n",
			(uint64_t)rec->inode, perms, di->i_links_count,
			di->i_uid, di->i_gid,
			(uint64_t)di->i_size, timestr, rec->name);
	}

	rec->name[rec->name_len] = tmp;

	return 0;
}

void dump_dir_block(FILE *out, char *buf)
{
	struct cmfs_dir_entry *dirent;
	int offset = 0, end;
	struct list_dir_opts ls_opts = {
		.fs = gbls.fs,
		.out = out,
	};

	end = gbls.fs->fs_blocksize;

	fprintf(out, "\tEntries\n");
	while (offset < end) {
		dirent = (struct cmfs_dir_entry *)(buf + offset);
		if (((offset + dirent->rec_len) > end) ||
		    (dirent->rec_len < 8) ||
		    ((dirent->rec_len %4) != 0) ||
		    (((dirent->name_len & 0xFF) + 8) > dirent->rec_len)) {
			/* corrupted */
			    return;
		    }
		dump_dir_entry(dirent, 0, offset, gbls.fs->fs_blocksize,
			       NULL, &ls_opts);
		offset += dirent->rec_len;
	}
}

void dump_extent_list(FILE *out, struct cmfs_extent_list *ext)
{
	struct cmfs_extent_rec *rec;
	int i;
	uint64_t num;
	char flags[PATH_MAX];

	fprintf(out, "\tTree Depth: %u   Count: %u   Next Free Rec: %u\n",
		ext->l_tree_depth, ext->l_count, ext->l_next_free_rec);

	if (!ext->l_next_free_rec)
		goto bail;

	if (ext->l_tree_depth)
		fprintf(out, "\t## %-11s   %-12s   %-s\n",
			"Offset", "Cluster", "Block#");
	else
		fprintf(out, "\t## %-11s   %-12s   %-13s   %s\n",
			"Offset", "Clusters", "Block#", "Flags");

	for (i = 0; i < ext->l_next_free_rec; ++i) {
		rec = &(ext->l_recs[i]);

		num = cmfs_rec_clusters(gbls.fs, ext->l_tree_depth, rec);

		if (ext->l_tree_depth) {
			fprintf(out, "\t%-2d %-11lu   %-12lu   %"PRIu64"\n",
				i,
				(uint64_t)rec->e_cpos,
				num,
				(uint64_t)rec->e_blkno);
		} else {
			flags[0] = '\0';
			if (cmfs_snprint_extent_flags(flags,
						      PATH_MAX,
						      rec->e_flags))
				flags[0] = '\0';
			fprintf(out,
				"\t%-2d %-11"PRIu64"   %-12"PRIu64"u   "
				"%-13"PRIu64"   0x%x %s\n",
				i,
				(uint64_t)rec->e_cpos,
				num,
				(uint64_t)rec->e_blkno,
				rec->e_flags,
				flags);
		}
	}
bail:
	return;
}

void dump_extent_block(FILE *out, struct cmfs_extent_block *blk)
{
	fprintf(out, "\tSuballoc Bit: %u   SubAlloc Slot: %u\n",
		blk->h_suballoc_bit, blk->h_suballoc_slot);

	fprintf(out, "\tBlknum: %"PRIu64"   Next Leaf: %"PRIu64"\n",
		(uint64_t)blk->h_blkno, (uint64_t)blk->h_next_leaf_block);
	return;
}

void dump_fast_symlink(FILE *out, char *link)
{
	fprintf(out, "\tFast Symlink Destination: %s\n", link);
	return;
}

void dump_frag(FILE *out,
	       uint64_t ino,
	       uint32_t clusters,
	       uint32_t extents)
{
	float frag_level = 0;
	int clusters_per_mb =
		cmfs_clusters_in_bytes(gbls.fs, CMFS_MAX_CLUSTERSIZE);

	if (clusters > 1 && extents) {
		float e = extents, c= clusters;
		frag_level = 100 * (e/c);
	}

	fprintf(out, "Inode: %"PRIu64"\t%% fragmented: %.2f\tclusters:"
		" %u\textents: %u\tscore: %.0f\n",
		ino, frag_level, clusters, extents,
		frag_level * clusters_per_mb);
}

void dump_group_descriptor(FILE *out,
			   struct cmfs_group_desc *grp,
			   int index)
{
	int max_contig_free_bits = 0;

	if (!index) {
		fprintf(out, "\tGroup Chain: %u   Parent Inode: %"PRIu64
			"   Generation: %u\n",
			grp->bg_chain,
			(uint64_t)grp->bg_parent_dinode,
			grp->bg_generation);
		fprintf(out,
			"\t##   %-15s   %-6s   %-6s   %-6s   %-6s   %-6s\n",
			"Block#", "Total", "Used", "Free", "Contig", "Size");
	}

	find_max_contig_free_bits(grp, &max_contig_free_bits);

	fprintf(out,
		"\t%-2d   %-15"PRIu64"   %-6u   %-6u   %-6u   %-6u   %-6u\n",
		index, (uint64_t)grp->bg_blkno, grp->bg_bits,
		(grp->bg_bits - grp->bg_free_bits_count),
		grp->bg_free_bits_count, max_contig_free_bits, grp->bg_size);

	return;
}

void dump_inode(FILE *out, struct cmfs_dinode *in)
{
	struct passwd *pw;
	struct group *gr;
	char *str;
	uint16_t mode;
	GString *flags = NULL;
	GString *dyn_features = NULL;
	char tmp_str[30];
	struct timespec ts;
	char timebuf[50];
	time_t tm;

	if (S_ISREG(in->i_mode))
		str = "Regular";
	else if (S_ISDIR(in->i_mode))
		str = "Directory";
	else if (S_ISCHR(in->i_mode))
		str = "Char Device";
	else if (S_ISBLK(in->i_mode))
		str = "Block Device";
	else if (S_ISFIFO(in->i_mode))
		str = "FIFO";
	else if (S_ISLNK(in->i_mode))
		str = "Symbolic Link";
	else if (S_ISSOCK(in->i_mode))
		str = "Socket";
	else
		str = "Unknown";

	mode = in->i_mode & (S_IRWXU | S_IRWXG | S_IRWXO);

	flags = g_string_new(NULL);
	if (in->i_flags & CMFS_VALID_FL)
		g_string_append(flags, "Valid ");
	if (in->i_flags & CMFS_SYSTEM_FL)
		g_string_append(flags, "System ");
	if (in->i_flags & CMFS_SUPER_BLOCK_FL)
		g_string_append(flags, "Superblock ");
	if (in->i_flags & CMFS_LOCAL_ALLOC_FL)
		g_string_append(flags, "Localalloc ");
	if (in->i_flags & CMFS_BITMAP_FL)
		g_string_append(flags, "Allocbitmap ");
	if (in->i_flags & CMFS_JOURNAL_FL)
		g_string_append(flags, "Journal ");
	if (in->i_flags & CMFS_CHAIN_FL)
		g_string_append(flags, "Chain ");

	fprintf(out,
		"\tInode: %"PRIu64"   Mode: 0%0o   Generation: %u (0x%x)\n",
		(uint64_t)in->i_blkno, mode,
		in->i_generation, in->i_generation);

	fprintf(out,
		"\tFS Generation: %u (0x%x)\n",
		in->i_fs_generation, in->i_fs_generation);

	fprintf(out,
		"\tType: %s   Attr: 0x%x   Flags: %s\n",
		str, in->i_attr, flags->str);

	fprintf(out,
		"\tDynamic Features: (0x%x) %s\n",
		in->i_dyn_features, dyn_features->str);

	pw = getpwuid(in->i_uid);
	gr = getgrgid(in->i_gid);
	fprintf(out,
		"\tUser: %d (%s)   Group: %d (%s)   Size: %"PRIu64"\n",
		in->i_uid, (pw ? pw->pw_name : "unknown"),
		in->i_gid, (gr ? gr->gr_name : "unknown"),
		(uint64_t)in->i_size);

	fprintf(out,
		"\tLinks: %u   Clusters: %u\n",
		in->i_links_count, in->i_clusters);

#define SHOW_TIME(str, sec, nsec)						\
	do {									\
		ts.tv_sec = (sec); ts.tv_nsec = (nsec);				\
		ctime_nano(&ts, timebuf, sizeof(timebuf));			\
		fprintf(out, "\t%s: 0x%"PRIx64" 0x%x -- %s", (str), (uint64_t)(sec),\
			(nsec), timebuf);					\
	} while (0)

	SHOW_TIME("ctime", in->i_ctime, in->i_ctime_nsec);
	SHOW_TIME("atime", in->i_atime, in->i_atime_nsec);
	SHOW_TIME("mtime", in->i_mtime, in->i_mtime_nsec);
#undef SHOW_TIME

	tm = (time_t)in->i_dtime;
	fprintf(out, "\tdtime: 0x%"PRIx64" -- %s", (uint64_t)tm, ctime(&tm));

	fprintf(out, "\tRefcount Block: %"PRIu64"\n",
		(uint64_t)in->i_refcount_loc);

	fprintf(out, "\tLast Extblk: %"PRIu64"\n",
		(uint64_t)in->i_last_eb_blk);
	if (in->i_suballoc_slot == (uint16_t)CMFS_INVALID_SLOT)
		strcpy(tmp_str, "Global");
	else
		sprintf(tmp_str, "%d", in->i_suballoc_slot);
	fprintf(out, "\tSub Alloc Slot: %s   Sub Alloc Bit: %u",
		tmp_str, in->i_suballoc_bit);

	if (in->i_suballoc_loc)
		fprintf(out, "   Sub Alloc Group %"PRIu64"\n",
			(uint64_t)in->i_suballoc_loc);
	else
		fprintf(out, "\n");

	if (in->i_flags & CMFS_BITMAP_FL)
		fprintf(out, "\tBitmap Total: %u   Used: %u   Free: %u\n",
		       in->id1.bitmap1.i_total, in->id1.bitmap1.i_used,
		       (in->id1.bitmap1.i_total - in->id1.bitmap1.i_used));

	if (in->i_flags & CMFS_JOURNAL_FL) {
		fprintf(out, "\tJournal Flags: ");
		if (in->id1.journal1.ij_flags & CMFS_JOURNAL_DIRTY_FL)
			fprintf(out, "Dirty ");
		fprintf(out, "\n");
		fprintf(out, "\tRecovery Generation: %u\n",
			in->id1.journal1.ij_recovery_generation);
	}

	if (flags)
		g_string_free(flags, 1);
	if (dyn_features)
		g_string_free(dyn_features, 1);
	return ;
}

void dump_chain_list(FILE *out, struct cmfs_chain_list *cl)
{
	struct cmfs_chain_rec *rec;
	int i;

	fprintf(out, "\tClusters per Group: %u   Bits per Cluster: %u\n",
		cl->cl_cpg, cl->cl_bpc);

	fprintf(out, "\tCount: %u   Next Free Rec: %u\n",
		cl->cl_count, cl->cl_next_free_rec);

	if (!cl->cl_next_free_rec)
		goto bail;

	fprintf(out, "\t##   %-10s   %-10s   %-10s   %s\n",
		"Total", "Used", "Free", "Block#");
	
	for (i = 0; i < cl->cl_next_free_rec; ++i) {
		rec = &(cl->cl_recs[i]);
		fprintf(out, "\t%-2d   %-10u   %-10u   %-10u   %"PRIu64"\n",
			i, rec->c_total, (rec->c_total - rec->c_free),
			rec->c_free, (uint64_t)rec->c_blkno);
	}

bail:
	return ;
}

void dump_local_alloc(FILE *out, struct cmfs_local_alloc *loc)
{
	fprintf(out,
		"\tLocal Bitmap Offset: %u   Size: %u\n",
		loc->la_bm_off, loc->la_size);

	return;
}

void dump_logical_blkno(FILE *out, uint64_t blkno)
{
	fprintf(out, "\t%"PRIu64"\n", blkno);
	return;
}




















































