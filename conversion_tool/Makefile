CWARN  := -Wall -std=c99 -g
CFLAGS := -O0
DEFINES := 
CLIBS := -L/usr/lib/ -lSDL -lm -lpthread #-lOpenCL
all:
	$(CC) $(CWARN) $(CFLAGS) $(DEFINES) main.c -o voxconv -I./../voxen/ -Bstatic -L./../voxen/bin/posix/static/ -lvoxen -Bdynamic -lSDL_image $(CLIBS)
