add_custom_target(asm_multiboot_header COMMAND ${TOOL_NASM} -f elf64
  ${CMAKE_SOURCE_DIR}/src/boot/multiboot_header.asm -o multiboot_header.o)
SET_SOURCE_FILES_PROPERTIES(
  ${CMAKE_CURRENT_BINARY_DIR}/multiboot_header.o
  PROPERTIES
  EXTERNAL_OBJECT true
  GENERATED true
)
add_custom_target(asm_boot COMMAND ${TOOL_NASM} -f elf64
  ${CMAKE_SOURCE_DIR}/src/boot/boot.asm -o boot.o)
SET_SOURCE_FILES_PROPERTIES(
  ${CMAKE_CURRENT_BINARY_DIR}/boot.o
  PROPERTIES
  EXTERNAL_OBJECT true
  GENERATED true
)
add_custom_target(asm_interrupt_definations COMMAND ${TOOL_NASM} -f elf64
  ${CMAKE_SOURCE_DIR}/src/interrupts/interrupts_definations.asm -o
  interrupts_definations.o)
SET_SOURCE_FILES_PROPERTIES(
  ${CMAKE_CURRENT_BINARY_DIR}/interrupts_definations.o
  PROPERTIES
  EXTERNAL_OBJECT true
  GENERATED true
)

set(SRC_MODULES
  containers
  cpu
  drivers
  interrupts
  kernel
  log
  mm
  panel
  util
  include
)
foreach(MOD ${SRC_MODULES})
  set(SRC_OF_MOD "")
  aux_source_directory(${MOD} SRC_OF_MOD)
  list(APPEND SRC_ALL ${SRC_OF_MOD})

  set(HEADER_OF_MOD "")
  file(GLOB HEADER_OF_MOD ${MOD}/*.h)
  list(APPEND HEADER_ALL ${HEADER_OF_MOD})
endforeach()

message("${CMAKE_CURRENT_SOURCE_DIR}: All sources: ${SRC_ALL}")
message("${CMAKE_CURRENT_SOURCE_DIR}: All headers: ${HEADER_ALL}")

add_executable(kernel ${SRC_ALL} )
add_dependencies(kernel asm_multiboot_header asm_boot asm_interrupt_definations)

set(FLAGS_COMPILE_KERNEL -ffreestanding -nostdlib -fno-builtin
  -mno-red-zone -std=gnu11 -fstack-protector-all
)
set(FLAGS_COMPILE_WARNINGS
  # -pedantic
  -Wall -Werror
  # -Wbad-function-cast
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

if(BUILD_DEBUG_ENABLED)
  set(FLAGS_COMPILE -O0 -fno-common -fno-omit-frame-pointer -ggdb3)
else()
  set(FLAGS_COMPILE -O3 -fno-common)
endif(BUILD_DEBUG_ENABLED)

target_compile_options(kernel PRIVATE ${FLAGS_COMPILE_KERNEL}
  ${FLAGS_COMPILE_WARNINGS} ${FLAGS_COMPILE}
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

# Strip debug symbols from image later:
# objcopy --strip-debug kernel
add_custom_target(kernel_symbols COMMAND objcopy --only-keep-debug
  ${CMAKE_BINARY_DIR}/src/kernel ${CMAKE_BINARY_DIR}/kernel.sym
)
add_dependencies(kernel_symbols kernel)

if(TOOL_CLANG_FORMAT)
  foreach(SRC_FILE ${SRC_ALL})
    add_custom_command(TARGET kernel PRE_BUILD
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      # Use ! to negate the result
      COMMAND ! ${TOOL_CLANG_FORMAT} -style=file -output-replacements-xml 
        -verbose ${SRC_FILE} | grep "replacement offset"
      COMMENT "Checking ${SRC_FILE} code style ..."
    )
  endforeach()
  foreach(HEADER_FILE ${HEADER_ALL})
    add_custom_command(TARGET kernel PRE_BUILD
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      # Use ! to negate the result
      COMMAND ! ${TOOL_CLANG_FORMAT} -style=file -output-replacements-xml 
        -verbose ${HEADER_FILE} | grep "replacement offset"
      COMMENT "Checking ${HEADER_FILE} code style ..."
    )
  endforeach()
endif()
