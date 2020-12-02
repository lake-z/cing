# CMake list file at root.
cmake_minimum_required(VERSION 3.8)

# Project information
project(cing LANGUAGES C CXX ASM)

# Process all build options.
if(NOT TOOL_NASM)
  set(TOOL_NASM nasm)
endif()
message("${CMAKE_CURRENT_SOURCE_DIR}: Assembler: ${TOOL_NASM}")

if(NOT TOOL_GRUB_RESCURE)
  set(TOOL_GRUB_RESCURE grub2-mkrescure)
endif()
message("${CMAKE_CURRENT_SOURCE_DIR}: \
GRUB mkrescure: ${TOOL_GRUB_RESCURE}")

if(TOOL_CLANG)
  set(TOOL_CLANG_TIDY ${TOOL_CLANG}/bin/clang-tidy)
  set(TOOL_CLANG_FORMAT ${TOOL_CLANG}/bin/clang-format)
  message("${CMAKE_CURRENT_SOURCE_DIR}: Using clang-tidy: \
${TOOL_CLANG_TIDY}")
  message("${CMAKE_CURRENT_SOURCE_DIR}: Using clang-format: \
${TOOL_CLANG_FORMAT}")
else()
  message("${CMAKE_CURRENT_SOURCE_DIR}: No clang tools avaliable")
endif()


set(CMAKE_LINKER ld)
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_LINKER>  <FLAGS> <CMAKE_C_LINK_FLAGS> \
  <LINK_FLAGS> <OBJECTS> <LINK_LIBRARIES> -o <TARGET>")

option(BUILD_DEBUG_ENABLED "Enable debug codes or not" ON)
message("${CMAKE_CURRENT_SOURCE_DIR}: Debug enabled: ${BUILD_DEBUG_ENABLED}")
if(BUILD_DEBUG_ENABLED)
  ADD_COMPILE_DEFINITIONS(BUILD_DEBUG_ENABLED)
endif(BUILD_DEBUG_ENABLED)

option(BUILD_BUILTIN_TEST_ENABLED "Enable test mode or not" ON)
message("${CMAKE_CURRENT_SOURCE_DIR}: Built-in tests enabled: ${BUILD_BUILTIN_TEST_ENABLED}")
if(BUILD_BUILTIN_TEST_ENABLED)
ADD_COMPILE_DEFINITIONS(BUILD_BUILTIN_TEST_ENABLED)
endif(BUILD_BUILTIN_TEST_ENABLED)

execute_process(COMMAND git log --pretty=format:"%h" -n 1
  OUTPUT_VARIABLE BUILD_GIT_REVISION ERROR_QUIET)
add_compile_definitions(BUILD_GIT_REVISION=${BUILD_GIT_REVISION})
message("${CMAKE_CURRENT_SOURCE_DIR}: Git revision: ${BUILD_GIT_REVISION}")

add_subdirectory(src)

add_custom_target(iso_create_dirs COMMAND mkdir -pv iso_files/boot/grub)
add_dependencies(iso_create_dirs kernel kernel_symbols)

add_custom_target(iso_install_kernel COMMAND mv -v
  ${CMAKE_BINARY_DIR}/src/kernel
  ${CMAKE_CURRENT_BINARY_DIR}/iso_files/boot/kernel.bin)
add_dependencies(iso_install_kernel iso_create_dirs)

add_custom_target(iso_install_grub_cfg COMMAND cp -v
  ${CMAKE_SOURCE_DIR}/scripts/grub.cfg
  ${CMAKE_CURRENT_BINARY_DIR}/iso_files/boot/grub)
add_dependencies(iso_install_grub_cfg iso_create_dirs)

add_custom_target(iso ALL grub2-mkrescue -o cing.iso iso_files)
add_dependencies(iso iso_install_kernel iso_install_grub_cfg)
