; This is a Multiboot-compliant header file in assembly code.
; Multiboot spec 2:
;     https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html
section .multiboot_header

%define MULTIBOOT2_MAGIC_NUMBER	0xe85250d6
%define PROTECTED_MODE_CODE		  0   ; architecture 0 (protected mode i386)
                                    ; architecture 4 (MIPS)

header_start:
    ; `dd` means 'define double word'
    dd MULTIBOOT2_MAGIC_NUMBER      ; magic number
    dd PROTECTED_MODE_CODE          ; architecture
    dd header_end - header_start    ; header length

    ; checksum
    dd 0x100000000 - (MULTIBOOT2_MAGIC_NUMBER + 0 + (header_end - header_start))

    ; List a series of “tags”, which is a way for the OS to tell the bootloader
    ; to do some extra things before handing control over to the OS, or to give
    ; the OS some extra information once started.
    ; Every tag should start at a 8 bytes algined offset

    align 8
    dw 5    ; type: Framebuffer tag
    dw 1    ; flags: Flags, set bit 0 means non-optinal tag
    dd 20   ; size
    dd 1920 ; Video width, in pixel or columns
    dd 1080 ; Video height, in pixel or rows
    dd 32   ; Video depth , in bits

    align 8
    ; required end tag
    ; `dw` means 'define word' (word = 16 bits on x86_64)
    dw 0    ; type
    dw 0    ; flags
    dd 8    ; size
header_end:
