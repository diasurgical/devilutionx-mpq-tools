include(functions/FetchContent_MakeAvailableExcludeFromAll)

FetchContent_Declare(
  dvl_gfx
  URL https://github.com/diasurgical/devilutionx-graphics-tools/archive/451199134addb5b7e8159fd0d5605f198f7efdc7.tar.gz
  URL_HASH MD5=354e0b5f07634bd35beae1489ef56d4b
)
FetchContent_MakeAvailable(dvl_gfx)
