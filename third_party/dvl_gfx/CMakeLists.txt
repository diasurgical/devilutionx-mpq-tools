include(functions/FetchContent_MakeAvailableExcludeFromAll)

set(ENABLE_INSTALL OFF)
FetchContent_Declare(
  dvl_gfx
  URL https://github.com/diasurgical/devilutionx-graphics-tools/archive/95c3b4174b1c5e83bfb7f8681b663cb77ec9bcaa.tar.gz
  URL_HASH MD5=8d9124f114a0b9a0bfdb1b9a9b7518dd
)
FetchContent_MakeAvailableExcludeFromAll(dvl_gfx)
