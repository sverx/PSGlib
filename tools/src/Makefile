LD=$(CC)
CFLAGS=-Wall -O3
LDLIBS=-lz

all: psgcomp psgdecomp psgcmp vgm2psg psgcomp_ng psg2txt

psgcomp_ng: psgcomp_ng.o psgcompress.o growbuf.o
	$(LD) $(LDFLAGS) -o $@ $^

psg2txt: psg2txt.o growbuf.o
	$(LD) $(LDFLAGS) -o $@ $^

clean: psgcomp psgdecomp psgcmp vgm2psg psgcomp_ng psg2txt
	rm -f $^ *.o
