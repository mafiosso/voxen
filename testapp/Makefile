CWARN  := -Wall -std=c99 -g
CFLAGS := -O3
DEFINES := 
CLIBS := -L/usr/lib/ -lSDL -lm -lpthread #-lOpenCL
all:
	$(CC) $(CWARN) $(CFLAGS) $(DEFINES) main.c -o testapp -I./../voxen/ -Bstatic -L./../voxen/bin/posix/static/ -lvoxen -Bdynamic $(CLIBS)
