#!/bin/bash

mkdir -p build
pushd build
gcc $(pwd)/../handmade.cpp -lX11 -g -o handmade
popd

