include(functions/FetchContent_MakeAvailableExcludeFromAll)

set(ENABLE_INSTALL OFF)
FetchContent_Declare(
  dvl_gfx
  URL https://github.com/diasurgical/devilutionx-graphics-tools/archive/refs/tags/1.1.0.tar.gz
  URL_HASH MD5=767297b5c2fd208dc6b3a97146b1065b
)
FetchContent_MakeAvailableExcludeFromAll(dvl_gfx)
