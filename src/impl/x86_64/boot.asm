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

%include "src/impl/graphics/font.inc"

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

    ; CRITICAL: Set up paging tables DIRECTLY in assembly before calling hal_init
    ; We can't rely on C code for this because we need paging enabled before 64-bit mode
    
    ; Set up page tables in BSS section (they're already allocated)
    ; P4 table at p4_table, P3 at p3_table, P2 at p2_table
    extern p4_table
    extern p3_table
    extern p2_table
    
    ; Zero out page tables first (safety)
    mov edi, p4_table
    mov ecx, 512 * 3     ; Clear P4, P3, P2 tables (512 entries each)
    xor eax, eax
    rep stosd
    
    ; Set up P4 table: entry 0 points to P3 table
    mov eax, p3_table
    or eax, 0b11         ; Present + Writable
    mov [p4_table], eax
    
    ; Set up P3 table: entry 0 points to P2 table
    mov eax, p2_table
    or eax, 0b11         ; Present + Writable
    mov [p3_table], eax
    
    ; Set up P2 table: entry 0 maps 0-2MB (2MB huge page)
    mov eax, 0x0
    or eax, 0b10000011   ; Present + Writable + Huge (bit 7)
    mov [p2_table], eax
    
    ; Set up P2 table: entry 1 maps 2-4MB (2MB huge page)
    mov eax, 0x200000
    or eax, 0b10000011   ; Present + Writable + Huge
    mov [p2_table + 8], eax
    
    ; CRITICAL: Enable paging BEFORE entering 64-bit mode
    ; x86_64 REQUIRES paging to be enabled - without it, CPU will triple fault
    
    ; Step 1: Enable PAE (Physical Address Extension) - required for 64-bit paging
    mov eax, cr4
    or eax, 1 << 5       ; Set PAE bit (bit 5)
    mov cr4, eax
    
    ; Step 2: Load CR3 with P4 table address
    mov eax, p4_table
    mov cr3, eax
    
    ; Step 3: Enable long mode (EFER.LME)
    mov ecx, 0xC0000080  ; EFER MSR
    rdmsr
    or eax, 1 << 8       ; Set LME bit (bit 8) - Enable Long Mode
    wrmsr
    
    ; Step 4: Enable paging (CR0.PG) - THIS IS THE CRITICAL MISSING STEP!
    mov eax, cr0
    or eax, 1 << 31      ; Set PG bit (bit 31) - ENABLE PAGING
    mov cr0, eax

    ; Initialize HAL (GDT/IDT setup - page tables already done above)
    extern hal_init
    call hal_init

    ; Step 5: Now we can safely jump to 64-bit code segment
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


; try_vesa_mode function removed - BIOS interrupts cause SMM activation in QEMU

bits 64
start_64:
    ; Update segment registers for 64-bit mode
    ; In 64-bit mode, segment registers are mostly ignored (except FS/GS for TLS)
    ; Setting to 0 is fine, but 0x10 (data segment) is also valid
    mov ax, 0x10        ; Data segment selector
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Set up stack (must be 16-byte aligned for x86_64 ABI)
    mov rsp, stack_top_64
    ; Ensure 16-byte alignment (required for SSE/AVX and C function calls)
    and rsp, 0xFFFFFFFFFFFFFFF0

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


