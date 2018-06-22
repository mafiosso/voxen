CC         := gcc
CLIBS      := -Bdynamic -L/usr/lib -lSDL -lm -lc -lpthread #-lOpenCL
DEFINES    := 
#COMPATIBLE := -O3 -march=i686
#GCCFAST    := -O4 -mtune=native -march=native -pipe -flto
#CLANGFAST  := -O3 -mtune=native -march=native -pipe
#DEBUGOPT   := -g -O0
CWARN      := -fPIC -Wall -fno-strict-aliasing -std=c99 -c
CFLAGS     := -O3

all:
	make clean
	make execute_make

execute_make:
	$(CC) $(CWARN) $(CFLAGS) $(DEFINES) camera.c $(CLIBS)
	$(CC) $(CWARN) $(CFLAGS) $(DEFINES) display.c $(CLIBS)
	$(CC) $(CWARN) $(CFLAGS) $(DEFINES) fpu_octree_traversal.c $(CLIBS)
	$(CC) $(CWARN) $(CFLAGS) $(DEFINES) fpu_octree_traversal_stck.c $(CLIBS)
	$(CC) $(CWARN) $(CFLAGS) $(DEFINES) lighting.c $(CLIBS)
	$(CC) $(CWARN) $(CFLAGS) $(DEFINES) matrix.c $(CLIBS)
	$(CC) $(CWARN) $(CFLAGS) $(DEFINES) null_driver.c $(CLIBS)
	$(CC) $(CWARN) $(CFLAGS) $(DEFINES) octree.c $(CLIBS)
	$(CC) $(CWARN) $(CFLAGS) $(DEFINES) adaptive_octree.c $(CLIBS)
	$(CC) $(CWARN) $(CFLAGS) $(DEFINES) ao_traversal.c $(CLIBS)
	$(CC) $(CWARN) $(CFLAGS) $(DEFINES) octree_traversal.c $(CLIBS)
	$(CC) $(CWARN) $(CFLAGS) $(DEFINES) opencl.c $(CLIBS)
	$(CC) $(CWARN) $(CFLAGS) $(DEFINES) peasant.c $(CLIBS)
	$(CC) $(CWARN) $(CFLAGS) $(DEFINES) raw.c $(CLIBS)
	$(CC) $(CWARN) $(CFLAGS) $(DEFINES) sdl_driver.c $(CLIBS)
	$(CC) $(CWARN) $(CFLAGS) $(DEFINES) slab.c $(CLIBS)
	$(CC) $(CWARN) $(CFLAGS) $(DEFINES) slab_fpu.c $(CLIBS)
	$(CC) $(CWARN) $(CFLAGS) $(DEFINES) stream.c $(CLIBS)
	$(CC) $(CWARN) $(CFLAGS) $(DEFINES) vox_fmt.c $(CLIBS)
	$(CC) $(CWARN) $(CFLAGS) $(DEFINES) voxmodel_ops.c $(CLIBS)
	$(CC) $(CWARN) $(CFLAGS) $(DEFINES) vx_fundamentals.c $(CLIBS)

	make build_in_cl

	make shared
	make static

build_in_cl:
	mkdir cltmp
	cp ./CL_code/octree_traversal_cl.cl ./cltmp/
	printf '\0' >> ./cltmp/octree_traversal_cl.cl
	ld -r -b binary -o ocl_traversal.o ./cltmp/octree_traversal_cl.cl
	rm -r ./cltmp/

shared:
	mkdir -p ./bin/posix/shared/
	$(CC) -shared *.o -o ./bin/posix/shared/libvoxen.so $(CLIBS)

static:
	mkdir -p ./bin/posix/static/
	$(AR) rcs ./bin/posix/static/libvoxen.a *.o

clean:
	rm -rf ./bin/*
	rm -f *.o
