# Tools

## for cing

### qemu

```
qemu-system-x86_64 -vnc 100.81.235.146:0 -cdrom cing.iso -m 5G
qemu-system-x86_64 -curses -cdrom cing.iso -m 5G
qemu-system-x86_64 -curses -cdrom cing.iso -m 5G -s -S
qemu-system-x86_64 -vnc 100.81.235.146:0 -cdrom cing.iso -m 5G -monitor stdio -serial file:log.txt
qemu-system-x86_64
  -machine q35
  -vnc 100.81.235.146:0
  -cdrom cing.iso -m 5G
  -serial stdio
  -drive file=../nvme1.raw,format=raw,if=none,id=nvme0
  -device nvme,drive=nvme0,serial=deadbeaf1,max_ioqpairs=8
```

### qemu monitor

```
info mem
(Lab 2+) Display mapped virtual memory and permissions. For example,
ef7c0000-ef800000 00040000 urw
efbf8000-efc00000 00008000 -rw

tells us that the 0x00040000 bytes of memory from 0xef7c0000 to 0xef800000 are
mapped read/write and user-accessible, while the memory from 0xefbf8000 to
0xefc00000 is mapped read/write, but only kernel-accessible.

info tlb
```

### gdb

```
target remote localhost:1234
symbol-file kernel.sym

layout split 
Divides the window into two parts - one of them displaying the source code, the 
other one the corresponding assembly.

ni
next instruction

si 
step instruction

set disassembly-flavor intel

set print asm-demangle
demangles C++ names in assembly view

```

### cmake

```
cmake .. -DTOOL_NASM=/home/t4/wanghu/os/install/bin/nasm -DBUILD_SELF_TEST_ENABLED=OFF
cmake .. -DTOOL_NASM=/home/t4/wanghu/os/install/bin/nasm -DTOOL_CLANG=/usr/local/histore-clang
```

### clang

```
/usr/local/histore-clang/bin/clang-format -style=file -fallback-style=none -sort-includes -i src/kernel/*.c
```



## for linux hacking

### build kernel

```
export ARCH=x86 

make  x86_64_defconfig

make menuconfig

Check the configuration. Set CONFIG_DEBUG_INFO=y if not configured.
```

### prepare rootfs

```
mkdir etc dev mnt 
mkdir -p proc sys tmp mnt 
mkdir -p etc/init.d/ 

mknod console c 5 1
mknod null c 1 3
mknod tty1 c 4 1

chmod 755 etc/init.d/rcS
chmod 755 etc/inittab
cd dev
mknod console c 5 1
mknod null c 1 3
mknod tty1 c 4 1

dd if=/dev/zero of=./rootfs.ext3 bs=1M count=32

mkfs.ext3 rootfs.ext3
mkdir rootfs.mnt

mount -o loop rootfs.ext3 rootfs.mnt

cp -rvf ./rootfs/* ./rootfs.mnt


gzip --best -c rootfs.ext3 > rootfs.img.gz

https://www.bilibili.com/read/cv11271232?spm_id_from=333.999.0.0

```

### qemu

```
qemu-system-x86_64 -kernel linux-5.10.80/arch/x86_64/boot/bzImage -initrd  rootfs.img.gz  -append 'root=/dev/ram init=/linuxrc console=ttyS0 nokaslr'   -nographic 

Add kernel parameter nokaslr. It is required to make breakpoint work correctly.

```

### gdb

```


$ echo "add-auto-load-safe-path /opt/zhou/code/kernel/playground/linux-5.10.80/vmlinux-gdb.py" >> ~/.gdbinit


$ gdb vmlinux
(gdb) target remote :1234
(gdb) hbreak start_kernel
(gdb) c
(gdb) lx-dmesg


Make symbol file from vmlinux kernel image. vmlinux file is located in the kernel source directory (generated after compilation). 
objcopy --only-keep-debug vmlinux kernel.sym

```

### kernel logging

```
https://www.kernel.org/doc/html/latest/core-api/printk-basics.html
```