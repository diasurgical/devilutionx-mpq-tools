cmake_minimum_required(VERSION 3.13)

foreach(_policy CMP0083 CMP0092 CMP0111 CMP0126 CMP0135)
  if(POLICY ${_policy})
    cmake_policy(SET ${_policy} NEW)
    set(CMAKE_POLICY_DEFAULT_${_policy} NEW)
  endif()
endforeach()

# Projects added via `add_subdirectory` or `FetchContent` may have a lower
# `cmake_minimum_required` than we set here. Set policies that we require
# to their new value so that they still apply.
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

project(DvlMpqTools VERSION 1.0.0 LANGUAGES C CXX)

option(ASAN "Enable address sanitizer" ON)
option(UBSAN "Enable undefined behaviour sanitizer" ON)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

if(CMAKE_BUILD_TYPE MATCHES "Release")
  set(ASAN OFF)
  set(UBSAN OFF)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for clang-tidy

# For ease of distribution, we produce a single statically-linked executable
set(BUILD_SHARED_LIBS OFF)
set(DEVILUTIONX_STATIC_BZIP2 ON)
set(DEVILUTIONX_STATIC_ZLIB ON)

add_subdirectory(third_party/bzip2)
add_subdirectory(third_party/zlib)
add_subdirectory(third_party/libmpq)
add_subdirectory(third_party/dvl_gfx)

foreach(_path
  diabdat-clx diabdat-listfile diabdat-rm hellfire-clx hellfire-listfile hellfire-rm
  hfmonk-clx hfmonk-listfile hfmonk-rm hfmusic-listfile hfmusic-rm
  hfvoice-listfile hfvoice-rm spawn-clx spawn-listfile spawn-rm save-listfile)
  file(STRINGS data/${_path}.txt _lines)
  set(_output_contents "")
  foreach(_line ${_lines})
    string(REPLACE "\\" "\\\\" _line ${_line})
    string(APPEND _output_contents "  \"${_line}\",\n")
  endforeach()
  string(REPLACE "-" "_" _c_name ${_path})
  set(_c_name "embedded_${_c_name}")
  string(APPEND _output_c "
const char *const ${_c_name}_data[] = {
${_output_contents}
};
const unsigned ${_c_name}_size = sizeof(${_c_name}_data) / sizeof(${_c_name}_data[0]);
")
  string(APPEND _output_h "
extern const char *const ${_c_name}_data[];
extern const unsigned ${_c_name}_size;
")
endforeach()

# Note: The file will only be regenarated on reconfigure, not when the source file changes.
# We do not expect the source files to change much so it's fine.
file(GENERATE
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/embedded_files.h
  CONTENT "#ifndef EMBEDDED_FILES_H_
#define EMBEDDED_FILES_H_
${_output_h}
#endif // EMBEDDED_FILES_H_
")
file(GENERATE
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/embedded_files.c
  CONTENT "#include \"embedded_files.h\"

${_output_c}
")
add_library(embedded_files OBJECT
  ${CMAKE_CURRENT_BINARY_DIR}/generated/embedded_files.c)
target_include_directories(embedded_files PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}/generated/)

add_library(extract_spell_icons OBJECT src/extract_spell_icons.cpp)
target_include_directories(extract_spell_icons PUBLIC src)

target_link_libraries(extract_spell_icons
  PRIVATE
  DvlGfx::clx2pixels
  DvlGfx::pixels2clx)

add_executable(unpack_and_minify_mpq src/unpack_and_minify_mpq.cpp)
target_link_libraries(unpack_and_minify_mpq PRIVATE
  libmpq
  DvlGfx::cel2clx
  DvlGfx::cl22clx
  DvlGfx::pcx2clx
  extract_spell_icons
  embedded_files)

add_custom_command(
  TARGET unpack_and_minify_mpq POST_BUILD
  DEPENDS unpack_and_minify_mpq
  COMMAND $<$<CONFIG:release>:${CMAKE_STRIP}>
  ARGS --strip-all $<TARGET_FILE:unpack_and_minify_mpq>
)

install(
  TARGETS unpack_and_minify_mpq
  CONFIGURATIONS Release
)

foreach(_target unpack_and_minify_mpq)
  if(ASAN)
    target_compile_options(${_target} PUBLIC -fsanitize=address -fsanitize-recover=address)
    target_link_libraries(${_target} PUBLIC -fsanitize=address -fsanitize-recover=address)
  endif()

  if(UBSAN)
    target_compile_options(${_target} PUBLIC -fsanitize=undefined)
    target_link_libraries(${_target} PUBLIC -fsanitize=undefined)
  endif()

  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_link_libraries(${_target} PUBLIC "-static-libgcc -static-libstdc++")
  endif()
endforeach()
