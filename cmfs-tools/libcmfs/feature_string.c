/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * feature_strings.c
 *
 * Routines for analyzing a feature string.
 * (Copied and modified from ocfs2-tools/libocfs2/feature_string.c)
 *
 * Copyright (C) 2007 Oracle.  All rights reserved.
 * CMFS modification, by Coly Li <i@coly.li>
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
#include <string.h>
#include <stdlib.h>

#include <cmfs/cmfs.h>

#include "cmfs_err.h"


struct fs_feature_flags {
	const char *ff_str;
	/* this flag is the feature's own flag. */
	cmfs_fs_options ff_own_flags;
	/*
	 * this flag includes the feature's own flag and
	 * all the other features' flag it depends on.
	 */
	cmfs_fs_options ff_flags;
};

/* Printable names for feature flags */
struct feature_name {
	const char		*fn_name;
	cmfs_fs_options		fn_flag;	/* Only the bit for this
						   feature */
};

struct flag_name {
	const char		*fl_name;
	uint32_t		fl_flag;
};

static cmfs_fs_options mkfs_features_default = {
	CMFS_FEATURE_COMPAT_BACKUP_SB |
	CMFS_FEATURE_COMPAT_HAS_JOURNAL,
	0,
	0
};

/* these are the features we support in mkfs/tunefs via --fs-features */
static struct fs_feature_flags cmfs_supported_features[] = {
	{
		"backup-super",
		{CMFS_FEATURE_COMPAT_BACKUP_SB, 0, 0},
		{CMFS_FEATURE_COMPAT_BACKUP_SB, 0, 0}
	},
	{
		"journal",
		{CMFS_FEATURE_COMPAT_HAS_JOURNAL, 0, 0},
		{CMFS_FEATURE_COMPAT_HAS_JOURNAL, 0, 0}
	},
	{
		NULL,
		{0, 0, 0},
		{0, 0, 0}
	},
};

static int feature_match(cmfs_fs_options *a, cmfs_fs_options *b)
{
	if ((a->opt_compat & b->opt_compat) ||
	    (a->opt_incompat & b->opt_incompat) ||
	    (a->opt_ro_compat & b->opt_ro_compat))
		return 1;
	return 0;
}

static inline void merge_features(cmfs_fs_options *features,
				  cmfs_fs_options supp_features)
{
	features->opt_compat |= supp_features.opt_compat;
	features->opt_incompat |= supp_features.opt_incompat;
	features->opt_ro_compat |= supp_features.opt_ro_compat;
}

/*
 * If we are asked to clear a feature, we also need to
 * clear any other features that depend on it
 */
static void cmfs_feature_clear_deps(cmfs_fs_options *reverse_set)
{
	int i;
	for (i = 0; cmfs_supported_features[i].ff_str; i++) {
		if (feature_match(reverse_set,
				  &cmfs_supported_features[i].ff_flags)) {
			merge_features(reverse_set,
				      cmfs_supported_features[i].ff_own_flags);
		}
	}
}

static int check_feature_flags(cmfs_fs_options *fs_flags,
			       cmfs_fs_options *fs_r_flags)
{
	int ret = 1;

	if (fs_r_flags->opt_compat &&
	    (fs_flags->opt_compat & fs_r_flags->opt_compat))
		ret = 0;
	else if (fs_r_flags->opt_incompat &&
		 (fs_flags->opt_incompat & fs_r_flags->opt_incompat))
		ret = 0;
	else if (fs_r_flags->opt_ro_compat &&
		 (fs_flags->opt_ro_compat & fs_r_flags->opt_ro_compat))
		ret = 0;

	return ret;
}

/*
 * Parse the feature string.
 *
 * For those the user want to clear (with "no" in the beginning),
 * they are stored in "reversed_flags".
 *
 * For those the user want to set, they are store in "feature_flags"
 */
errcode_t cmfs_parse_feature(const char *opts,
			     cmfs_fs_options *feature_flags,
			     cmfs_fs_options *reverse_flags)
{
	char *options, *token, *next, *p, *arg;
	int i, reverse = 0, match = 0;

	memset(feature_flags, 0, sizeof(cmfs_fs_options));
	memset(reverse_flags, 0, sizeof(cmfs_fs_options));

	options = strdup(opts);
	for (token = options; token && *token; token = next) {
		reverse = 0;
		p = strchr(token, ',');
		next = NULL;
		if (p) {
			*p = '\0';
			next = p + 1;
		}

		arg = strstr(token, "no");
		if (arg && arg ==token) {
			reverse = 1;
			token += 2;
		}

		for (i = 0; cmfs_supported_features[i].ff_str; i++) {
			if (strcmp(token,
				   cmfs_supported_features[i].ff_str) == 0) {
				match = 1;
				break;
			}
		}

		if (!match) {
			free(options);
			return CMFS_ET_UNSUPP_FEATURE;
		}

		if (!reverse)
			merge_features(feature_flags,
				cmfs_supported_features[i].ff_flags);
		else
			merge_features(reverse_flags,
				cmfs_supported_features[i].ff_own_flags);
	}

	free(options);
	cmfs_feature_clear_deps(reverse_flags);

	/*
	 * Check whether the user asks for a flag to be set and cleared,
	 * which is illegal. The feature_set and reverse_set are both
	 * set by "--fs-features", so they should not collide with each
	 * other.
	 */
	if (!check_feature_flags(feature_flags, reverse_flags))
		return CMFS_ET_CONFLICTING_FEATURES;

	return 0;
}

/*
 * Check and Merge all the different features set by the user.
 *
 * dest: merged result
 * feature_set: all the features a user set by "--fs-features"
 * reverse_set: all the features a user want to clear by "--fs-features"
 */
errcode_t cmfs_merge_feature_with_default_flags(
				cmfs_fs_options *dest,
				cmfs_fs_options *feature_set,
				cmfs_fs_options *reverse_set)
{
	/*
	 * Ensure that all dependancies are correct in the reverse set.
	 * A reverse set from cmfs_parse_feature() will be correct, but
	 * a hand-built one might not be.
	 */
	cmfs_feature_clear_deps(reverse_set);

	/*
	 * Check whether the user asked for a flag to be set and cleared,
	 * which is illegal. The feature_set and reverse_set are both
	 * set by "--fs-features", so they shoudl not collide with each
	 * other, but a hand-built one might have problem.
	 */
	if (!check_feature_flags(feature_set, reverse_set))
		return CMFS_ET_CONFLICTING_FEATURES;

	/* Now combine all the features the user has set */
	*dest = mkfs_features_default;
	merge_features(dest, *feature_set);

	/* Now clear the reverse set from our destination */
	dest->opt_compat &= ~(reverse_set->opt_compat);
	dest->opt_incompat &= ~(reverse_set->opt_incompat);
	dest->opt_ro_compat &= ~(reverse_set->opt_ro_compat);

	return 0;
}














