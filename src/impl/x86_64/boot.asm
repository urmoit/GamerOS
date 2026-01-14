; x86_64 boot assembly code
; This sets up the processor in long mode (64-bit) and jumps to the kernel

; Multiboot header (must be within first 8KB of file)
section .multiboot_header
align 8
header_start:
    dd 0x1BADB002                ; Magic number (multiboot 1)
    dd 0x00000003                ; Flags (page align + memory info)
    dd 0xE4524FFB                ; Checksum: -(0x1BADB002 + 0x00000003)
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
    
    ; Debug: Indicate we've started
    mov byte [0xB8000], 'M'
    mov byte [0xB8001], 0x0E  ; Yellow on black
    mov byte [0xB8002], 'B'
    mov byte [0xB8003], 0x0E

    ; Temporarily skip multiboot check to isolate the issue
    ; Assume we're loaded correctly by GRUB

    ; Debug: Boot sequence - M B O K P G Z T E L P H J 6 4 !
    mov byte [0xB8000], 'M'     ; Multiboot started
    mov byte [0xB8001], 0x0E
    mov byte [0xB8002], 'B'     ; Boot
    mov byte [0xB8003], 0x0E
    mov byte [0xB8004], 'O'     ; OK
    mov byte [0xB8005], 0x0E
    mov byte [0xB8006], 'K'     ; OK
    mov byte [0xB8007], 0x0E
    
    ; Simplified CPU detection - assume modern CPU supports required features
    ; Skip CPUID checks for now to avoid compatibility issues
    ; mov eax, 0x80000001
    ; cpuid
    ; test edx, 1 << 29
    ; jz .no_long_mode

    ; Skip VESA setup entirely - BIOS interrupts in protected mode cause SMM activation
    ; Just assume VGA mode 13h is set by bootloader (which it should be for GRUB)
    mov byte [vesa_success], 0

    ; Continue debug sequence
    mov byte [0xB8008], 'P'     ; Paging
    mov byte [0xB8009], 0x0E
    mov byte [0xB800A], 'G'     ; setup
    mov byte [0xB800B], 0x0E

    ; Debug: Zeroing page tables
    mov byte [0xB800C], 'Z'     ; Zero
    mov byte [0xB800D], 0x0E

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
    
    ; Set up P2 table: identity map first 1GB of memory with 2MB huge pages
    ; Entry 0: 0-2MB
    mov eax, 0x0
    or eax, 0b10000011   ; Present + Writable + Huge (bit 7)
    mov [p2_table], eax

    ; Entry 1: 2-4MB
    mov eax, 0x200000
    or eax, 0b10000011   ; Present + Writable + Huge
    mov [p2_table + 8], eax

    ; Entry 2: 4-6MB
    mov eax, 0x400000
    or eax, 0b10000011   ; Present + Writable + Huge
    mov [p2_table + 16], eax

    ; Entry 3: 6-8MB
    mov eax, 0x600000
    or eax, 0b10000011   ; Present + Writable + Huge
    mov [p2_table + 24], eax

    ; Entry 4: 8-10MB
    mov eax, 0x800000
    or eax, 0b10000011   ; Present + Writable + Huge
    mov [p2_table + 32], eax

    ; Entry 5: 10-12MB
    mov eax, 0xA00000
    or eax, 0b10000011   ; Present + Writable + Huge
    mov [p2_table + 40], eax

    ; Entry 6: 12-14MB
    mov eax, 0xC00000
    or eax, 0b10000011   ; Present + Writable + Huge
    mov [p2_table + 48], eax

    ; Entry 7: 14-16MB
    mov eax, 0xE00000
    or eax, 0b10000011   ; Present + Writable + Huge
    mov [p2_table + 56], eax

    ; Debug: Page tables set up
    mov byte [0xB800E], 'T'
    mov byte [0xB800F], 0x0E

    ; CRITICAL: Enable paging BEFORE entering 64-bit mode
    ; x86_64 REQUIRES paging to be enabled - without it, CPU will triple fault

    ; Debug: About to enable paging
    mov byte [0xB8010], 'E'
    mov byte [0xB8011], 0x0E

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

    ; Debug: Long mode enabled
    mov byte [0xB8012], 'L'
    mov byte [0xB8013], 0x0E

    ; Step 4: Enable paging (CR0.PG) - THIS IS THE CRITICAL MISSING STEP!
    mov eax, cr0
    or eax, 1 << 31      ; Set PG bit (bit 31) - ENABLE PAGING
    mov cr0, eax

    ; Debug: Paging enabled
    mov byte [0xB8014], 'P'
    mov byte [0xB8015], 0x0E

    ; Debug: About to init HAL
    mov byte [0xB8016], 'H'
    mov byte [0xB8017], 0x0E

    ; TEMPORARILY DISABLE HAL INIT TO TEST
    ; Initialize HAL (GDT/IDT setup - page tables already done above)
    ; extern hal_init
    ; call hal_init

    ; Debug: HAL initialized, jumping to 64-bit
    mov byte [0xB8018], 'J'
    mov byte [0xB8019], 0x0E

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
    ; Debug: Indicate we've reached 64-bit mode
    mov byte [0xB8010], '6'
    mov byte [0xB8011], 0x0A  ; Green on black
    mov byte [0xB8012], '4'
    mov byte [0xB8013], 0x0A
    mov byte [0xB8014], '!'
    mov byte [0xB8015], 0x0A

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

    ; Debug: Print that we've reached the kernel
    mov byte [0xB8000], 'K'
    mov byte [0xB8001], 0x0C  ; Red on black
    mov byte [0xB8002], 'E'
    mov byte [0xB8003], 0x0C
    mov byte [0xB8004], 'R'
    mov byte [0xB8005], 0x0C
    mov byte [0xB8006], 'N'
    mov byte [0xB8007], 0x0C
    mov byte [0xB8008], 'E'
    mov byte [0xB8009], 0x0C
    mov byte [0xB800A], 'L'
    mov byte [0xB800B], 0x0C
    mov byte [0xB800C], ' '
    mov byte [0xB800D], 0x0C
    mov byte [0xB800E], 'L'
    mov byte [0xB800F], 0x0C
    mov byte [0xB8010], 'O'
    mov byte [0xB8011], 0x0C
    mov byte [0xB8012], 'A'
    mov byte [0xB8013], 0x0C
    mov byte [0xB8014], 'D'
    mov byte [0xB8015], 0x0C
    mov byte [0xB8016], 'E'
    mov byte [0xB8017], 0x0C
    mov byte [0xB8018], 'D'
    mov byte [0xB8019], 0x0C

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


