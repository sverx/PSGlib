#include <stdio.h>
#include <string.h>

FILE *fIN;

#define BUF_SIZE        65536
#define COMP_MIN        0x08
#define COMP_MAX        0x37
#define COMP_MIN_LEN    4

char in_buf[BUF_SIZE], f1_buf[BUF_SIZE], f2_buf[BUF_SIZE];
int in_buf_size, f1_buf_size, f2_buf_size;


int decomp_psg (char *src_buf, char *dst_buf, int in_size) {
  char *src=src_buf;
  char *dst=dst_buf;
  int out_size=0;

  while (in_size>0) {
    if ((*src>=COMP_MIN) && (*src<=COMP_MAX)) {
      int cmp_size=*src-COMP_MIN+COMP_MIN_LEN;                                       // calculate compressed block size
      int cmp_offs=((unsigned char)src[1])+(((unsigned char)src[2])*256);            // calculate compressed block offset
      memcpy (dst, &src_buf[cmp_offs], cmp_size);
      src+=3;
      in_size-=3;
      dst+=cmp_size;
      out_size+=cmp_size;
    } else {
      *dst++=*src++;
      out_size++;
      in_size--;
    }
  }

  return(out_size);
}

int main (int argc, char *argv[]) {

  if (argc!=3) {
    printf ("Usage: psgcmp file_1.PSG file_2.PSG\n");
    return (1);
  }

  fIN=fopen(argv[1],"rb");
  if (!fIN) {
    perror(argv[1]);
    return (1);
  }

  in_buf_size=fread (&in_buf, 1, BUF_SIZE, fIN);      // read first file
  fclose (fIN);

  f1_buf_size=decomp_psg(in_buf, f1_buf, in_buf_size);

  printf ("%s: file size = %d : decompressed PSG data size = %d\n",argv[1],in_buf_size,f1_buf_size);

  fIN=fopen(argv[2],"rb");
  if (!fIN) {
    perror(argv[2]);
    return (1);
  }

  in_buf_size=fread (&in_buf, 1, BUF_SIZE, fIN);      // read second file
  fclose (fIN);

  f2_buf_size=decomp_psg(in_buf, f2_buf, in_buf_size);

  printf ("%s: file size = %d : decompressed PSG data size = %d\n",argv[2],in_buf_size,f2_buf_size);

  if (f1_buf_size!=f2_buf_size) {
    printf ("Files differ (data length mismatch)\n");
    return (1);
  }

  if (memcmp(f1_buf, f2_buf, f1_buf_size)) {
    printf ("Files differ (data content mismatch)\n");
    return (1);
  }

  printf ("Files data contents match!\n");
  return(0);
}
