#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define     FALSE                   0
#define     TRUE                    1

#define     VGM_OLD_HEADERSIZE      64        // 'old' VGM header
#define     VGM_HEADER_LOOPPOINT    0x1C
#define     VGM_DATA_OFFSET         0x34

#define     VGM_GGSTEREO    0x4F
#define     VGM_PSGFOLLOWS  0x50
#define     VGM_FRAMESKIP   0x62
#define     VGM_SAMPLESKIP  0x61
#define     VGM_ENDOFDATA   0x66

#define     MAX_WAIT        7                 // fits in 3 bits only

#define     PSG_ENDOFDATA   0x00
#define     PSG_LOOPMARKER  0x01
#define     PSG_WAIT        0x38

#define     CHANNELS        4



unsigned int loop_offset;
unsigned int data_offset;
FILE *fIN;
FILE *fOUT;
  

void decLoopOffset(int n) {
  loop_offset-=n;
}

void incLoopOffset(void) {
  loop_offset++;
}  


int checkLoopOffset(void) {        // returns 1 when loop_offset becomes 0
  return (loop_offset==0);
}

void writeLoopMarker(void) {
  fputc(PSG_LOOPMARKER,fOUT);
}


int main (int argc, char *argv[]) {

  int i,c;
  int leave=0;
  int fatal=0;
  int ss,fs;
  int active[CHANNELS];
  int is_sfx=0;
  int latched_chn=0;
  int lastlatch=0x9F;   // latch volume silent on channel 0
  int first_byte=TRUE;

  if ((argc<3) || (argc>4)) {
    printf ("*** Sverx's VGM to PSG converter ***\n");
    printf ("Usage: vgm2psg inputfile.VGM outputfile.PSG [2|3|23]\n");
    printf (" the optional third parameter specifies which channel(s) should be active,\n");
    printf (" for SFX conversion:\n");
    printf (" - 2 means the SFX is using channel 2 only\n");
    printf (" - 3 means the SFX is using channel 3 (noise) only\n");
    printf (" - 23 means the SFX is using both channels\n");
    return (1);
  }
  
  if (argc==4) {
    for (i=0;i<CHANNELS;i++)
      active[i]=0;
  
    for (i=0;i<strlen(argv[3]);i++) {
      switch (argv[3][i]) {
        case '2':
          active[2]=1;
          break;
        case '3':
          active[3]=1;
          break;
      }
    }
    
    is_sfx=1;
    
    printf ("Info: SFX conversion on channel(s): %s%s\n",active[2]?"2":"",active[3]?"3":"");
  }
  
  
  fIN=fopen(argv[1],"rb");
  fOUT=fopen(argv[2],"wb");
  
  fseek(fIN,VGM_HEADER_LOOPPOINT,SEEK_SET);  // seek to LOOPPOINT in the VGM header
  fread (&loop_offset, 4, 1, fIN);           // read loop_offset
  
  fseek(fIN,VGM_DATA_OFFSET,SEEK_SET);       // seek to DATAOFFSET in the VGM header
  fread (&data_offset, 4, 1, fIN);           // read data_offset
  
  if (data_offset) {
    fseek(fIN,VGM_DATA_OFFSET+data_offset,SEEK_SET);  // skip VGM header
    data_offset=VGM_DATA_OFFSET+data_offset;
  } else {
    fseek(fIN,VGM_OLD_HEADERSIZE,SEEK_SET);           // skip 'old' VGM header
    data_offset=VGM_OLD_HEADERSIZE;
  }
  
  if (loop_offset!=0) {
    printf ("Info: loop point at 0x%08x\n",loop_offset);
    loop_offset=loop_offset+VGM_HEADER_LOOPPOINT-data_offset;
  } else 
    printf ("Info: no loop point defined\n");
    
  while ((!leave) && (!feof(fIN))) {
  
    c=fgetc(fIN);
    decLoopOffset(1);
    if (checkLoopOffset()) writeLoopMarker();
    
    switch (c) {
    
      case VGM_GGSTEREO:            // stereo data byte follows
        // BETA: this is simply DISCARDED atm
        c=fgetc(fIN);
        printf("Warning: GameGear stereo info discarded\n",c);
        decLoopOffset(1);
        if (checkLoopOffset()) writeLoopMarker();
        break;
     
      case VGM_PSGFOLLOWS:          // PSG byte follows
        c=fgetc(fIN);
        
        if (c&0x80) {
          lastlatch=c;                // latch value
          latched_chn=(c&0x60)>>5;   // isolate chn number
        } else {
          c|=0x40;                   // make sure DATA bytes have 1 as 6th bit
        }

        if ((!is_sfx) || (active[latched_chn])) {   // output only if on an active channel
          if ((first_byte) && ((c&0x80)==0)) {
            fputc(lastlatch,fOUT);
            printf("Warning: added missing latch command in frame start\n");
          }
          fputc(c,fOUT);
          first_byte=FALSE;
        }
        
        decLoopOffset(1);
        if (checkLoopOffset()) writeLoopMarker();
        break;
        
      case VGM_FRAMESKIP:          // frame skip, now count how many
        fs=0;
        do {
          c=fgetc(fIN);
          if (c==VGM_FRAMESKIP) fs++;
          decLoopOffset(1);
        } while ((fs<MAX_WAIT) && (c==VGM_FRAMESKIP) && (!checkLoopOffset()));
        
        if (fs<MAX_WAIT) {
          ungetc(c,fIN);
          incLoopOffset();
        }
        
        fputc(PSG_WAIT+fs,fOUT);   // write PSG_WAIT+[0 to MAX_WAIT] to file
        if (checkLoopOffset()) writeLoopMarker();
        
        first_byte=TRUE;
        
        break;
        
      case VGM_SAMPLESKIP:         // sample skip, now count how many
        c=fgetc(fIN);
        ss=c;
        c=fgetc(fIN);
        ss+=c*256;
        fs=ss/0x2df;                   // samples to NTSC frames
        if ((ss%0x2df)!=0) {
          printf("Warning: pause length isn't perfectly frame sync'd\n",c);
          if ((ss%0x2df)>(0x2df/2))   // round to closest int
            fs++;
        }

        while (fs>MAX_WAIT) {
          fputc(PSG_WAIT+MAX_WAIT,fOUT);   // write PSG_WAIT+7 to file
          fs-=MAX_WAIT+1;                  // skip MAX_WAIT+1 
        }
        
        if (fs>0)
          fputc(PSG_WAIT+(fs-1),fOUT);    // write PSG_WAIT+[0 to 7] to file, don't do it if 0
          
        decLoopOffset(2);
        if (checkLoopOffset()) writeLoopMarker();
        
        first_byte=TRUE;
        
        break;
      
        
      case VGM_ENDOFDATA:         // end of data
        leave=1;
        decLoopOffset(1);
        if (checkLoopOffset()) writeLoopMarker();
        fputc(PSG_ENDOFDATA,fOUT);
        break;
        
      default:
        printf("Fatal: found unknown char 0x%02x\n",c);
        leave=1;
        fatal=1;
        break;
    }

  }
  
  fclose (fIN);
  fclose (fOUT);
  
  if (!fatal) {
    printf ("Info: conversion complete\n");
    return(0);
  } else {
    return(1);
  }
}
