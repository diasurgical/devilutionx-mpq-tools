include(functions/FetchContent_MakeAvailableExcludeFromAll)

set(ENABLE_INSTALL OFF)
FetchContent_Declare(
  dvl_gfx
  URL https://github.com/diasurgical/devilutionx-graphics-tools/archive/refs/tags/1.0.0.tar.gz
  URL_HASH MD5=aa502f0179ec6eb285bc61c19c98d5db
)
FetchContent_MakeAvailableExcludeFromAll(dvl_gfx)
