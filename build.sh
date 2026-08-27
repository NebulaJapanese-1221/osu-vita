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

$VITASDK/bin/vita-mksfoex -s PSP2_SYSTEM_VER=03.000 -d PARENTAL_LEVEL=1 -s ATTRIBUTE2=00.000 -s CATEGORY=ac -s TITLE_ID=OSU000001 "osu! vita" osu_vita.param.sfo

mkdir -p vpk_package/sce_sys/livearea/contents
cp osu_vita.self vpk_package/eboot.bin
cp osu_vita.param.sfo vpk_package/sce_sys/param.sfo
cp ../assets/icon0.png vpk_package/sce_sys/icon0.png
cp ../assets/startup.png vpk_package/sce_sys/livearea/contents/startup.png
cp ../assets/bg.png vpk_package/sce_sys/livearea/contents/bg.png
cp ../assets/livearea/contents/template.xml vpk_package/sce_sys/livearea/contents/template.xml

mkdir -p vpk_package/ux0:/data/osu-vita/maps
cp ../assets/bgm.wav vpk_package/ux0:/data/osu-vita/bgm.wav
cp ../assets/sample_beatmap.osu vpk_package/ux0:/data/osu-vita/maps/sample_beatmap.osu
cp ../assets/config.ini vpk_package/ux0:/data/osu-vita/config.ini

cd vpk_package
zip -0 -r ../osu_vita.vpk eboot.bin sce_sys/ ux0:/
cd ..

rm -rf vpk_package osu_vita.param.sfo

echo "Build complete. VPK at: build/osu_vita.vpk"
