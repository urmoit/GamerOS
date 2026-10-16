; boot.asm - multiboot entry, VGA setup, long mode handoff


section .multiboot_header
align 8

; --- Multiboot 2 header ---
mb2_header_start:
    dd 0xE85250D6                       ; magic
    dd 0x00000000                       ; architecture: i386 (32-bit protected mode)
    dd mb2_header_end - mb2_header_start ; header_length
    dd -(0xE85250D6 + 0x00000000 + (mb2_header_end - mb2_header_start)) ; checksum

    ; Framebuffer request: 1920x1080x30 RGB. GRUB fills in addr/pitch; the
    ; rest of the header is static. Note: GRUB may refuse to load an MB2 image
    ; carrying a framebuffer tag when it cannot set the requested console mode;
    ; in that case boot with the "multiboot" (MB1) command instead.
    dd 8                                ; type 8 = framebuffer
    dd 32                               ; size (8-byte header + payload, 8-aligned)
    dq 0                                ; addr (filled in by loader)
    dd 0                                ; pitch (filled in by loader)
    dd 1920                             ; width
    dd 1080                             ; height
    db 30                               ; bpp
    db 1                                ; type = RGB

    ; End tag: end of header tags.
    dd 0                                ; type 0 = end, flags 0
    dd 8                                ; size
mb2_header_end:

; --- Multiboot 1 header ---
align 4
header_start:
    dd 0x1BADB002
    dd 0x00000007       ; Flags: ALIGN | MEMINFO | VIDEO_MODE
    dd -(0x1BADB002 + 0x00000007) ; Checksum

    ; Address fields (unused, kept as zeroes)
    dd 0, 0, 0, 0, 0

    ; Graphics request: linear framebuffer 1920x1080x32.
    dd 0
    dd 1920
    dd 1080
    dd 32
header_end:

section .text
bits 32

%include "graphics/font.inc"

VGA_GRAPHICS_BUFFER equ 0xA0000

global _start
_start:
    ; EAX = boot magic (MB1 0x2BADB002 / MB2 0x36D76289), EBX = info pointer.
    mov [multiboot_magic], eax
    mov [multiboot_ptr], ebx
    mov esp, stack_top
    jmp boot_main32

; VGA mode 13h (320x200x256) setup
set_vga_mode13h:
    push eax
    push edx
    push ecx

    mov dx, 0x3C2
    mov al, 0x63
    out dx, al

    mov dx, 0x3C4
    mov al, 0x00
    out dx, al
    mov dx, 0x3C5
    mov al, 0x01
    out dx, al

    mov dx, 0x3C4
    mov al, 0x01
    out dx, al
    mov dx, 0x3C5
    mov al, 0x01
    out dx, al

    mov dx, 0x3C4
    mov al, 0x02
    out dx, al
    mov dx, 0x3C5
    mov al, 0x0F
    out dx, al

    mov dx, 0x3C4
    mov al, 0x03
    out dx, al
    mov dx, 0x3C5
    mov al, 0x00
    out dx, al

    mov dx, 0x3C4
    mov al, 0x04
    out dx, al
    mov dx, 0x3C5
    mov al, 0x0E
    out dx, al

    mov dx, 0x3D4
    mov al, 0x11
    out dx, al
    mov dx, 0x3D5
    in al, dx
    and al, 0x7F
    out dx, al

    mov esi, crtc_data
    xor ecx, ecx
.crtc_loop:
    mov dx, 0x3D4
    mov al, cl
    out dx, al
    mov dx, 0x3D5
    mov al, [esi]
    out dx, al
    inc esi
    inc ecx
    cmp ecx, 25
    jl .crtc_loop

    mov dx, 0x3CE
    mov al, 0x05
    out dx, al
    mov dx, 0x3CF
    mov al, 0x40
    out dx, al

    mov dx, 0x3CE
    mov al, 0x06
    out dx, al
    mov dx, 0x3CF
    mov al, 0x05
    out dx, al

    xor ecx, ecx
.gc_loop:
    cmp ecx, 5
    je .gc_skip1
    cmp ecx, 6
    je .gc_skip2

    mov dx, 0x3CE
    mov al, cl
    out dx, al
    mov dx, 0x3CF
    ; Register 8 is the plane bit mask: 0x00 would disable every plane write.
    cmp ecx, 8
    je .gc_bitmask
    mov al, 0x00
    out dx, al
    jmp .gc_next
.gc_bitmask:
    mov al, 0xFF
    out dx, al
.gc_next:

.gc_skip2:
.gc_skip1:
    inc ecx
    cmp ecx, 9
    jl .gc_loop

    ; Attribute controller
    mov dx, 0x3DA
    in al, dx               ; Reset flip-flop

    xor ecx, ecx
.attr_loop:
    mov dx, 0x3C0
    mov al, cl
    out dx, al
    mov al, cl
    out dx, al
    inc ecx
    cmp ecx, 16
    jl .attr_loop

    mov dx, 0x3C0
    mov al, 0x10
    out dx, al
    mov al, 0x41
    out dx, al

    mov dx, 0x3C0
    mov al, 0x20
    out dx, al

    mov dx, 0x3C4
    mov al, 0x00
    out dx, al
    mov dx, 0x3C5
    mov al, 0x03
    out dx, al

    call init_palette

    mov edi, VGA_GRAPHICS_BUFFER
    mov ecx, 64000
    mov al, 0x00
    rep stosb

    pop ecx
    pop edx
    pop eax
    ret

; VGA mode 12h (640x480x16) setup
set_vga_mode12h:
    push eax
    push edx
    push ecx

    mov dx, 0x3C2
    mov al, 0xE3
    out dx, al

    mov dx, 0x3C4
    mov al, 0x00
    out dx, al
    mov dx, 0x3C5
    mov al, 0x01
    out dx, al

    mov dx, 0x3C4
    mov al, 0x01
    out dx, al
    mov dx, 0x3C5
    mov al, 0x01
    out dx, al

    mov dx, 0x3C4
    mov al, 0x02
    out dx, al
    mov dx, 0x3C5
    mov al, 0x0F
    out dx, al

    mov dx, 0x3C4
    mov al, 0x03
    out dx, al
    mov dx, 0x3C5
    mov al, 0x00
    out dx, al

    mov dx, 0x3C4
    mov al, 0x04
    out dx, al
    mov dx, 0x3C5
    mov al, 0x06
    out dx, al

    mov dx, 0x3D4
    mov al, 0x11
    out dx, al
    mov dx, 0x3D5
    in al, dx
    and al, 0x7F
    out dx, al

    mov esi, crtc_data_12h
    xor ecx, ecx
.crtc_loop_12h:
    mov dx, 0x3D4
    mov al, cl
    out dx, al
    mov dx, 0x3D5
    mov al, [esi]
    out dx, al
    inc esi
    inc ecx
    cmp ecx, 25
    jl .crtc_loop_12h

    mov dx, 0x3CE
    mov al, 0x05
    out dx, al
    mov dx, 0x3CF
    mov al, 0x00
    out dx, al

    mov dx, 0x3CE
    mov al, 0x06
    out dx, al
    mov dx, 0x3CF
    mov al, 0x05
    out dx, al

    xor ecx, ecx
.gc_loop_12h:
    cmp ecx, 5
    je .gc_skip1_12h
    cmp ecx, 6
    je .gc_skip2_12h

    mov dx, 0x3CE
    mov al, cl
    out dx, al
    mov dx, 0x3CF
    ; Register 8 is the plane bit mask: 0x00 would disable every plane write.
    cmp ecx, 8
    je .gc_bitmask_12h
    mov al, 0x00
    out dx, al
    jmp .gc_next_12h
.gc_bitmask_12h:
    mov al, 0xFF
    out dx, al
.gc_next_12h:

.gc_skip2_12h:
.gc_skip1_12h:
    inc ecx
    cmp ecx, 9
    jl .gc_loop_12h

    ; Attribute controller
    mov dx, 0x3DA
    in al, dx               ; Reset flip-flop

    xor ecx, ecx
.attr_loop_12h:
    mov dx, 0x3C0
    mov al, cl
    out dx, al
    mov al, cl
    out dx, al
    inc ecx
    cmp ecx, 16
    jl .attr_loop_12h

    mov dx, 0x3C0
    mov al, 0x10
    out dx, al
    mov al, 0x41
    out dx, al

    mov dx, 0x3C0
    mov al, 0x20
    out dx, al

    mov dx, 0x3C4
    mov al, 0x00
    out dx, al
    mov dx, 0x3C5
    mov al, 0x03
    out dx, al

    xor eax, eax
    mov edi, VGA_GRAPHICS_BUFFER
    mov ecx, 9600           ; 640*480/8/4
    rep stosd

    pop ecx
    pop edx
    pop eax
    ret

; CRTC data: mode 12h (640x480)
crtc_data_12h:
    db 0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E
    db 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0xEA, 0x8C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xC3
    db 0xFF

; CRTC data: mode 13h (320x200)
crtc_data:
    db 0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F
    db 0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x9C, 0x8E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3
    db 0xFF

init_palette:
    push eax
    push ecx
    push edx

    mov ecx, 0
.loop:
    mov dx, 0x3C8
    mov al, cl
    out dx, al

    mov al, cl
    and al, 0x04
    shr al, 2
    mul byte [intensity]
    mov bl, al

    mov al, cl
    and al, 0x02
    shr al, 1
    mul byte [intensity]
    mov bh, al

    mov al, cl
    and al, 0x01
    mul byte [intensity]

    mov dx, 0x3C9
    mov al, bl
    out dx, al
    mov al, bh
    out dx, al
    mov al, cl
    and al, 0x01
    mul byte [intensity]
    out dx, al

    inc ecx
    cmp ecx, 256
    jl .loop

    mov dx, 0x3C8
    mov al, 0x04
    out dx, al
    mov dx, 0x3C9
    mov al, 0x3F
    out dx, al
    xor al, al
    out dx, al
    out dx, al

    mov dx, 0x3C8
    mov al, 0x02
    out dx, al
    mov dx, 0x3C9
    xor al, al
    out dx, al
    mov al, 0x3F
    out dx, al
    xor al, al
    out dx, al

    mov dx, 0x3C8
    mov al, 0x01
    out dx, al
    mov dx, 0x3C9
    xor al, al
    out dx, al
    out dx, al
    mov al, 0x3F
    out dx, al

    mov dx, 0x3C8
    mov al, 0x0F
    out dx, al
    mov dx, 0x3C9
    mov al, 0x3F
    out dx, al
    out dx, al
    out dx, al

    pop edx
    pop ecx
    pop eax
    ret

intensity db 0x3F

detect_cpu:
    xor eax, eax
    cpuid

    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .fail

    mov eax, 0x80000001
    cpuid
    test edx, (1 << 29)
    jz .fail

    clc
    ret
.fail:
    stc
    ret

; --- 32-bit entry (boot magic and pointer already saved in _start) ---
boot_main32:

    in al, 0x70
    or al, 0x80
    out 0x70, al

    ; Debug 'S' in the legacy VGA text buffer at 0xB8000.
    mov byte [0xB8000], 'S'
    mov byte [0xB8001], 0x0E

    ; Force a VGA mode now: BIOS interrupts are gone in long mode.
    call set_vga_mode12h

    call detect_cpu
    jc .cpu_unsupported

    ; --- Paging: identity-map the first 4 GiB with 2 MiB huge pages ---


    mov edi, p4_table
    mov ecx, 512 * 6 * 2 ; 6 pages * 512 entries * 2 dwords
    xor eax, eax
    rep stosd

    mov eax, p3_table
    or eax, 0b11
    mov [p4_table], eax

    mov ecx, 0
.p3_init:
    mov eax, p2_tables
    mov edx, ecx
    shl edx, 12
    add eax, edx
    or eax, 0b11
    mov [p3_table + ecx*8], eax
    mov dword [p3_table + ecx*8 + 4], 0
    inc ecx
    cmp ecx, 4
    jl .p3_init

    mov edi, p2_tables
    mov ecx, 2048
    mov eax, 0x83        ; Present + Writable + Huge
.p2_loop:
    mov [edi], eax
    mov dword [edi + 4], 0
    add eax, 0x200000    ; next 2 MiB page (wraps at 4 GiB)
    add edi, 8
    loop .p2_loop

    lgdt [gdt64_descriptor]

    ; PAE/OSFXSR/OSXMMEXCPT must be set before any SSE opcode (#UD otherwise).
    mov eax, cr4
    or eax, 1 << 5         ; CR4.PAE
    or eax, 1 << 9         ; CR4.OSFXSR
    or eax, 1 << 10        ; CR4.OSXMMEXCPT
    mov cr4, eax

    mov eax, p4_table
    mov cr3, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    jmp 0x08:start_64

.cpu_unsupported:
    hlt

; --- Long mode ---
bits 64
start_64:
    mov ax, 0x10
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov rsp, stack_top_64
    and rsp, 0xFFFFFFFFFFFFFFF0

    extern hal_init
    call hal_init

    ; Normalise MB1/MB2 info into one multiboot_info_t; slots read as 32-bit words.
    extern multiboot2_build
    mov edi, [multiboot_magic]      ; boot magic (zero-extended)
    mov esi, [multiboot_ptr]        ; information pointer (zero-extended)
    call multiboot2_build

    extern kernel_main
    mov rdi, rax        ; Multiboot information pointer as first argument
    call kernel_main

    hlt

section .bss
global cpu_vendor
cpu_vendor:
    resb 12
    
multiboot_ptr:
    resd 1

; Boot magic handed over by the loader in EAX at entry (next to multiboot_ptr).
multiboot_magic:
    resd 1

; alignb, not align: plain align emits fill bytes, which NASM rejects in .bss.
alignb 4096
p4_table:
    resq 512
alignb 4096
p3_table:
    resq 512
alignb 4096
p2_tables:
    resq (512 * 4)

stack_bottom:
    resb 16384
stack_top:
stack_top_64 equ stack_top

section .data
vga_test_color db 0x00

; --- GDT ---
gdt64:
    dq 0x0000000000000000
    dq 0x00AF9A000000FFFF
    dq 0x00AF92000000FFFF

gdt64_descriptor:
    dw gdt64_descriptor_end - gdt64 - 1
    dd gdt64
gdt64_descriptor_end:
