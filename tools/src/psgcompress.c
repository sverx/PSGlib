#include <stdio.h>
#include <string.h>
#include "growbuf.h"

#define COMPRESSION_MAXLEN	51
#define MIN_COMPRESSION		4
#define PSG_LOOPMARKER  0x01

extern int g_verbose;

static int searchPastString(const unsigned char *past, int past_length, const unsigned char *data, int maxlen, int min_length, int *matchOffset)
{
	int testlength;
	int startoffset;
	int bestlength=0;
	int bestoffset=0;
	int matchesFound=0;

	// Try all supported compressed block length, 4 to 51 (or less, if past_length or maxlen are small)
	for (testlength = min_length; testlength < COMPRESSION_MAXLEN && testlength < past_length && testlength < maxlen; testlength++)
	{
		if (memchr(data, PSG_LOOPMARKER, testlength)) {
			// if testlength now encompasses the loop marker, stop here, since the loop marker cannot be
			// in a compressed block.
			break;
		}

		// Now check if the past data contains a sequence identical to the first 'testlength' bytes of 'data'
		matchesFound = 0;
		for (startoffset = 0; ((startoffset + testlength) < past_length); startoffset++) {
			if (memcmp(past + startoffset, data, testlength) == 0) {
				bestlength = testlength;
				bestoffset = startoffset;
				matchesFound = 1;
				// Since testlength is always increasing, the match here is always the best so far,
				// and as further matches in this inner loop would also be of "testlength" length (i.e. never longer),
				// we can break out from the loop. (length then increases in the outer loop)
				break;
			}
		}

		// If no match is found for a given length, there cannot be a match for a longer lengths, so stop here.
		if (!matchesFound) {
			break;
		}
	}

	if (matchOffset) {
		*matchOffset = bestoffset;
	}

	return bestlength;
}

static struct growbuf *compressPSG_min(struct growbuf *gb, int minimum_substring_length)
{
	struct growbuf *gb_out;
	int pos;
	int length;
	int matchoffset;
	unsigned char tmp[3];

	if (minimum_substring_length < MIN_COMPRESSION)
		return NULL;

	gb_out = growbuf_alloc(gb->count);
	if (!gb_out)
		return NULL;

	for (pos=0; pos<gb->count; pos++) {
		if (pos < MIN_COMPRESSION) {
			growbuf_append(gb_out, &gb->data[pos], 1);
			continue;
		}

		// Check how many bytes starting at the current position
		// match something that was already output.
		length = searchPastString(gb_out->data, gb_out->count,
									&gb->data[pos], gb->count - pos,
									minimum_substring_length,
									&matchoffset
							);

		if (length > COMPRESSION_MAXLEN) {
			fprintf(stderr, "internal error\n");
			growbuf_free(gb_out);
			return NULL;
		}

		if (length < minimum_substring_length) {
			// not worth compressing (would result in equal or larger file size)
			growbuf_append(gb_out, &gb->data[pos], 1);
		}
		else {
			tmp[0] = 0x08 + length - MIN_COMPRESSION;
			tmp[1] = matchoffset & 0xff;
			tmp[2] = (matchoffset >> 8) & 0xff;
			growbuf_append(gb_out, tmp, 3);
			pos += length-1; // skip additional bytes "consumed" by compression
		}
	}

	return gb_out;
}

struct growbuf *compressPSG(struct growbuf *gb, int agressive)
{
	int i;
	int best_result = -1;
	struct growbuf *attempt;
	struct growbuf *best = NULL;

	if (!agressive) {
		// According to my testing, the sweet spot is around 8
		return compressPSG(gb, 8);
	}

	if (g_verbose) {
		printf("Looking for best compression...\n");
	}


	for (i=MIN_COMPRESSION; i<COMPRESSION_MAXLEN; i++) {
		attempt = compressPSG_min(gb, i);
		if (!attempt) {
			fprintf(stderr, "out of memory\n");
			return NULL;
		}

		if (g_verbose) {
			printf("%d , %d\n", i, attempt->count);
		}

		if (best == NULL || attempt->count < best_result) {
			best = attempt;
			best_result = attempt->count;
			attempt = NULL;
		} else {
			if (g_verbose) {
				printf("Size started going up - stopping here\n");
			}
			break;
		}
		if (attempt) {
			growbuf_free(attempt);
		}
	}

	return best;
}

