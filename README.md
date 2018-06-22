# voxen
Voxel engine based on octal trees and adaptive octal trees.

## Key features:
* Implements Raw voxel model - analogy to surface representation.
* Implements Octal tree.
* Implements Adaptive octal tree (optimal coding inovation).
* Implements basic raycasting for Raw voxel models and Octal trees (including dynamic lighting).
* Includes testapp for basic model loading and viewing, lighting settings available as well.
* Includes voxconv (conversion_tool dir) which allows to conversion between implemented formats. Support for image sets and Ken Silverman .vxl format loading is also supported.
* Includes basics tests.
* Includes prebuilt docs in html and latex format.

## Build
Prerequisities:
* libsdl1.2-dev
* libsdl-image1.2-dev
* gcc, make
* unix system (Windows port not completed yet)

Build:
```
  make
  cd conversion_tool && make && cd ..
```

There are several other make targets:
* make clang - build with clang instead of gcc.
* make fastgcc - build with lto options with gcc and native flags.
* make fastclang - build with optimized options of clang and native flags are set.
* make debug.
* make sse.

## Running
* testapp - ./testapp/testapp
* voxconv - ./conversion_tool/voxconv (help is printed)
* library binaries are placed in voxen/bin/ after successful build.




