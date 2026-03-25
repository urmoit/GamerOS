; boot.asm - COMPLETE VGA Mode 13h setup from scratch

section .multiboot_header
align 4
header_start:
    dd 0x1BADB002
    dd 0x00000007       ; Flags: ALIGN | MEMINFO | VIDEO_MODE
    dd -(0x1BADB002 + 0x00000007) ; Checksum

    ; Address fields (unused if bit 16 is not set, but kept for alignment if needed, usually 0)
    dd 0, 0, 0, 0, 0

    ; Graphics request fields (Multiboot 1): linear graphics mode 1920x1080x32.
    dd 0
    dd 1920
    dd 1080
    dd 32
header_end:

section .text
bits 32

%include "../src/graphics/font.inc"

VGA_GRAPHICS_BUFFER equ 0xA0000

global _start
_start:
    mov esp, stack_top
    jmp boot_main32

; Complete VGA Mode 13h initialization
set_vga_mode13h:
    push eax
    push edx
    push ecx

    ; First, set misc output register for proper timings
    mov dx, 0x3C2
    mov al, 0x63            ; Proper value for mode 13h
    out dx, al

    ; Disable video during setup
    mov dx, 0x3C4           ; Sequencer address
    mov al, 0x00            ; Reset register
    out dx, al
    mov dx, 0x3C5           ; Sequencer data
    mov al, 0x01            ; Synchronous reset
    out dx, al

    ; Set up sequencer registers for mode 13h
    mov dx, 0x3C4
    mov al, 0x01            ; Clocking mode
    out dx, al
    mov dx, 0x3C5
    mov al, 0x01            ; 8 dots per character
    out dx, al

    mov dx, 0x3C4
    mov al, 0x02            ; Map mask
    out dx, al
    mov dx, 0x3C5
    mov al, 0x0F            ; Enable all planes
    out dx, al

    mov dx, 0x3C4
    mov al, 0x03            ; Character map select
    out dx, al
    mov dx, 0x3C5
    mov al, 0x00
    out dx, al

    mov dx, 0x3C4
    mov al, 0x04            ; Memory mode - CRITICAL!
    out dx, al
    mov dx, 0x3C5
    mov al, 0x0E            ; Sequential, extended memory
    out dx, al

    ; Unprotect CRTC registers 0-7
    mov dx, 0x3D4
    mov al, 0x11
    out dx, al
    mov dx, 0x3D5
    in al, dx
    and al, 0x7F
    out dx, al

    ; Set CRTC registers for 320x200
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

    ; Graphics controller registers
    mov dx, 0x3CE
    mov al, 0x05            ; Mode register - CRITICAL!
    out dx, al
    mov dx, 0x3CF
    mov al, 0x40            ; 256-color mode
    out dx, al

    mov dx, 0x3CE
    mov al, 0x06            ; Miscellaneous register - CRITICAL!
    out dx, al
    mov dx, 0x3CF
    mov al, 0x05            ; A0000h, 64K, graphics mode
    out dx, al

    ; Set other GC registers
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
    mov al, 0x00
    out dx, al

.gc_skip2:
.gc_skip1:
    inc ecx
    cmp ecx, 9
    jl .gc_loop

    ; Attribute controller - set up for 256 colors
    mov dx, 0x3DA           ; Input status register
    in al, dx               ; Reset flip-flop

    ; Set palette registers
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

    ; Set attribute mode control
    mov dx, 0x3C0
    mov al, 0x10
    out dx, al
    mov al, 0x41            ; Graphics mode, enable line graphics
    out dx, al

    ; Enable video
    mov dx, 0x3C0
    mov al, 0x20
    out dx, al

    ; End sequencer reset
    mov dx, 0x3C4
    mov al, 0x00
    out dx, al
    mov dx, 0x3C5
    mov al, 0x03            ; Normal operation
    out dx, al

    ; Initialize palette
    call init_palette

    ; Clear screen to test
    mov edi, VGA_GRAPHICS_BUFFER
    mov ecx, 64000          ; 320*200
    mov al, 0x00            ; Black
    rep stosb

    pop ecx
    pop edx
    pop eax
    ret

; VGA Mode 12h (640x480x16) - 16-color planar mode
set_vga_mode12h:
    push eax
    push edx
    push ecx

    ; Set misc output register for 640x480
    mov dx, 0x3C2
    mov al, 0xE3            ; Proper value for mode 12h
    out dx, al

    ; Disable video during setup
    mov dx, 0x3C4
    mov al, 0x00
    out dx, al
    mov dx, 0x3C5
    mov al, 0x01
    out dx, al

    ; Setup sequencer registers
    mov dx, 0x3C4
    mov al, 0x01            ; Clocking mode
    out dx, al
    mov dx, 0x3C5
    mov al, 0x01            ; 8 dots per character
    out dx, al

    mov dx, 0x3C4
    mov al, 0x02            ; Map mask
    out dx, al
    mov dx, 0x3C5
    mov al, 0x0F            ; Enable all 4 planes
    out dx, al

    mov dx, 0x3C4
    mov al, 0x03
    out dx, al
    mov dx, 0x3C5
    mov al, 0x00
    out dx, al

    mov dx, 0x3C4
    mov al, 0x04            ; Memory mode
    out dx, al
    mov dx, 0x3C5
    mov al, 0x06            ; Planar mode, not chain-4
    out dx, al

    ; Unprotect CRTC registers
    mov dx, 0x3D4
    mov al, 0x11
    out dx, al
    mov dx, 0x3D5
    in al, dx
    and al, 0x7F
    out dx, al

    ; Set CRTC registers for 640x480
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

    ; Graphics controller setup for planar mode
    mov dx, 0x3CE
    mov al, 0x05            ; Mode register
    out dx, al
    mov dx, 0x3CF
    mov al, 0x00            ; Planar mode
    out dx, al

    mov dx, 0x3CE
    mov al, 0x06            ; Miscellaneous
    out dx, al
    mov dx, 0x3CF
    mov al, 0x05            ; A0000h, 64K, graphics mode
    out dx, al

    ; Set other GC registers to 0
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
    mov al, 0x00
    out dx, al

.gc_skip2_12h:
.gc_skip1_12h:
    inc ecx
    cmp ecx, 9
    jl .gc_loop_12h

    ; Attribute controller setup
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

    ; End sequencer reset
    mov dx, 0x3C4
    mov al, 0x00
    out dx, al
    mov dx, 0x3C5
    mov al, 0x03
    out dx, al

    ; Clear video memory (all planes)
    xor eax, eax
    mov edi, VGA_GRAPHICS_BUFFER
    mov ecx, 9600           ; 640*480/8/4 (38400 bytes / 4 bytes per dword)
    rep stosd

    pop ecx
    pop edx
    pop eax
    ret

; CRTC register data for mode 12h (640x480)
crtc_data_12h:
    db 0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E
    db 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0xEA, 0x8C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xC3
    db 0xFF

; CRTC register data for mode 13h (320x200)
crtc_data:
    db 0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F
    db 0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x9C, 0x8E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3
    db 0xFF

init_palette:
    push eax
    push ecx
    push edx

    ; Standard 16-color palette
    mov ecx, 0
.loop:
    mov dx, 0x3C8
    mov al, cl
    out dx, al

    ; Calculate RGB based on color index
    mov al, cl
    and al, 0x04            ; Red bit
    shr al, 2
    mul byte [intensity]
    mov bl, al

    mov al, cl
    and al, 0x02            ; Green bit
    shr al, 1
    mul byte [intensity]
    mov bh, al

    mov al, cl
    and al, 0x01            ; Blue bit
    mul byte [intensity]

    ; Output RGB
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

    ; Set specific colors we need
    ; Color 0x04 - Red
    mov dx, 0x3C8
    mov al, 0x04
    out dx, al
    mov dx, 0x3C9
    mov al, 0x3F
    out dx, al
    xor al, al
    out dx, al
    out dx, al

    ; Color 0x02 - Green
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

    ; Color 0x01 - Blue
    mov dx, 0x3C8
    mov al, 0x01
    out dx, al
    mov dx, 0x3C9
    xor al, al
    out dx, al
    out dx, al
    mov al, 0x3F
    out dx, al

    ; Color 0x0F - White
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

boot_main32:
    mov [multiboot_ptr], ebx  ; Save Multiboot pointer

    ; Disable NMI
    in al, 0x70
    or al, 0x80
    out 0x70, al

    ; Debug message (Legacy VGA text buffer for initial debug)
    mov byte [0xB8000], 'S'
    mov byte [0xB8001], 0x0E

    ; Force a known-good higher-resolution VGA graphics mode before entering long mode.
    ; BIOS interrupts are not available in 64-bit mode.
    call set_vga_mode12h

    ; Check CPU
    call detect_cpu
    jc .cpu_unsupported

    ; We run in VGA mode 12h and use a planar-present path in the 64-bit renderer.

    ; Setup paging - Identity map first 4GB using 2MB huge pages.
    ; Use page-aligned tables allocated in kernel BSS to avoid fixed-address conflicts.

    ; Clear P4, P3, and 4 P2 tables (6 pages = 24KB)
    mov edi, p4_table
    mov ecx, 512 * 6 * 2 ; 512 entries/page * 6 pages * 2 (8 bytes/entry)
    xor eax, eax
    rep stosd

    ; P4[0] -> P3
    mov eax, p3_table
    or eax, 0b11
    mov [p4_table], eax

    ; P3[0-3] -> P2 tables (4 entries)
    mov ecx, 0
.p3_init:
    mov eax, p2_tables
    mov edx, ecx
    shl edx, 12          ; ecx * 4096
    add eax, edx
    or eax, 0b11
    mov [p3_table + ecx*8], eax
    mov dword [p3_table + ecx*8 + 4], 0
    inc ecx
    cmp ecx, 4
    jl .p3_init

    ; Fill P2 tables with 2MB huge pages (2048 entries total = 4GB)
    mov edi, p2_tables
    mov ecx, 2048
    mov eax, 0x83        ; Present + Writable + Huge
.p2_loop:
    mov [edi], eax
    mov dword [edi + 4], 0 ; Clear upper 32 bits
    add eax, 0x200000    ; Next 2MB (will wrap at 4GB)
    add edi, 8
    loop .p2_loop

    lgdt [gdt64_descriptor]

    mov eax, cr4
    or eax, 1 << 5
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

bits 64
start_64:
    mov ax, 0x10
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    mov ebx, [multiboot_ptr] ; Restore Multiboot pointer to pass to kernel

    mov rsp, stack_top_64
    and rsp, 0xFFFFFFFFFFFFFFF0

    extern hal_init
    call hal_init

    extern kernel_main
    mov rdi, rbx        ; Pass multiboot info pointer (from EBX) as first argument (RDI)
    call kernel_main

    hlt

section .bss
global cpu_vendor
cpu_vendor:
    resb 12
    
multiboot_ptr:
    resd 1

align 4096
p4_table:
    resq 512
align 4096
p3_table:
    resq 512
align 4096
p2_tables:
    resq (512 * 4)

stack_bottom:
    resb 16384
stack_top:
stack_top_64 equ stack_top

section .data
vga_test_color db 0x00

gdt64:
    dq 0x0000000000000000
    dq 0x00AF9A000000FFFF
    dq 0x00AF92000000FFFF

gdt64_descriptor:
    dw gdt64_descriptor_end - gdt64 - 1
    dd gdt64
gdt64_descriptor_end:
