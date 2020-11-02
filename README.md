qemu-system-x86_64  -vnc 100.81.235.146:0 -cdrom cing.iso -m 5G
qemu-system-x86_64  -curses -cdrom cing.iso -m 5G
qemu-system-x86_64  -curses -cdrom cing.iso -m 5G -s -S

## gdb
target remote localhost
symbol-file kernel.sym

