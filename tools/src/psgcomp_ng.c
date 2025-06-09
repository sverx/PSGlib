#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <ctype.h>
#include "psgcompress.h"

int g_verbose = 0;

static void printUsage(void)
{
	printf("Usage: ./psgcomp_ng [options] inputfile.psg output.psgcomp\n");
	printf("\n");
	printf("Supported options:\n");
	printf(" -h          Display this usage information\n");
	printf(" -v          Verbose output\n");
	printf("\n");
}

int main(int argc, char **argv)
{
	struct growbuf *gb = NULL;
	struct growbuf *gb_compressed = NULL;
	const char *in_filename = NULL;
	const char *out_filename = NULL;
	int exit_value = -1;
	int opt;

	while ((opt = getopt(argc, argv, "hcvi:")) != -1) {
		switch (opt)
		{
			case 'h':
				printUsage();
				return 0;
				break;

			case 'v':
				g_verbose = 1;
				break;
		}
	}

	if (optind >= argc-1) {
		fprintf(stderr, "Missing arguments(s). Try -h\n");
		return -1;
	}

	in_filename = argv[optind];
	out_filename = argv[optind+1];

	if (g_verbose) {
		printf(" * * * raphnet's vgmcomp_ng\n");
		printf("Input file: %s\n", in_filename);
		printf("Output file: %s\n", out_filename);
	}

	gb = growbuf_createFromFile(in_filename);
	if (!gb) {
		fprintf(stderr, "Could not load %s\n", in_filename);
		goto error;
	}

	if (g_verbose) {
		printf("Loaded PSG data size: %d\n", gb->count);
		printf("Compressing...\n");
	}

	gb_compressed = compressPSG(gb, 1);
	if (!gb_compressed) {
		fprintf(stderr, "Compression failed\n");
		goto error;
	}

	if (g_verbose) {
		printf("Compressed PSG size: %d\n", gb_compressed->count);
	}


	growbuf_saveToFile(gb_compressed, out_filename);
	exit_value = 0;

error:
	if (gb) {
		growbuf_free(gb);
	}
	if (gb_compressed) {
		growbuf_free(gb_compressed);
	}

	return exit_value;
}

