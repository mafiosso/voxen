DEFINES := 
CLIBS := -Bdynamic -L/usr/lib/ -lSDL -lm -lpthread #-lOpenCL
all:
	$(CC) -Wall -pedantic -g -std=c99 $(DEFINES) main.c -o testvoxen -I./../voxen/ -Bstatic -L./../voxen/bin/posix/static/ -lvoxen -Bdynamic $(CLIBS)
