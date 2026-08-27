cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(osuvita)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "osuvita")
set(VITA_TITLEID  "OSU000002")
set(VITA_VERSION  "01.00")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(${PROJECT_NAME}
  src/main.c
  src/beatmap.c
  src/game.c
  src/render.c
  src/input.c
  src/scene.c
  src/skin.c
  src/audio.c
)

target_link_libraries(${PROJECT_NAME}
  m
  vita2d
  freetype
  png
  jpeg
  bz2
  z
  SceCtrl_stub
  SceTouch_stub
  SceAudio_stub
  SceKernelThreadMgr_stub
  SceProcessmgr_stub
  SceDisplay_stub
  ScePower_stub
  ScePgf_stub
  ScePvf_stub
  SceGxm_stub
  SceSysmem_stub
  SceAppMgr_stub
  SceCommonDialog_stub
  SceIofilemgr_stub
  SceLibc_stub
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME} UNSAFE)

set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -s PSP2_SYSTEM_VER=03.000 -d PARENTAL_LEVEL=1 -s ATTRIBUTE2=00.000 -s CATEGORY=ac")

vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)
