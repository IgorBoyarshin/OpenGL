#!/usr/bin/zsh

export MESA_GL_VERSION_OVERRIDE=4.5

make
./main
make clean
