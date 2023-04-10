include(functions/FetchContent_MakeAvailableExcludeFromAll)

set(ENABLE_INSTALL OFF)
FetchContent_Declare(
  dvl_gfx
  URL https://github.com/diasurgical/devilutionx-graphics-tools/archive/e825e35a991abcef2de5a84fb38efa84d88d6620.tar.gz
  URL_HASH MD5=611400c6f7552aa9f6da78aa39cc9eda
)
FetchContent_MakeAvailableExcludeFromAll(dvl_gfx)
