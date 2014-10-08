#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *fIN;
FILE *fOUT;

#define BUF_SIZE        65536
#define MIN_LEN         4
#define MAX_LEN         51        // 47+4

#define PSG_SUBSTRING   0x08


unsigned char buf[BUF_SIZE];

int size;


int main (int argc, char *argv[]) {

  int testlen, haystack, consolidate;
  
  int skip, needle;
  
  int max_save, max_save_at_skip,max_save_at_len,max_save_at_haystack;

  if (argc!=3) {
    printf ("Usage: psgcomp inputfile.PSG outputfile.PSG\n");
    return (1);
  }
  
  fIN=fopen(argv[1],"rb");
  size=fread (&buf, 1, BUF_SIZE, fIN);      // read input file
  fclose (fIN);
  
  printf ("Info: input file size is %d bytes\n",size);
  
  if (size>=4*1024)                             // just for fun
    printf ("Info: this will take a while...");   
  else
    printf ("Info: compression started...");
  
  // start 'consolidating' from beginning of buf[4], to end-4
  for (consolidate=MIN_LEN; consolidate<(size-MIN_LEN); consolidate++) {
  
    max_save=0;
    
    for (skip=0; skip<MAX_LEN; skip++) {
    
      // try longest 'substrings' first!
      for (testlen=MAX_LEN; testlen>=MIN_LEN; testlen--) {
      
        // is it worth trying?
        if (max_save<(testlen-3-skip)) {
    
          // haystack is from buf[0]
          for (haystack=0; haystack<=(consolidate+skip-testlen); haystack++) {   // less or equal, not only less than
        
            needle=consolidate+skip;
      
            if ((needle+testlen<size) &&
                (memcmp(&buf[needle],&buf[haystack],testlen)==0)) {
            
              // we've found a correspondance!
              if ((testlen-3-skip)>max_save) {
              
                // check if it's the first one OR
                // if it's not and saves more ONLY 'sacrificing' something previously found
                if ((max_save==0) || 
                   (needle<(max_save_at_haystack+max_save_at_len))) {
                  max_save=(testlen-3-skip);
                  max_save_at_skip=skip;
                  max_save_at_len=testlen;
                  max_save_at_haystack=haystack;
                }
              }
            }
          }
        }
      }
    }
    
    if (max_save>0) {
      
      if (max_save_at_skip==0) {
      
        consolidate+=max_save_at_skip;   // consolidate 'skip' bytes
        testlen=max_save_at_len;
        haystack=max_save_at_haystack;
      
        //  printf ("Skip %2d bytes, compress string of len %d\n",max_save_at_skip,max_save_at_len);
      
        buf[consolidate]=(testlen-MIN_LEN)+PSG_SUBSTRING;
        buf[consolidate+1]=(haystack&0xFF);
        buf[consolidate+2]=(haystack>>8);
        memmove(&buf[consolidate+3],&buf[consolidate+testlen],size-(consolidate+3));
        size-=(testlen-3);
        consolidate+=2;                  // 'consolidate' two additional bytes
       } else {
        consolidate+=(max_save_at_skip-1);   // consolidate 'skip-1' bytes
       }
        
        
    } else {
    
      // printf ("Skip 1 byte\n");
    
    }
    
    if (consolidate%100==0)
      printf (".");      // print a dot every 100 chars consolidated, to show progress
    
  }

  fOUT=fopen(argv[2],"wb");
  fwrite (&buf, 1, size, fOUT); 
  fclose (fOUT);
  
  printf ("done!\n");

  printf ("Info: output file size is %d bytes\n",size);
  return(0);
}
