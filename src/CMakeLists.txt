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
add_custom_target(asm_interrupt_definations COMMAND ${BUILD_TOOL_NASM} -f elf64
  ${CMAKE_SOURCE_DIR}/src/interrupts/interrupts_definations.asm -o
  interrupts_definations.o)
SET_SOURCE_FILES_PROPERTIES(
  ${CMAKE_CURRENT_BINARY_DIR}/interrupts_definations.o
  PROPERTIES
  EXTERNAL_OBJECT true
  GENERATED true
)

set(BUILD_SRC_MODULES 
  include
  containers  
  drivers
  interrupts
  kernel
  mm
)
foreach(MOD ${BUILD_SRC_MODULES})
  set(SRC_OF_MOD "")
  aux_source_directory(${MOD} SRC_OF_MOD)
  list(APPEND BUILD_SRC_ALL ${SRC_OF_MOD})

  set(HEADER_OF_MOD "")
  file(GLOB HEADER_OF_MOD ${MOD}/*.h)
  list(APPEND BUILD_HEADER_ALL ${HEADER_OF_MOD})
endforeach()

message("${CMAKE_CURRENT_SOURCE_DIR}: All sources: ${BUILD_SRC_ALL}")
message("${CMAKE_CURRENT_SOURCE_DIR}: All headers: ${BUILD_HEADER_ALL}")

add_executable(kernel ${BUILD_SRC_ALL} )
add_dependencies(kernel asm_multiboot_header asm_boot asm_interrupt_definations)

set(BUILD_FLAGS_COMPILE_KERNEL -ffreestanding -nostdlib -fno-builtin
  -mno-red-zone -std=gnu11 -fstack-protector-all
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
  ${CMAKE_CURRENT_BINARY_DIR}/boot.o
  ${CMAKE_CURRENT_BINARY_DIR}/interrupts_definations.o
)

target_link_options(kernel PRIVATE --nmagic
  --script=${CMAKE_SOURCE_DIR}/scripts/linker.ld)

# Fix cide format by:
# clang-format -style=file -fallback-style=none -sort-includes -i <path to src>
if(BUILD_TOOL_CLANG_FORMAT)
  foreach(SRC_FILE ${BUILD_SRC_ALL})
    add_custom_command(TARGET kernel PRE_BUILD
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      # Use ! to negate the result
      COMMAND ! ${BUILD_TOOL_CLANG_FORMAT} -style=file -output-replacements-xml 
        -verbose ${SRC_FILE} | grep "replacement offset"
      COMMENT "Checking ${SRC_FILE} code style ..."
    )
  endforeach()
  foreach(HEADER_FILE ${BUILD_HEADER_ALL})
    add_custom_command(TARGET kernel PRE_BUILD
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      # Use ! to negate the result
      COMMAND ! ${BUILD_TOOL_CLANG_FORMAT} -style=file -output-replacements-xml 
        -verbose ${HEADER_FILE} | grep "replacement offset"
      COMMENT "Checking ${HEADER_FILE} code style ..."
    )
  endforeach()
endif()
