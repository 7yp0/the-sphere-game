#!/bin/bash

mkdir -p build
cd build || exit

if [ "$1" == "release" ]; then
    cmake -DCMAKE_BUILD_TYPE=Release ..
else
    cmake -DCMAKE_BUILD_TYPE=Debug ..
fi

make -j$(sysctl -n hw.ncpu)

echo "Build complete. Run with ./the_sphere_game"
