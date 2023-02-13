include(functions/FetchContent_MakeAvailableExcludeFromAll)

set(ENABLE_INSTALL OFF)
FetchContent_Declare(
  dvl_gfx
  URL https://github.com/diasurgical/devilutionx-graphics-tools/archive/6714687c09c21aaa10a65e27c51dd386d9e4d6be.tar.gz
  URL_HASH MD5=f01d19d7ca5df29deda04707292ebd2f
)
FetchContent_MakeAvailableExcludeFromAll(dvl_gfx)
