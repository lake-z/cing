add_custom_target(asm_multiboot_header COMMAND ${BUILD_TOOL_NASM} -f elf64
  ${CMAKE_SOURCE_DIR}/src/boot/multiboot_header.asm -o multiboot_header.o)
SET_SOURCE_FILES_PROPERTIES(
  ${CMAKE_CURRENT_BINARY_DIR}/multiboot_header.o
  PROPERTIES
  EXTERNAL_OBJECT true
  GENERATED true
)

add_custom_target(asm_boot COMMAND ${BUILD_TOOL_NASM} -f elf64
  ${CMAKE_SOURCE_DIR}/src/boot/boot.asm -o boot.o)
SET_SOURCE_FILES_PROPERTIES(
  ${CMAKE_CURRENT_BINARY_DIR}/boot.o
  PROPERTIES
  EXTERNAL_OBJECT true
  GENERATED true
)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/kernel BUILD_SRC_ALL)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/drivers BUILD_SRC_ALL)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/containers BUILD_SRC_ALL)
message("${CMAKE_CURRENT_SOURCE_DIR}: All sources: ${BUILD_SRC_ALL}")

add_executable(kernel ${BUILD_SRC_ALL} )
add_dependencies(kernel asm_multiboot_header asm_boot)

# FIXME add -fstack-protector-all here
set(BUILD_FLAGS_COMPILE_KERNEL -ffreestanding -nostdlib -fno-builtin
  -mno-red-zone -std=gnu11
)
set(BUILD_FLAGS_COMPILE_WARNINGS
  -pedantic 
  -Wall -Werror
  -Wbad-function-cast
  -Wcast-align
  -Wconversion 
  -Wextra
  -Wfloat-equal
  -Wformat=2
  -Winline 
  -Wmissing-prototypes
  -Wnested-externs
  -Wpointer-arith
  -Wshadow 
  -Wstrict-aliasing=2
  -Wstrict-prototypes 
  -Wswitch-default
  -Wundef 
  -Wunreachable-code
  -Wwrite-strings 
)

set(BUILD_FLAGS_COMPILE_DEBUG -O0 -fno-common -fno-omit-frame-pointer -ggdb3)
target_compile_options(kernel PRIVATE ${BUILD_FLAGS_COMPILE_KERNEL} 
  ${BUILD_FLAGS_COMPILE_WARNINGS} ${BUILD_FLAGS_COMPILE_DEBUG}
)

target_include_directories(kernel PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(kernel PRIVATE 
  ${CMAKE_CURRENT_BINARY_DIR}/multiboot_header.o 
  ${CMAKE_CURRENT_BINARY_DIR}/boot.o)

target_link_options(kernel PRIVATE --nmagic
  --script=${CMAKE_SOURCE_DIR}/scripts/linker.ld)
