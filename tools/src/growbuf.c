#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "growbuf.h"

void growbuf_clear(struct growbuf *gb)
{
	gb->count = 0;
}

void growbuf_free(struct growbuf *gb)
{
	if (gb) {
		if (gb->data) {
			free(gb->data);
		}
		free(gb);
	}
}

struct growbuf *growbuf_alloc(int initial_size)
{
	struct growbuf *gb;

	gb = calloc(1, sizeof(struct growbuf));
	if (!gb) {
		perror("growbuf");
		return NULL;
	}

	gb->data = calloc(1, initial_size);
	if (!gb->data) {
		perror("could not allocate growbuf initial");
		free(gb);
		return NULL;
	}

	gb->alloc_size = initial_size;

	return gb;
}

void growbuf_append(struct growbuf *gb, const uint8_t *data, int size)
{
	uint8_t *d;
	int newsize;

	if (size <= 0)
		return;

	if (gb->count + size > gb->alloc_size) {
		newsize = gb->alloc_size * 2 + size;
		d = realloc(gb->data, newsize);
		if (!d) {
			perror("Could not grow buffer");
			return;
		}
		gb->data = d;
		gb->alloc_size = newsize;
	}

	memcpy(gb->data + gb->count, data, size);
	gb->count += size;
}

void growbuf_add32le(struct growbuf *gb, uint32_t val)
{
	uint8_t tmp[4] = { val, val >> 8, val >> 16, val >> 24 };
	growbuf_append(gb, tmp, 4);
}

void growbuf_add16le(struct growbuf *gb, uint16_t val)
{
	uint8_t tmp[2] = { val, val >> 8 };
	growbuf_append(gb, tmp, 2);
}

void growbuf_add8(struct growbuf *gb, uint8_t val)
{
	uint8_t tmp[2] = { val };
	growbuf_append(gb, tmp, 1);
}

int growbuf_writeToFPTR(const struct growbuf *gb, FILE *fptr)
{
	return fwrite(gb->data, gb->count, 1, fptr);
}

int growbuf_saveToFile(const struct growbuf *gb, const char *filename)
{
	FILE *fptr;

	fptr = fopen(filename, "wb");
	if (!fptr) {
		perror(filename);
		return -1;
	}

	growbuf_writeToFPTR(gb, fptr);

	fclose(fptr);

	return 0;
}

struct growbuf *growbuf_createFromFile(const char *filename)
{
	FILE *fptr;
	long filesize;
	struct growbuf *gb;

	fptr = fopen(filename, "rb");
	if (!fptr) {
		perror(filename);
		return NULL;
	}

	fseek(fptr, 0, SEEK_END);
	filesize = ftell(fptr);
	fseek(fptr, 0, SEEK_SET);

	if (filesize > INT_MAX) {
		fprintf(stderr, "File too large\n");
		return NULL;
	}

	gb = growbuf_alloc(filesize);
	if (!gb) {
		fprintf(stderr, "could not load %s\n", filename);
		fclose(fptr);
		return NULL;
	}

	if (1 != fread(gb->data, filesize, 1, fptr)) {
		perror(filename);
		fclose(fptr);
		growbuf_free(gb);
		return NULL;
	}
	gb->count = filesize;

	fclose(fptr);

	return gb;
}


