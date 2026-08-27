#!/bin/bash
set -e

if [ "$1" = "clean" ]; then
  rm -rf build
  echo "Cleaned build directory."
  exit 0
fi

mkdir -p build
cd build

cmake -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake \
      -DCMAKE_BUILD_TYPE=Release \
      ..

make -j$(nproc)

echo "Build complete. VPK at: build/osu_vita.vpk"
