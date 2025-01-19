mkdir -p tools
cd tools

VPATH=../ext/pixler/tools make -j -f ../ext/pixler/tools/Makefile
VPATH=../ext/famitone5 make -j -f ../ext/famitone5/Makefile

git clone https://github.com/cc65/cc65.git --depth 1 -b V2.19
cd cc65
make -j bin nes
