#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *fIN;
FILE *fOUT;

#define BUF_SIZE        65536
#define MIN_LEN         4
#define MAX_LEN         51        // 47+4

#define PSG_LOOPMARKER  0x01
#define PSG_SUBSTRING   0x08

unsigned char buf[BUF_SIZE];

int size;


int main (int argc, char *argv[]) {

  int i,offset,length;
  int lp_substr=0;

  if (argc!=3) {
    printf ("Usage: psgdecomp inputfile.PSG outputfile.PSG\n");
    return (1);
  }
  
  fIN=fopen(argv[1],"rb");
  size=fread (&buf, 1, BUF_SIZE, fIN);      // read input file
  fclose (fIN);
  
  printf ("Info: input file size is %d bytes\n",size);
  fOUT=fopen(argv[2],"wb");
  
  for (i=0; i<size; i++) {
  
    if ((buf[i]>=PSG_SUBSTRING) && (buf[i]<=PSG_SUBSTRING+MAX_LEN-MIN_LEN)) {
    
      offset=buf[i+1]+(buf[i+2]*256);
      length = buf[i]-PSG_SUBSTRING+MIN_LEN;
      if (memchr(&buf[offset], PSG_LOOPMARKER, length)) {
        lp_substr=1;
      }
      fwrite (&buf[offset], 1, length, fOUT);
      i+=2;  // skip two additional bytes

    } else {
      fwrite (&buf[i], 1, 1, fOUT);
      if (buf[i] == PSG_LOOPMARKER) {
        lp_substr = 0;
      }
    }
  
  }

  if (lp_substr) {
    printf ("Warning: Loop point set in substring\n");
  }

  fclose (fOUT);

  printf ("Info: done!\n");
  return(0);
}
