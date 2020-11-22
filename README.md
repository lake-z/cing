# Tools

## qemu

```
qemu-system-x86_64  -vnc 100.81.235.146:0 -cdrom cing.iso -m 5G
qemu-system-x86_64  -curses -cdrom cing.iso -m 5G
qemu-system-x86_64  -curses -cdrom cing.iso -m 5G -s -S
qemu-system-x86_64 -vnc 100.81.235.146:0 -cdrom cing.iso -m 5G -monitor stdio -serial file:log.txt
```

```
info mem
(Lab 2+) Display mapped virtual memory and permissions. For example,
ef7c0000-ef800000 00040000 urw
efbf8000-efc00000 00008000 -rw

tells us that the 0x00040000 bytes of memory from 0xef7c0000 to 0xef800000 are
mapped read/write and user-accessible, while the memory from 0xefbf8000 to
0xefc00000 is mapped read/write, but only kernel-accessible.
```

## gdb

```
target remote localhost
symbol-file kernel.sym
```

## cmake

```
cmake ..
  -DBUILD_TOOL_NASM=/home/t4/wanghu/os/install/bin/nasm
  -DBUILD_TOOL_CLANG=/usr/local/histore-clang
```
