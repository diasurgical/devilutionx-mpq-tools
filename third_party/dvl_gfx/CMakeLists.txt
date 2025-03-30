include(functions/FetchContent_MakeAvailableExcludeFromAll)

set(ENABLE_INSTALL OFF)
FetchContent_Declare(
  dvl_gfx
  URL https://github.com/diasurgical/clx-conversion-tools/archive/ae39773dc2932b82328507bb51f79fa13b561ec0.tar.gz
  URL_HASH MD5=3f879a34fa84cc4dc5e81302900bbb44
)
FetchContent_MakeAvailableExcludeFromAll(dvl_gfx)
