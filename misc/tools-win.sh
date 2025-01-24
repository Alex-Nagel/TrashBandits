set -ex

mkdir -p tools
cd tools

export CC=x86_64-w64-mingw32-gcc
VPATH=../ext/pixler/tools make -j -f ../ext/pixler/tools/Makefile
VPATH=../ext/famitone5 make -j -f ../ext/famitone5/Makefile

git clone https://github.com/cc65/cc65.git --depth 1
cd cc65
make -j bin
mkdir -p lib
make -j nes
