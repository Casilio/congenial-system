#!/bin/bash

mkdir -p build
pushd build
gcc ../handmade.cpp -lX11 -g -o handmade
popd

