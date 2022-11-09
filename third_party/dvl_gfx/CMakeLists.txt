include(functions/FetchContent_MakeAvailableExcludeFromAll)

set(ENABLE_INSTALL OFF)
FetchContent_Declare(
  dvl_gfx
  URL https://github.com/diasurgical/devilutionx-graphics-tools/archive/0c772ebf497ef9d4aa82b4339913babd535e9bb5.tar.gz
  URL_HASH MD5=2a9bb407f81badc047d75899db9228f2
)
FetchContent_MakeAvailableExcludeFromAll(dvl_gfx)
