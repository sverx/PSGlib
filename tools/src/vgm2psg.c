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

unsigned char volume[CHANNELS]={0x0F,0x0F,0x0F,0x0F};  // starting volume = silence
unsigned short freq[CHANNELS]={0,0,0,0};
int volume_change[CHANNELS]={FALSE,FALSE,FALSE,FALSE};
int freq_change[CHANNELS]={FALSE,FALSE,FALSE,FALSE};
int frame_started=TRUE;
int pause_started=FALSE;
int pause_len=0;
unsigned char lastlatch=0x9F;   // latch volume silent on channel 0
int active[CHANNELS]={FALSE,FALSE,FALSE,FALSE};
int is_sfx=FALSE;


void decLoopOffset(int n) {
  loop_offset-=n;
}

void incLoopOffset(void) {
  loop_offset++;
}  


int checkLoopOffset(void) {        // returns 1 when loop_offset becomes 0
  return (loop_offset==0);
}


void init_frame(int initial_state) {
  int i;
  for (i=0;i<CHANNELS;i++) {
    if ((!initial_state) ||                                // set to FALSE
        ((initial_state) && (!is_sfx)) ||                  // or set to TRUE if it's not a SFX
        ((initial_state) && (is_sfx) && (active[i]))) {    // or set to TRUE if it's a SFX and the chn is active
      volume_change[i]=initial_state;    
      freq_change[i]=initial_state;
    }
  }
  frame_started=initial_state;
}

void add_command (unsigned char c) {
  int chn,typ;
  if (c&0x80) {           // is a latch
    chn=(c&0x60)>>5;
    typ=(c&0x10)>>4;
    if (typ==1) {
      volume[chn]=c&0x0F;
      volume_change[chn]=TRUE;
    } else {
      freq[chn]=(freq[chn]&0xFFF0)|(c&0x0F);
      freq_change[chn]=TRUE;
    }
  } else {
    chn=(lastlatch&0x60)>>5;
    typ=(lastlatch&0x10)>>4;	
    if (typ==1) {
      volume[chn]=c&0x0F;
      volume_change[chn]=TRUE;
    } else {
      freq[chn]=(freq[chn]&0x000F)|((c&0x3F)<<4);
      freq_change[chn]=TRUE;
    }	
  }  	
}

void dump_frame(void) {
  int i;
  unsigned char c;
  for (i=0;i<CHANNELS-1;i++) {
    if (freq_change[i]) {
      c=(freq[i]&0x0F)|(i<<5)|0x80;          // latch channel 0-2 freq
      fputc(c,fOUT);
      c=(freq[i]>>4)|0x40;                   // make sure DATA bytes have 1 as 6th bit
      fputc(c,fOUT);
    }
    
    if (volume_change[i]) {
      c=0x90|(i<<5)|(volume[i]&0x0F);        // latch channel 0-2 volume
      fputc(c,fOUT);
    }

  }
  
  if (freq_change[3]) {
    c=(freq[i]&0x07)|0xE0;                   // latch channel 3 (noise)
    fputc(c,fOUT);
  }

  if (volume_change[3]) {
    c=0x90|(i<<5)|(volume[3]&0x0F);          // latch channel 3 volume
    fputc(c,fOUT);
  }
}


void dump_pause(void) {
  if (pause_len>0) {
    while (pause_len>MAX_WAIT) {
          fputc(PSG_WAIT+MAX_WAIT,fOUT);   // write PSG_WAIT+7 to file
          pause_len-=MAX_WAIT+1;           // skip MAX_WAIT+1 
    }
    if (pause_len>0)
      fputc(PSG_WAIT+(pause_len-1),fOUT);  // write PSG_WAIT+[0 to 7] to file, don't do it if 0
  }
}

void found_pause(void) {
  if (frame_started) {
    dump_frame();
    init_frame(FALSE);
  }
  pause_started=TRUE;
}

void found_frame(void) {
  if (pause_started)
    dump_pause();
  frame_started=TRUE;
  
  pause_started=FALSE;
  pause_len=0;
}

void empty_data(void) {
  if (pause_started) {
    dump_pause();
    pause_len=0;
    pause_started=FALSE;
  } else if (frame_started) {
    dump_frame();
    init_frame(FALSE);
  }
}


void writeLoopMarker(void) {
  empty_data();
  fputc(PSG_LOOPMARKER,fOUT);
}


int main (int argc, char *argv[]) {

  unsigned int i;
  int c;
  int leave=0;
  int fatal=0;
  int ss,fs;
  int latched_chn=0;
  int first_byte=TRUE;
  unsigned int file_signature;
  
  printf ("*** Sverx's VGM to PSG converter ***\n");
  
  if ((argc<3) || (argc>4)) {
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
    is_sfx=TRUE;
    printf ("Info: SFX conversion on channel(s): %s%s\n",active[2]?"2":"",active[3]?"3":"");
  }
  
  init_frame(TRUE);
  
  fIN=fopen(argv[1],"rb");
  if (!fIN) {
    printf("Fatal: can't open input VGM file\n");
    return(1);
  }
  
  fread (&file_signature, 4, 1, fIN);
  if (file_signature!=0x206d6756) {    // check for 'Vgm ' file signature
    printf("Fatal: input VGM file doesn't seem a valid *uncompressed* VGM file\n");
    return(1);
  }

  fOUT=fopen(argv[2],"wb");
  if (!fOUT) {
    printf("Fatal: can't write to output PSG file\n");
    return(1);
  }

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
  } else {
    printf ("Info: no loop point defined\n");
    loop_offset=-1; // make it negative so that won't happen
  }
    
  while ((!leave) && (!feof(fIN))) {
  
    c=fgetc(fIN);
    decLoopOffset(1);
    if (checkLoopOffset()) writeLoopMarker();
    
    switch (c) {
    
      case VGM_GGSTEREO:            // stereo data byte follows
        // BETA: this is simply DISCARDED atm
        c=fgetc(fIN);
        printf("Warning: GameGear stereo info discarded\n");
        decLoopOffset(1);
        if (checkLoopOffset()) writeLoopMarker();
        break;
     
      case VGM_PSGFOLLOWS:          // PSG byte follows
      
        c=fgetc(fIN);
        
        if (c&0x80) {
          lastlatch=c;               // latch value
          latched_chn=(c&0x60)>>5;   // isolate chn number
        } else {
          c|=0x40;                   // make sure DATA bytes have 1 as 6th bit
        }

        if ((!is_sfx) || (active[latched_chn])) {   // output only if on an active channel
        
        found_frame();
        
          if ((first_byte) && ((c&0x80)==0)) {
            add_command(lastlatch);
            printf("Warning: added missing latch command in frame start\n");
          }
          add_command(c);
          first_byte=FALSE;
        }
        
        decLoopOffset(1);
        if (checkLoopOffset()) writeLoopMarker();
        break;
        
      case VGM_FRAMESKIP:          // frame skip, now count how many
      
        found_pause();
      
        fs=1;
        do {
          c=fgetc(fIN);
          if (c==VGM_FRAMESKIP) fs++;
          decLoopOffset(1);
        } while ((fs<MAX_WAIT) && (c==VGM_FRAMESKIP) && (!checkLoopOffset()));
        
        if (c!=VGM_FRAMESKIP) {
          ungetc(c,fIN);
          incLoopOffset();
        }
        
        pause_len+=fs;
        if (checkLoopOffset()) writeLoopMarker();
        
        first_byte=TRUE;
        
        break;
        
      case VGM_SAMPLESKIP:         // sample skip, now count how many
      
        found_pause();
      
        c=fgetc(fIN);
        ss=c;
        c=fgetc(fIN);
        ss+=c*256;
        fs=ss/0x2df;                   // samples to NTSC frames
        if ((ss%0x2df)!=0) {
          printf("Warning: pause length isn't perfectly frame sync'd\n");
          if ((ss%0x2df)>(0x2df/2))   // round to closest int
            fs++;
        }

        pause_len+=fs;
        
        decLoopOffset(2);
        if (checkLoopOffset()) writeLoopMarker();
        
        first_byte=TRUE;
        
        break;
      
        
      case VGM_ENDOFDATA:         // end of data
        leave=1;
        decLoopOffset(1);
        if (checkLoopOffset()) writeLoopMarker();
        empty_data();
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
