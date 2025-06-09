#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include "growbuf.h"

static void printUsage(void)
{
	printf("Usage: ./psg2txt [options] input.psg\n");
	printf("\n");
	printf("Supported options:\n");
	printf(" -h          Display this usage information\n");
	printf(" -t          Terse output (summary only)\n");
	printf("\n");
}

int main(int argc, char **argv)
{
	uint8_t op;
	int op_offset;
	uint8_t chn, type, data;
	int frameno;
	struct growbuf *psg;
	int pos;
	uint8_t tmp[2];
	int loopoffset = 0;
	int loopset_count = 0;
	const char *in_filename;
	int compressed = 0;
	int end_offset = -1;
	int block_offset, block_length;
	int opt;
	int terse_output = 0;

	while ((opt = getopt(argc, argv, "ht")) != -1) {
		switch (opt)
		{
			case 'h':
				printUsage();
				return 0;
				break;

			case 't':
				terse_output = 1;
				break;

			case '?':
				fprintf(stderr, "Unsupported option. Try -h\n");
				return -1;
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "Missing arguments(s). Try -h\n");
		return -1;
	}

	in_filename = argv[optind];

	psg = growbuf_createFromFile(in_filename);
	if (!psg) {
		return -1;
	}

	printf("* * * psg2txt by raphnet\n");
	printf("Examining %s...\n", in_filename);

	frameno = 0;
	pos = 0;

	while (pos < psg->count)
	{
		// Store offset of new operation
		op_offset = pos;

		// Store operation, advance position
		op = psg->data[pos];
		pos++;

		if (op >= 0x08 && op <= 0x37) {
			// special case for compression (3 bytes)
			if (pos+2 > psg->count) {
				printf("Error: truncated compression command at end of data\n");
				break;
			}
			memcpy(tmp, &psg->data[pos], 2);
			if (!terse_output) {
				printf("[%04x] %02x %02x %02x : Frame %05d : ", op_offset, op, tmp[0], tmp[1], frameno);
			}
		}
		else {
			// Normal case (1 bytes)
			if (!terse_output) {
				printf("[%04x] %02x       : Frame %05d : ", op_offset, op, frameno);
			}
		}

		// %1cct xxxx = Latch/Data byte for SN76489 channel c, type t, data xxxx (4 bits)
		if (op & 0x80) {
			chn = (op >> 5) & 3;
			type = (op >> 4) & 1;
			data = op & 0xf;

			if (terse_output) {
				continue;
			}

			if (type == 1) {
				printf("Channel %d, attenuation=0x%x\n", chn, data);
			} else {
				printf("Channel %d, latch data=0x%x\n", chn, data);
			}
			continue;
		}

		// %01xx xxxx = Data byte for SN76489 latched channel and type, data xxxxxx (6 bits)
		if (op & 0x40) {
			data = op & 0x3f;
			if (!terse_output) {
				printf("data=0x%02x\n", data);
			}
			// todo: track latched channel and data, show full value here?
			continue;
		}

		// %0011 1nnn - end of frame, wait nnn additional frames (0-7)
		if (op >= 0x38) {
			frameno += 1 + (op & 0x7);

			if (!terse_output) {
				printf("Wait %d frames\n", op & 0x7);
			}
			continue;
		}

		if (op == 0x01) {
			if (!terse_output) {
				printf("Loop marker (offset 0x%04x)\n", op_offset);
			}
			loopset_count++;
			loopoffset = op_offset;
			continue;
		}
		if (op == 0) { // End of data
			if (!terse_output) {
				printf("End of data\n");
			}
			end_offset = op_offset;
			break;
		}
		if (op < 7) {
			printf("Error: Unknown command 0x%02x\n", op);
			break;
		}

		// Compression (values 0x08-0x37)
		compressed = 1;
		if (pos+2 > psg->count) {
			fprintf(stderr, "Error: truncated compression command at end of data\n");
			break;
		}
		memcpy(tmp, &psg->data[pos], 2);
		pos+=2;
		block_offset = tmp[0] | (tmp[1]<<8);
		block_length = op - 0x08 + 4;

		if (!terse_output) {
			printf("Repeat block[%d] at 0x%04x\n", block_length, block_offset);
		}

		// detect invalid out-of-range blocks
		if ((block_offset + block_length) > op_offset) {
			fprintf(stderr, "Error: Compressed block goes past current position\n");
			break;
		}
	}

	if (pos != psg->count) {
		growbuf_free(psg);
		return -1;
	}

	printf("--- Summary ---\n");
	printf("Size: %d\n", psg->count);
	printf("Frames: %d\n", frameno);
	printf("Compressed PSG: %s\n", compressed ? "Yes":"No");
	if (loopset_count == 0) {
		printf("No loop point\n");
	} else {
		printf("Looppoint: 0x%04x (%d)\n", loopoffset, loopoffset);
	}
	if (loopoffset > 1) {
		printf("Warning: Loop point set more than once\n");
	}
	if (end_offset == -1) {
		printf("Warning: No end command\n");
	} else {
		printf("End offset: 0x%04x (%d)\n", end_offset, end_offset);
	}
	if (end_offset+1 != psg->count) {
		printf("Warning: Extra data after end of data marker.\n");
	}

	growbuf_free(psg);
	return 0;
}

