global start	; exports a label (makes it public). As start will be the entry
		; point of our kernel, it needs to be public.
global boot_stack_bottom
global boot_stack_top

section .text	; executable code
bits 32		; specifies that the following lines are 32-bit instructions.
		; it's needed because the CPU is still in Protected mode when
		; GRUB starts our kernel. When we switch to Long mode, we can
		; use bits 64 (64-bit instructions).
start:
  ; Set up stack
	mov esp, boot_stack_bottom
  ; According to the ABI, the first 6 integer or pointer arguments to a
  ; function are passed in registers. The first is placed in rdi, the second in 
  ; rsi, the third in rdx, and then rcx, r8 and r9. Only the 7th argument and
  ; onwards are passed on the stack.
	mov edi, ebx ; ebx points to a boot information structure.

	; various checks before we can move on.
	call check_multiboot
	call check_cpuid
	call check_long_mode

  ; When we switch to long mode, paging will be activated automatically.
	call set_up_page_tables
	call enable_paging

    lgdt [gdt64.pointer]    ; load the 64-bit GDT
    jmp gdt64.code:long_mode_start

    ; Should not be reached.
    hlt

; -----------------------------------------------------------------------------
; make sure the kernel was really loaded by a Multiboot compliant bootloader

%define MULTIBOOT2_MAGIC_VALUE 0x36d76289

check_multiboot:
	cmp eax, MULTIBOOT2_MAGIC_VALUE
	jne .no_multiboot
	ret
.no_multiboot:
	mov al, "0"
	jmp error

; -----------------------------------------------------------------------------
; CPUID check
; cf. http://wiki.osdev.org/Setting_Up_Long_Mode#Detection_of_CPUID

check_cpuid:
    ; Check if CPUID is supported by attempting to flip the ID bit (bit 21)
    ; in the FLAGS register. If we can flip it, CPUID is available.

    ; Copy FLAGS in to EAX via stack
    pushfd
    pop eax

    ; Copy to ECX as well for comparing later on
    mov ecx, eax

    ; Flip the ID bit
    xor eax, 1 << 21

    ; Copy EAX to FLAGS via the stack
    push eax
    popfd

    ; Copy FLAGS back to EAX (with the flipped bit if CPUID is supported)
    pushfd
    pop eax

    ; Restore FLAGS from the old version stored in ECX (i.e. flipping the ID
    ; bit back if it was ever flipped).
    push ecx
    popfd

    ; Compare EAX and ECX. If they are equal then that means the bit wasn't
    ; flipped, and CPUID isn't supported.
    cmp eax, ecx
    je .no_cpuid
    ret
.no_cpuid:
    mov al, "1"
    jmp error

; -----------------------------------------------------------------------------
; Long Mode check

check_long_mode:
    ; test if extended processor info in available
    mov eax, 0x80000000    ; implicit argument for cpuid
    cpuid                  ; get highest supported argument
    cmp eax, 0x80000001    ; it needs to be at least 0x80000001
    jb .no_long_mode       ; if it's less, the CPU is too old for long mode

    ; use extended info to test if long mode is available
    mov eax, 0x80000001    ; argument for extended processor info
    cpuid                  ; returns various feature bits in ecx and edx
    test edx, 1 << 29      ; test if the LM-bit is set in the D-register
    jz .no_long_mode       ; If it's not set, there is no long mode
    ret
.no_long_mode:
    mov al, "2"
    jmp error

; -----------------------------------------------------------------------------
; We setting up identity mapping here,
; i.e. map a physical address to the same virtual address.

set_up_page_tables:
    ; See: https://os.phil-opp.com/page-tables/#implementation
    mov eax, p4_table
    or eax, 0b11 ; present + writable
    mov [p4_table + 511 * 8], eax

    ; Point the first entry of the level 4 page table to the first entry in the
    ; p3 table.
    mov eax, p3_table
    or eax, 0b11    ; present + writable
    mov dword [p4_table], eax

    ; Point the first entry of the level 3 page table to the first entry in the
    ; p2 table
    mov eax, p2_table
    or eax, 0b11    ; present + writable
    mov dword [p3_table], eax

    ; point each page table level two entry to a page
    mov ecx, 0         ; counter variable
.map_p2_table:
    ; map ecx-th P2 entry to a huge page that starts at address 2MiB*ecx
    mov eax, 0x200000		    ; 2MiB
    mul ecx			    ; start address of ecx-th page
    or eax, 0b10000011		    ; present + writable + huge
    mov [p2_table + ecx * 8], eax   ; map ecx-th entry

    inc ecx		; increase counter
    cmp ecx, 512	; if counter == 512, the whole P2 table is mapped
    jne .map_p2_table	; else map the next entry

    ret

; -----------------------------------------------------------------------------
; Enable paging to enter long-mode.

enable_paging:
    ; load P4 to cr3 register (cpu uses this to access the P4 table)
    mov eax, p4_table
    mov cr3, eax

    ; Long mode is an extension of Physical Address Extension (PAE),
    ; so we need to enable PAE first
    ; enable PAE-flag in cr4 (Physical Address Extension)
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; set the long mode bit in the EFER MSR (model specific register)
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; enable paging in the cr0 register
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ret

; -----------------------------------------------------------------------------
; Prints `ERR: ` and the given error code to screen and hangs.
;
; Parameter: error code (in ascii) in al
;
;	0 = no multiboot
;	1 = no CPUID
;	2 = no long mode
error:
	mov dword [0xb8000], 0x4f524f45
	mov dword [0xb8004], 0x4f3a4f52
	mov dword [0xb8008], 0x4f204f20
	mov byte  [0xb800a], al
	hlt

; -----------------------------------------------------------------------------

section .bss
align 4096
p4_table:
    ; `resb` means 'reserves bytes'
    resb 4096
p3_table:
    resb 4096
p2_table:
    resb 4096
boot_stack_top:
    resb 4096
boot_stack_bottom:

; -----------------------------------------------------------------------------
; After enable paging, the processor is still in a 32-bit compatibility
; submode. To actually execute 64-bit code, we need to set up a new Global
; Descriptor Table.

; Setting up 3 segments here: the required NULL segment (the very first element 
; in the GDT has to be an invalid "NULL" segment, to make a segment register
; loaded with 0x0 invalid), a code segment, and a data segment

; Most bits of segment descriptors are not used.

; Only 1 bit of data segment is used:
; Bit 15: P, the Present bit, should be set in order to use the segment.

; Only 6 bits of code segment is used:
; Bits 10: C (conforming bit)
; Bits 13..14: The two DPL bits, to set protection ring
; Bit 15: P, the Present bit, should be set in order to use the segment.
; Bit 21: L, determines whether the code in this segment should be interpreted
;   as 32-bit or 64-bit code; set it as we'd like 64-bit code.
; Bit 22: D, should be clear for 64-bit code.

section .rodata
gdt64:
    ; `dq` means 'define quad-word', which is 8 bytes len
    dq 0
.code: equ $ - gdt64
    dq (1<<44) | (1<<47) | (1<<41) | (1<<43) | (1<<53)
.data: equ $ - gdt64
    dq (1<<44) | (1<<47) | (1<<41)
; To load our new 64-bit GDT, we have to tell the CPU its address and length.
.pointer:
    dw .pointer - gdt64 - 1
    dq gdt64

; -----------------------------------------------------------------------------
; 64-bit code below

extern kernal_main

section .text
bits 64
long_mode_start:
	; load 0 into all data segment registers
	mov ax, 0
	mov ss, ax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	call kernal_main

	; Should not happen.
	hlt
