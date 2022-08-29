include(functions/FetchContent_MakeAvailableExcludeFromAll)

set(ENABLE_INSTALL OFF)
FetchContent_Declare(
  dvl_gfx
  URL https://github.com/diasurgical/devilutionx-graphics-tools/archive/6f7e8c73df255d15617e5cb6ae088667b387207b.tar.gz
  URL_HASH MD5=47717c8d6d4bab5c19b44717fc40f798
)
FetchContent_MakeAvailableExcludeFromAll(dvl_gfx)
