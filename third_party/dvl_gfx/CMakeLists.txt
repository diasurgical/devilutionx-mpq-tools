include(functions/FetchContent_MakeAvailableExcludeFromAll)

set(ENABLE_INSTALL OFF)
FetchContent_Declare(
  dvl_gfx
  URL https://github.com/diasurgical/devilutionx-graphics-tools/archive/ae39773dc2932b82328507bb51f79fa13b561ec0.tar.gz
  URL_HASH MD5=8036b84df319a8f5202bb8e4f999a275
)
FetchContent_MakeAvailableExcludeFromAll(dvl_gfx)
