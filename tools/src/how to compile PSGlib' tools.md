for Linux:
```
gcc -Wall -o vgm2psg vgm2psg.c -lz
gcc -Wall -o psgcomp psgcomp.c
gcc -Wall -o psgdecomp psgdecomp.c
gcc -Wall -o psgcomp_ng psgcomp_ng.c growbuf.c psgcompress.c
gcc -Wall -o psg2txt psg2txt.c growbuf.c
```

for Windows (cross compile):
```
i686-w64-mingw32-gcc -Wall -o vgm2psg.exe vgm2psg.c -lz
i686-w64-mingw32-gcc -Wall -o psgcomp.exe psgcomp.c
i686-w64-mingw32-gcc -Wall -o psgdecomp.exe psgdecomp.c
i686-w64-mingw32-gcc -Wall -o psgcomp_ng psgcomp_ng.c growbuf.c psgcompress.c
i686-w64-mingw32-gcc -Wall -o psg2txt psg2txt.c growbuf.c
```

