; boot.asm - COMPLETE VGA Mode 13h setup from scratch

section .multiboot_header
align 8
header_start:
    dd 0x1BADB002
    dd 0x00000003
    dd 0xE4524FFB
header_end:

section .text
bits 32

%include "src/impl/graphics/font.inc"

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
    mov ecx, 153600         ; 640*480/4 (4 planes)
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
    ; Disable NMI
    in al, 0x70
    or al, 0x80
    out 0x70, al

    ; Debug message
    mov byte [0xB8000], 'S'
    mov byte [0xB8001], 0x0E

    ; Check CPU
    call detect_cpu
    jc .cpu_unsupported

    ; Set VGA mode 12h (640x480x16)
    call set_vga_mode12h

    ; Draw test pattern using proper sequencer access
    mov edi, VGA_GRAPHICS_BUFFER
    mov ebx, 0              ; Color stripe counter
    
.stripe_loop:
    cmp ebx, 5
    jge .stripes_done
    
    ; Determine color based on stripe
    mov al, bl
    cmp al, 0
    jne .not_red
    mov byte [vga_test_color], 0x04            ; Red
    jmp .paint_stripe
.not_red:
    cmp al, 1
    jne .not_green
    mov byte [vga_test_color], 0x02            ; Green
    jmp .paint_stripe
.not_green:
    cmp al, 2
    jne .not_blue
    mov byte [vga_test_color], 0x01            ; Blue
    jmp .paint_stripe
.not_blue:
    cmp al, 3
    jne .not_yellow
    mov byte [vga_test_color], 0x0E            ; Yellow
    jmp .paint_stripe
.not_yellow:
    mov byte [vga_test_color], 0x0F            ; White
    
.paint_stripe:
    ; Set graphics controller for set/reset mode
    mov dx, 0x3CE
    mov al, 0x05
    out dx, al
    mov dx, 0x3CF
    mov al, 0x01            ; Write mode 1
    out dx, al
    
    ; Set color in set/reset register
    mov dx, 0x3CE
    mov al, 0x00
    out dx, al
    mov al, [vga_test_color]
    mov dx, 0x3CF
    out dx, al
    
    ; Enable all planes
    mov dx, 0x3CE
    mov al, 0x01
    out dx, al
    mov dx, 0x3CF
    mov al, 0x0F
    out dx, al
    
    ; Fill stripe (12800 bytes)
    mov ecx, 12800
    mov edx, 0xA0000
.fill_stripe:
    mov byte [edx], 0xFF
    inc edx
    loop .fill_stripe
    
    inc ebx
    jmp .stripe_loop
    
.stripes_done:
    ; Reset graphics controller
    mov dx, 0x3CE
    mov al, 0x05
    out dx, al
    mov dx, 0x3CF
    mov al, 0x00            ; Back to write mode 0
    out dx, al
    
    mov dx, 0x3CE
    mov al, 0x01
    out dx, al
    mov dx, 0x3CF
    mov al, 0x00
    out dx, al

    ; Setup paging
    %define P4_TABLE 0x200000
    %define P3_TABLE 0x201000
    %define P2_TABLE 0x202000

    mov edi, P4_TABLE
    mov ecx, 512 * 3
    xor eax, eax
    rep stosd

    mov eax, P3_TABLE
    or eax, 0b11
    mov [P4_TABLE], eax

    mov eax, P2_TABLE
    or eax, 0b11
    mov [P3_TABLE], eax

    mov eax, 0x0
    or eax, 0b10000011
    mov [P2_TABLE], eax

    mov eax, 0x200000
    or eax, 0b10000011
    mov [P2_TABLE + 8], eax

    lgdt [gdt64_descriptor]

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov eax, P4_TABLE
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

    mov rsp, stack_top_64
    and rsp, 0xFFFFFFFFFFFFFFF0

    extern hal_init
    call hal_init

    extern kernel_main
    call kernel_main

    hlt

section .bss
global cpu_vendor
cpu_vendor:
    resb 12

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