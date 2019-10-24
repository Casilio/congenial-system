#!/bin/bash

mkdir -p build
pushd build
gcc $(pwd)/../handmade.cpp -I/usr/include/SDL2 -lSDL2 -g -o handmade
popd

