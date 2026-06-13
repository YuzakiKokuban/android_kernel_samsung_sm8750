/* Copyright (c) 2018, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <assert.h>
#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <libfdt.h>

#include "util.h"

/* Usage related data. */
static const char usage_synopsis[] =
	"merge a number of overlays\n"
	"	fdtoverlaymerge <options> [<overlay.dtbo> [<overlay.dtbo>]]\n"
	"\n"
	USAGE_TYPE_MSG;
static const char usage_short_opts[] = "i:o:v" USAGE_COMMON_SHORT_OPTS;
static struct option const usage_long_opts[] = {
	{"input",            required_argument, NULL, 'i'},
	{"output",	     required_argument, NULL, 'o'},
	{"verbose",	           no_argument, NULL, 'v'},
	USAGE_COMMON_LONG_OPTS,
};
static const char * const usage_opts_help[] = {
	"Input base overlay DT blob",
	"Output DT blob",
	"Verbose messages",
	USAGE_COMMON_OPTS_HELP
};

int verbose = 0;

static void grow_blob(char **blob, off_t extra_len)
{
	int blob_len;

	if (!extra_len)
		return;

	blob_len = fdt_totalsize(*blob) + extra_len;
	*blob = xrealloc(*blob, blob_len);
	fdt_open_into(*blob, *blob, blob_len);
}

static int reload_blob(char *filename, char **blob, off_t extra_len)
{
	free(*blob);
	*blob = utilfdt_read(filename, NULL);
	if (!*blob) {
		fprintf(stderr, "Failed to reload blob %s\n", filename);
		return -1;
	}

	grow_blob(blob, extra_len);

	return 0;
}

static int do_fdtoverlay_merge(const char *input_filename,
			 const char *output_filename,
			 int argc, char *argv[])
{
	char *blob = NULL;
	char **ovblob = NULL;
	size_t ov_len, blob_len, total_len, extra_blob_len = 0;
	size_t *extra_ov_len;
	int i, ret = -1;

	/* allocate blob pointer array */
	ovblob = xmalloc(sizeof(*ovblob) * argc);
	memset(ovblob, 0, sizeof(*ovblob) * argc);

	extra_ov_len = xmalloc(sizeof(*extra_ov_len) * argc);
	memset(extra_ov_len, 0, sizeof(*extra_ov_len) * argc);

reload_all_blobs:
	/* Free existing buffer first */
	free(blob);
	blob = utilfdt_read(input_filename, &blob_len);
	if (!blob) {
		fprintf(stderr, "\nFailed to read base blob %s\n",
				input_filename);
		goto out_err;
	}
	if (fdt_totalsize(blob) > blob_len) {
		fprintf(stderr,
		 "\nBase blob is incomplete (%lu / %" PRIu32 " bytes read)\n",
			(unsigned long)blob_len, fdt_totalsize(blob));
		goto out_err;
	}
	ret = 0;

	/* read and keep track of the overlay blobs */
	total_len = extra_blob_len;
	for (i = 0; i < argc; i++) {
		/* Free existing buffer first */
		free(ovblob[i]);
		ovblob[i] = utilfdt_read(argv[i], &ov_len);
		if (!ovblob[i]) {
			fprintf(stderr, "\nFailed to read overlay %s\n",
					argv[i]);
			goto out_err;
		}
		grow_blob(&ovblob[i], extra_ov_len[i]);
		total_len += ov_len + extra_ov_len[i];
	}

	/* grow the blob to worst case */
	grow_blob(&blob, total_len);

	/* apply the overlays in sequence */
	for (i = 0; i < argc; i++) {
		do {
			int fdto_nospace;

			fprintf(stderr, "Merging overlay blob %s\n", argv[i]);
			ret = fdt_overlay_merge(blob, ovblob[i], &fdto_nospace);
			if (ret && ret == -FDT_ERR_NOSPACE) {
				if (fdto_nospace) {
					extra_ov_len[i] += 512;
					fprintf(stderr, "Reloading overlay blob %s\n", argv[i]);
					ret = reload_blob(argv[i], &ovblob[i], extra_ov_len[i]);
					if (!ret)
						continue;
				} else {
					extra_blob_len += 512;
					fprintf(stderr, "Reloading all blobs\n");
					goto reload_all_blobs;
				}
			}

			if (!ret)
				break;

			if (ret) {
				fprintf(stderr, "\nFailed to merge %s (%d)\n",
						argv[i], ret);
				goto out_err;
			}
		} while (1);
	}

	fdt_pack(blob);
	ret = utilfdt_write(output_filename, blob);
	if (ret)
		fprintf(stderr, "\nFailed to write output blob %s\n",
				output_filename);

out_err:
	if (ovblob) {
		for (i = 0; i < argc; i++) {
			if (ovblob[i])
				free(ovblob[i]);
		}
		free(ovblob);
	}
	free(blob);

	return ret;
}

int main(int argc, char *argv[])
{
	int opt, i;
	char *input_filename = NULL;
	char *output_filename = NULL;

	while ((opt = util_getopt_long()) != EOF) {
		switch (opt) {
		case_USAGE_COMMON_FLAGS

		case 'i':
			input_filename = optarg;
			break;
		case 'o':
			output_filename = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
		}
	}

	if (!input_filename)
		usage("missing input file");

	if (!output_filename)
		usage("missing output file");

	argv += optind;
	argc -= optind;

	if (argc <= 0)
		usage("missing overlay file(s)");

	if (verbose) {
		printf("input  = %s\n", input_filename);
		printf("output = %s\n", output_filename);
		for (i = 0; i < argc; i++)
			printf("overlay[%d] = %s\n", i, argv[i]);
	}

	if (do_fdtoverlay_merge(input_filename, output_filename, argc, argv))
		return 1;

	return 0;
}
