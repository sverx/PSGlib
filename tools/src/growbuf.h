#ifndef _growbuf_h__
#define _growbuf_h__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>

struct growbuf {
	int count;

	uint8_t *data;
	int alloc_size;
};

void growbuf_free(struct growbuf *gb);
struct growbuf *growbuf_alloc(int initial_size);
struct growbuf *growbuf_createFromFile(const char *filename);
void growbuf_append(struct growbuf *gb, const uint8_t *data, int size);
void growbuf_clear(struct growbuf *gb);

void growbuf_add32le(struct growbuf *gb, uint32_t val);
void growbuf_add16le(struct growbuf *gb, uint16_t val);
void growbuf_add8(struct growbuf *gb, uint8_t val);

int growbuf_saveToFile(const struct growbuf *gb, const char *filename);
int growbuf_writeToFPTR(const struct growbuf *gb, FILE *fptr);

#ifdef __cplusplus
}
#endif

#endif // _growbuf_h__
