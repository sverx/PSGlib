for Linux:
gcc -Wall -o vgm2psg vgm2psg.c -lz
gcc -Wall -o psgcomp psgcomp.c
gcc -Wall -o psgdecomp psgdecomp.c

for Windows (cross compile):
i686-w64-mingw32-gcc -Wall -o vgm2psg.exe vgm2psg.c -lz
i686-w64-mingw32-gcc -Wall -o psgcomp.exe psgcomp.c
i686-w64-mingw32-gcc -Wall -o psgdecomp.exe psgdecomp.c

