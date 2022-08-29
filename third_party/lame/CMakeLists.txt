include(functions/FetchContent_MakeAvailableExcludeFromAll)

include(FetchContent)
FetchContent_Declare(lame
    URL https://jaist.dl.sourceforge.net/project/lame/lame/3.100/lame-3.100.tar.gz
    URL_HASH MD5=83e260acbe4389b54fe08e0bdbf7cddb
)
FetchContent_MakeAvailableExcludeFromAll(lame)

add_library(
  lame_encoder
  STATIC
  ${lame_SOURCE_DIR}/libmp3lame/bitstream.c
  ${lame_SOURCE_DIR}/libmp3lame/encoder.c
  ${lame_SOURCE_DIR}/libmp3lame/fft.c
  ${lame_SOURCE_DIR}/libmp3lame/gain_analysis.c
  ${lame_SOURCE_DIR}/libmp3lame/id3tag.c
  ${lame_SOURCE_DIR}/libmp3lame/lame.c
  ${lame_SOURCE_DIR}/libmp3lame/newmdct.c
  ${lame_SOURCE_DIR}/libmp3lame/presets.c
  ${lame_SOURCE_DIR}/libmp3lame/psymodel.c
  ${lame_SOURCE_DIR}/libmp3lame/quantize.c
  ${lame_SOURCE_DIR}/libmp3lame/quantize_pvt.c
  ${lame_SOURCE_DIR}/libmp3lame/reservoir.c
  ${lame_SOURCE_DIR}/libmp3lame/set_get.c
  ${lame_SOURCE_DIR}/libmp3lame/tables.c
  ${lame_SOURCE_DIR}/libmp3lame/takehiro.c
  ${lame_SOURCE_DIR}/libmp3lame/util.c
  ${lame_SOURCE_DIR}/libmp3lame/vbrquantize.c
  ${lame_SOURCE_DIR}/libmp3lame/VbrTag.c
  ${lame_SOURCE_DIR}/libmp3lame/version.c
)

target_include_directories(
  lame_encoder
  PUBLIC
  ${lame_SOURCE_DIR}/include
)

target_include_directories(
  lame_encoder
  PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_definitions(
  lame_encoder
  PRIVATE
  HAVE_CONFIG_H)
