all:
	cd ./voxen/ && make CFLAGS="-O3" 
	cd ./tests/ && make CFLAGS="-O3"
	cd ./testapp/ && make CFLAGS="-O3"

clang:
	cd ./voxen/ && make CC="clang" CFLAGS="-O3" 
	cd ./tests/ && make CC="clang" CFLAGS="-O3"
	cd ./testapp/ && make CC="clang" CFLAGS="-O3"

fastgcc:
	cd ./voxen/ && make CFLAGS="-O3 -march=native -flto"
	cd ./tests/ && make CFLAGS="-O3 -march=native -flto"
	cd ./testapp/ && make CFLAGS="-O3 -march=native -flto"

fastclang:
	cd ./voxen/ && make CC="clang" CFLAGS="-O3 -march=native"
	cd ./tests/ && make CC="clang" CFLAGS="-O3 -march=native"
	cd ./testapp/ && make CC="clang" CFLAGS="-O3 -march=native"

nopencl:
	cd ./voxen/ && make CFLAGS="-O3" DEFINES="-D NO_OPENCL" CLIBS="-L/usr/lib -lSDL -Bstatic -lm -Bstatic -lc -lpthread" 
	cd ./tests/ && make CFLAGS="-O3" DEFINES="-D NO_OPENCL" CLIBS="-L/usr/lib -lSDL -Bstatic -lm -Bstatic -lc -lpthread"
	cd ./testapp/ && make CFLAGS="-O3" DEFINES="-D NO_OPENCL" CLIBS="-L/usr/lib -lSDL -Bstatic -lm -Bstatic -lc -lpthread"
	
debug:
	cd ./voxen/ && make CFLAGS="-O0 -g3"
	cd ./tests/ && make CFLAGS="-O0 -g3"
	cd ./testapp/ && make CFLAGS="-O0 -g3"

sse:
	cd ./voxen/ && make CC="clang" CFLAGS="-O3 -mfpmath=sse"
	cd ./tests/ && make CC="clang" CFLAGS="-O3 -mfpmath=sse"
	cd ./testapp/ && make CC="clang" CFLAGS="-O3 -mfpmath=sse"
	cd ./conversion_tool/ && make CC="clang" CFLAGS="-O3 -mfpmath=sse"


