cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(osu_vita C)

include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "osu! vita")
set(VITA_TITLEID  "OSU000001")
set(VITA_VERSION  "01.00")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -flto")

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

add_executable(${PROJECT_NAME}
  ${SOURCES}
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

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME}
  UNSAFE
)

set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -s PSP2_SYSTEM_VER=03.000 -d PARENTAL_LEVEL=1 -s ATTRIBUTE2=00.000")
