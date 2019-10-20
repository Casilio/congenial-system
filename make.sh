#!/bin/bash

pushd build
gcc ../handmade.cpp -lX11 -g -o handmade
objcopy --only-keep-debug handmade handmade.debug
strip -g handmade
popd
