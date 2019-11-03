#!/bin/bash

mkdir -p build
pushd build
gcc $(pwd)/../handmade.cpp -I/usr/include/SDL2 -lSDL2 -lm -g -o handmade
popd

