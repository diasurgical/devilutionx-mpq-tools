include(functions/FetchContent_MakeAvailableExcludeFromAll)

set(ENABLE_INSTALL OFF)
FetchContent_Declare(
  dvl_gfx
  URL https://github.com/diasurgical/devilutionx-graphics-tools/archive/64c0629480598c6ebdd25c527082606c1ae952a1.tar.gz
  URL_HASH MD5=47588ce602e3cfc9d40dc43aa0e85a42
)
FetchContent_MakeAvailableExcludeFromAll(dvl_gfx)
