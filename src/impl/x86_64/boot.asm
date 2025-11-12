; x86_64 boot assembly code
; This sets up the processor in long mode (64-bit) and jumps to the kernel

section .multiboot_header
align 4
header_start:
    dd 0xE85250D6                ; Magic number (multiboot 2)
    dd 0                         ; Architecture (i386)
    dd header_end - header_start ; Header length
    dd 0x100000000 - (0xE85250D6 + 0 + (header_end - header_start)) ; Checksum
    
    ; End tag
    dw 0    ; Type
    dw 0    ; Flags
    dd 8    ; Size
header_end:

section .text
bits 32

%include "src/impl/x86_64/font.inc"

; Define constants for assembly
VGA_GRAPHICS_BUFFER equ 0xA0000

global _start
_start:
    ; Set up stack
    mov esp, stack_top
    
    ; Check if multiboot is supported (magic number in EAX)
    cmp eax, 0x36D76289  ; Multiboot 1/2 magic number
    jne .no_multiboot
    
    ; Check CPUID
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je .no_cpuid
    
    ; Check for long mode
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode

    ; Skip VESA setup entirely - BIOS interrupts in protected mode cause SMM activation
    ; Just assume VGA mode 13h is set by bootloader (which it should be for GRUB)
    mov byte [vesa_success], 0

    ; Set up paging
    call setup_page_tables
    call enable_paging
    
    ; Load GDT
    lgdt [gdt_descriptor]
    
    ; Jump to 64-bit code segment
    jmp 0x08:start_64
    
.no_multiboot:
    mov al, "M"
    jmp error
.no_cpuid:
    mov al, "C"
    jmp error
.no_long_mode:
    mov al, "L"
    jmp error

error:
    ; Simple error display (prints character code)
    mov dword [0xB8000], 0x4F524F45
    mov dword [0xB8004], 0x4F3A4F52
    mov dword [0xB8008], 0x4F204F20
    mov byte  [0xB800A], al
    hlt

setup_page_tables:
    ; Identity map first 4MB (2 pages) to ensure kernel has enough mapped memory
    mov eax, p3_table
    or eax, 0b11 ; Present + Writable
    mov [p4_table], eax

    mov eax, p2_table
    or eax, 0b11
    mov [p3_table], eax

    ; Map first P2 entry (0-2MB)
    mov eax, 0x0
    or eax, 0b10000011 ; Present + Writable + Huge
    mov [p2_table + 0 * 8], eax

    ; Map second P2 entry (2-4MB)
    mov eax, 0x200000
    or eax, 0b10000011 ; Present + Writable + Huge
    mov [p2_table + 1 * 8], eax

    ; Map VGA memory region (0xA0000 - 0xBFFFF) for graphics
    ; VGA memory is at physical address 0xA0000, we want to map it to virtual address 0xA0000
    ; This requires setting up the page tables correctly for the VGA region

    ; For identity mapping of VGA memory, we need to map virtual 0xA0000 to physical 0xA0000
    ; Since we're using 2MB huge pages, we need to find which P2 entry corresponds to 0xA0000

    ; Virtual address 0xA0000 breaks down as:
    ; P4 index = 0 (bits 39-47)
    ; P3 index = 0 (bits 30-38)
    ; P2 index = 5 (bits 21-29: 0xA0000 >> 21 = 5)
    ; Page offset = 0 (bits 0-20)

    ; So we need to set p2_table[5] to point to physical 0xA0000 with huge page flags
    mov eax, VGA_GRAPHICS_BUFFER
    or eax, 0b10000011 ; Present + Writable + Huge
    mov [p2_table + 5 * 8], eax

    ret

enable_paging:
    ; Load P4 to cr3
    mov eax, p4_table
    mov cr3, eax
    
    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
    
    ; Enable long mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr
    
    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    
    ret

; try_vesa_mode function removed - BIOS interrupts cause SMM activation in QEMU

bits 64
start_64:
    ; Update segment registers
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Set up stack
    mov rsp, stack_top_64

    ; Simple text output to indicate we've reached 64-bit mode
    mov byte [0xB8000], '6'
    mov byte [0xB8001], 0x0A  ; Green on black
    mov byte [0xB8002], '4'
    mov byte [0xB8003], 0x0A
    mov byte [0xB8004], '-'
    mov byte [0xB8005], 0x0A
    mov byte [0xB8006], 'b'
    mov byte [0xB8007], 0x0A
    mov byte [0xB8008], 'i'
    mov byte [0xB8009], 0x0A
    mov byte [0xB800A], 't'
    mov byte [0xB800B], 0x0A
    mov byte [0xB800C], ' '
    mov byte [0xB800D], 0x0A
    mov byte [0xB800E], 'm'
    mov byte [0xB800F], 0x0A
    mov byte [0xB8010], 'o'
    mov byte [0xB8011], 0x0A
    mov byte [0xB8012], 'd'
    mov byte [0xB8013], 0x0A
    mov byte [0xB8014], 'e'
    mov byte [0xB8015], 0x0A
    mov byte [0xB8016], ' '
    mov byte [0xB8017], 0x0A
    mov byte [0xB8018], 'O'
    mov byte [0xB8019], 0x0A
    mov byte [0xB801A], 'K'
    mov byte [0xB801B], 0x0A

    ; Call kernel main
    extern kernel_main
    call kernel_main

    ; Should never return, but just in case
    hlt

section .bss
align 4096
p4_table:
    resb 4096
p3_table:
    resb 4096
p2_table:
    resb 4096

; VESA information storage
global vesa_info
global vesa_mode_info
global vesa_success

vesa_info:
    resb 512
vesa_mode_info:
    resb 256
vesa_success:
    db 0

stack_bottom:
    resb 16384
stack_top:
stack_top_64 equ stack_top

; Graphics text drawing function
; Input: AL = character, RBX = x, RCX = y
draw_char_graphics:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi

    ; Check if character is printable ASCII
    cmp al, 32
    jb .done
    cmp al, 127
    ja .done

    ; Calculate font data offset: (char - 32) * 8
    sub al, 32
    movzx rax, al
    shl rax, 3  ; Multiply by 8
    add rax, font_8x8

    ; RSI points to character bitmap
    mov rsi, rax

    ; For each row (8 rows)
    mov rdx, 8
.draw_row:
    lodsb  ; Load row data into AL
    push rdx
    push rcx

    ; For each column (8 columns)
    mov rdx, 8
    mov rdi, rbx  ; Start x position for this row
.draw_pixel:
    test al, 10000000b  ; Test leftmost bit
    jz .skip_pixel

    ; Draw pixel at (rdi, rcx) - assuming white on current background
    ; For VGA mode 13h: y * 320 + x
    push rax
    push rbx
    push rdx

    mov rax, rcx
    mov rbx, 320    ; VGA mode 13h width
    mul rbx
    add rax, rdi
    add rax, VGA_GRAPHICS_BUFFER  ; Framebuffer base

    mov byte [rax], 0x00  ; Black text (for now)

    pop rdx
    pop rbx
    pop rax

.skip_pixel:
    shl al, 1  ; Shift left to check next bit
    inc rdi    ; Next x position
    dec rdx
    jnz .draw_pixel

    pop rcx
    pop rdx
    inc rcx    ; Next row
    dec rdx
    jnz .draw_row

.done:
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    ret

section .data
graphics_message:
    db "64-bit mode reached!", 0
custom_font_message:
    db "Custom font loaded successfully!", 0
vesa_status_message:
    db "Graphics mode initialized", 0

gdt_64:
    dq 0 ; Null descriptor
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; Code segment
    dq (1 << 44) | (1 << 47) ; Data segment

gdt_descriptor:
    dw $ - gdt_64 - 1
    dq gdt_64

