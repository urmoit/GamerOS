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
    
    ; Jump over helper functions to main 32-bit boot logic
    jmp boot_main32

    ; Debug: Indicate we've started
    mov byte [0xB8000], 'M'
    mov byte [0xB8001], 0x0E  ; Yellow on black
    mov byte [0xB8002], 'B'
    mov byte [0xB8003], 0x0E

    ; Temporarily skip multiboot check to isolate the issue
    ; Assume we're loaded correctly by GRUB

; CPU Detection Function
; Returns: CF=1 if CPU unsupported, CF=0 if OK
detect_cpu:
    ; Debug: CPU detection started
    mov byte [0xB8000], 'D'
    mov byte [0xB8001], 0x0E
    ; Simplified: Assume CPUID is available on modern systems
    ; Skip the CPUID availability test for now

    ; CPUID supported, get vendor string
    xor eax, eax
    cpuid
    mov [cpu_vendor], ebx
    mov [cpu_vendor+4], edx
    mov [cpu_vendor+8], ecx

    ; Debug: Testing extended CPUID
    mov byte [0xB8002], 'E'
    mov byte [0xB8003], 0x0E

    ; Check for extended CPUID functions
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_ext_cpuid

    ; Debug: Extended CPUID OK
    mov byte [0xB8004], 'X'
    mov byte [0xB8005], 0x0E

    ; Check for long mode support
    mov eax, 0x80000001
    cpuid
    test edx, (1 << 29)     ; Long Mode bit
    jz .no_long_mode

    ; Debug: Long mode OK
    mov byte [0xB8006], 'L'
    mov byte [0xB8007], 0x0E

    ; Check for basic CPUID functions
    xor eax, eax
    cpuid
    test eax, eax           ; Function 1 supported?
    jz .no_basic_features

    ; Check for required features in function 1
    mov eax, 1
    cpuid
    test edx, (1 << 0)      ; FPU
    jz .no_fpu
    test edx, (1 << 6)      ; PAE
    jz .no_pae

    ; Debug: CPU features OK
    mov byte [0xB8008], 'F'
    mov byte [0xB8009], 0x0E

    ; CPU is supported!
    ; Debug: CPU detection successful
    mov byte [0xB800A], 'O'
    mov byte [0xB800B], 0x0E
    mov byte [0xB800C], 'K'
    mov byte [0xB800D], 0x0E

    clc                     ; Clear carry flag (success)
    ret

.no_cpuid:
    mov byte [0xB8000], 'N' ; No CPUID
    mov byte [0xB8001], 0x0C
    mov byte [0xB8002], 'C'
    mov byte [0xB8003], 0x0C
    stc
    ret

.no_ext_cpuid:
    mov byte [0xB8000], 'N' ; No extended CPUID
    mov byte [0xB8001], 0x0C
    mov byte [0xB8002], 'E'
    mov byte [0xB8003], 0x0C
    stc
    ret

.no_long_mode:
    mov byte [0xB8000], 'N' ; No long mode
    mov byte [0xB8001], 0x0C
    mov byte [0xB8002], 'L'
    mov byte [0xB8003], 0x0C
    stc
    ret

.no_basic_features:
    mov byte [0xB8000], 'N' ; No basic features
    mov byte [0xB8001], 0x0C
    mov byte [0xB8002], 'B'
    mov byte [0xB8003], 0x0C
    stc
    ret

.no_fpu:
    mov byte [0xB8000], 'N' ; No FPU
    mov byte [0xB8001], 0x0C
    mov byte [0xB8002], 'F'
    mov byte [0xB8003], 0x0C
    stc
    ret

.no_pae:
    mov byte [0xB8000], 'N' ; No PAE
    mov byte [0xB8001], 0x0C
    mov byte [0xB8002], 'P'
    mov byte [0xB8003], 0x0C
    stc
    ret

; Set VGA mode 13h (320x200x256) via direct register writes
; Proper VGA mode 13h setup requires configuring multiple register groups
set_vga_mode13h:
    ; Save registers
    push eax
    push edx
    push edi
    push ecx
    
    ; Unlock CRTC registers (required before modifying CRTC)
    mov dx, 0x3D4
    mov al, 0x11
    out dx, al
    mov dx, 0x3D5
    in al, dx
    and al, 0x7F  ; Clear bit 7 to unlock
    out dx, al
    
    ; Sequencer registers - configure for mode 13h
    mov dx, 0x3C4
    
    ; Sequencer: Reset Register
    mov al, 0x00
    out dx, al
    mov dx, 0x3C5
    mov al, 0x03  ; Synchronous reset
    out dx, al
    
    ; Sequencer: Clocking Mode Register
    mov dx, 0x3C4
    mov al, 0x01
    out dx, al
    mov dx, 0x3C5
    mov al, 0x01  ; 8-dot character clock
    out dx, al
    
    ; Sequencer: Map Mask Register - enable all planes
    mov dx, 0x3C4
    mov al, 0x02
    out dx, al
    mov dx, 0x3C5
    mov al, 0x0F  ; Enable all 4 planes
    out dx, al
    
    ; Sequencer: Character Map Select Register
    mov dx, 0x3C4
    mov al, 0x03
    out dx, al
    mov dx, 0x3C5
    mov al, 0x00
    out dx, al
    
    ; Sequencer: Memory Mode Register - CRITICAL for mode 13h
    mov dx, 0x3C4
    mov al, 0x04
    out dx, al
    mov dx, 0x3C5
    mov al, 0x06  ; Chain-4 mode (bit 2), odd/even disabled (bit 1)
    out dx, al
    
    ; Sequencer: Reset Register - end reset
    mov dx, 0x3C4
    mov al, 0x00
    out dx, al
    mov dx, 0x3C5
    mov al, 0x03  ; End reset
    out dx, al
    
    ; Graphics Controller registers
    mov dx, 0x3CE
    
    ; Graphics Controller: Set/Reset Register
    mov al, 0x00
    out dx, al
    mov dx, 0x3CF
    mov al, 0x00
    out dx, al
    
    ; Graphics Controller: Enable Set/Reset Register
    mov dx, 0x3CE
    mov al, 0x01
    out dx, al
    mov dx, 0x3CF
    mov al, 0x00
    out dx, al
    
    ; Graphics Controller: Color Compare Register
    mov dx, 0x3CE
    mov al, 0x02
    out dx, al
    mov dx, 0x3CF
    mov al, 0x00
    out dx, al
    
    ; Graphics Controller: Data Rotate Register
    mov dx, 0x3CE
    mov al, 0x03
    out dx, al
    mov dx, 0x3CF
    mov al, 0x00
    out dx, al
    
    ; Graphics Controller: Read Map Select Register
    mov dx, 0x3CE
    mov al, 0x04
    out dx, al
    mov dx, 0x3CF
    mov al, 0x00
    out dx, al
    
    ; Graphics Controller: Mode Register - CRITICAL for mode 13h
    mov dx, 0x3CE
    mov al, 0x05
    out dx, al
    mov dx, 0x3CF
    mov al, 0x40  ; 256 color mode (bit 6), write mode 0
    out dx, al
    
    ; Graphics Controller: Miscellaneous Register - CRITICAL for mode 13h
    mov dx, 0x3CE
    mov al, 0x06
    out dx, al
    mov dx, 0x3CF
    mov al, 0x05  ; Chain-4 (bit 2), graphics mode (bit 0)
    out dx, al
    
    ; Graphics Controller: Color Don't Care Register
    mov dx, 0x3CE
    mov al, 0x07
    out dx, al
    mov dx, 0x3CF
    mov al, 0x0F  ; Don't care about all planes
    out dx, al
    
    ; Graphics Controller: Bit Mask Register
    mov dx, 0x3CE
    mov al, 0x08
    out dx, al
    mov dx, 0x3CF
    mov al, 0xFF  ; All bits
    out dx, al
    
    ; Initialize VGA palette with standard colors
    ; VGA palette is at ports 0x3C8 (write index) and 0x3C9 (write data)
    ; Each color has 3 bytes: Red, Green, Blue (each 0-63, but we use 0-255 and shift right by 2)
    
    ; Set palette index 0x01 (Blue)
    mov dx, 0x3C8
    mov al, 0x01
    out dx, al
    mov dx, 0x3C9
    mov al, 0x00  ; R
    out dx, al
    mov al, 0x00  ; G
    out dx, al
    mov al, 0x3F  ; B (63 = max blue)
    out dx, al
    
    ; Set palette index 0x02 (Green)
    mov dx, 0x3C8
    mov al, 0x02
    out dx, al
    mov dx, 0x3C9
    mov al, 0x00  ; R
    out dx, al
    mov al, 0x3F  ; G (63 = max green)
    out dx, al
    mov al, 0x00  ; B
    out dx, al
    
    ; Set palette index 0x04 (Red)
    mov dx, 0x3C8
    mov al, 0x04
    out dx, al
    mov dx, 0x3C9
    mov al, 0x3F  ; R (63 = max red)
    out dx, al
    mov al, 0x00  ; G
    out dx, al
    mov al, 0x00  ; B
    out dx, al
    
    ; Set palette index 0x0F (White)
    mov dx, 0x3C8
    mov al, 0x0F
    out dx, al
    mov dx, 0x3C9
    mov al, 0x3F  ; R (63 = max red)
    out dx, al
    mov al, 0x3F  ; G (63 = max green)
    out dx, al
    mov al, 0x3F  ; B (63 = max blue)
    out dx, al
    
    ; Fill framebuffer with colored bars (so we can see if graphics mode is working)
    mov edi, 0xA0000
    
    ; White bar (top 50 rows = 16000 bytes)
    mov ecx, 320 * 50
    mov eax, 0x0F  ; White (palette 15)
    rep stosb
    
    ; Red bar (next 50 rows)
    mov ecx, 320 * 50
    mov eax, 0x04  ; Red (palette 4)
    rep stosb
    
    ; Green bar (next 50 rows)
    mov ecx, 320 * 50
    mov eax, 0x02  ; Green (palette 2)
    rep stosb
    
    ; Blue bar (bottom 50 rows)
    mov ecx, 320 * 50
    mov eax, 0x01  ; Blue (palette 1)
    rep stosb
    
    ; Restore registers
    pop ecx
    pop edi
    pop edx
    pop eax
    ret

    ; Debug: Boot sequence - C M B O K P G Z T E L P H J 6 4 !
    ; CPU detection already wrote status to 0xB8000-0xB8003
    mov byte [0xB8004], 'M'     ; Multiboot started
    mov byte [0xB8005], 0x0E

boot_main32:
    ; Main 32-bit boot code starts here
    ; (Debug characters at 0xB8004-0xB800B already written above)

    ; Explicitly DISABLE Non-Maskable Interrupts (NMI) via CMOS port 0x70.
    ; Bit 7 controls NMI (1 = disabled, 0 = enabled). Ensure it is set to 1
    ; to avoid spurious NMIs during early boot.
    in  al, 0x70
    or  al, 0x80
    out 0x70, al

    mov byte [0xB8006], 'B'     ; Boot
    mov byte [0xB8007], 0x0E
    mov byte [0xB8008], 'O'     ; OK
    mov byte [0xB8009], 0x0E
    mov byte [0xB800A], 'K'     ; OK
    mov byte [0xB800B], 0x0E
    
    ; Comprehensive CPU detection for x86/x64 and Intel/AMD CPUs
    call detect_cpu

    ; Debug: After CPU detection call
    mov byte [0xB800E], 'A'
    mov byte [0xB800F], 0x0E

    jc .cpu_unsupported

    ; Debug: CPU detection passed, continuing
    mov byte [0xB8010], 'C'
    mov byte [0xB8011], 0x0E

    ; Set VGA mode 13h via direct register writes (no BIOS needed)
    ; This sets 320x200x256 color mode
    call set_vga_mode13h
    
    ; Continue debug sequence
    mov byte [0xB8008], 'P'     ; Paging
    mov byte [0xB8009], 0x0E
    mov byte [0xB800A], 'G'     ; setup
    mov byte [0xB800B], 0x0E

    ; Debug: Zeroing page tables
    mov byte [0xB800C], 'Z'     ; Zero
    mov byte [0xB800D], 0x0E

    ; CRITICAL: Set up paging tables in FIXED memory locations
    ; Use safe addresses that are definitely accessible in 32-bit mode

    ; Fixed addresses for page tables (right after kernel at 2MB)
    %define P4_TABLE 0x200000   ; 2MB
    %define P3_TABLE 0x201000   ; 2MB + 4KB
    %define P2_TABLE 0x202000   ; 2MB + 8KB
    
    ; Zero out page tables first (safety)
    mov edi, P4_TABLE
    mov ecx, 512 * 3     ; Clear P4, P3, P2 tables (512 entries each)
    xor eax, eax
    rep stosd
    
    ; Set up P4 table: entry 0 points to P3 table
    mov eax, P3_TABLE
    or eax, 0b11         ; Present + Writable
    mov [P4_TABLE], eax
    
    ; Set up P3 table: entry 0 points to P2 table
    mov eax, P2_TABLE
    or eax, 0b11         ; Present + Writable
    mov [P3_TABLE], eax
    
    ; Set up P2 table: identity map first 2MB only (simplified)
    ; Entry 0: 0-2MB
    mov eax, 0x0
    or eax, 0b10000011   ; Present + Writable + Huge (bit 7)
    mov [P2_TABLE], eax
    
    ; Debug: Page tables set up
    mov byte [0xB800E], 'T'
    mov byte [0xB800F], 0x0E
    
    ; ------------------------------------------------------------------
    ; Set up a temporary 64-bit compatible GDT for the long mode jump
    ; ------------------------------------------------------------------
    ; This GDT provides:
    ;  - 0x00: null descriptor
    ;  - 0x08: 64-bit kernel code segment
    ;  - 0x10: 64-bit kernel data segment
    lgdt [gdt64_descriptor]

    ; ------------------------------------------------------------------
    ; Enable paging and long mode BEFORE entering 64-bit mode
    ; x86_64 REQUIRES paging + long mode to be enabled - without it,
    ; the CPU will triple fault on the 64-bit jump.
    ; ------------------------------------------------------------------

    ; Debug: About to enable paging
    mov byte [0xB8010], 'E'
    mov byte [0xB8011], 0x0E
    
    ; Step 1: Enable PAE (Physical Address Extension) - required for 64-bit paging
    mov eax, cr4
    or eax, 1 << 5       ; Set PAE bit (bit 5)
    mov cr4, eax
    
    ; Step 2: Load CR3 with P4 table address
    mov eax, P4_TABLE
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

    ; Step 5: Now we can safely jump to 64-bit code segment
    jmp 0x08:start_64
    
.cpu_unsupported:
    ; CPU not supported - halt
    hlt
    
.no_multiboot:
    mov al, "M"
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

    ; Initialize HAL (GDT/IDT setup) now that we are safely in 64-bit mode
    extern hal_init

    ; Debug: About to init HAL
    mov byte [0xB8016], 'H'
    mov byte [0xB8017], 0x0E

    call hal_init

    ; Debug: HAL initialized, kernel entry next
    mov byte [0xB8018], 'J'
    mov byte [0xB8019], 0x0E

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

; CPU information
global cpu_vendor
cpu_vendor:
    resb 12         ; Space for 12-byte vendor string

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
; ----------------------------------------------------------------------
; Temporary 64-bit compatible GDT for the initial long mode transition
; ----------------------------------------------------------------------
gdt64:
    dq 0x0000000000000000         ; Null descriptor
    dq 0x00AF9A000000FFFF         ; 0x08: 64-bit kernel code segment
    dq 0x00AF92000000FFFF         ; 0x10: 64-bit kernel data segment

gdt64_descriptor:
    dw gdt64_descriptor_end - gdt64 - 1
    dd gdt64                     ; Base (fits in 32 bits; below 4GB)
gdt64_descriptor_end:

graphics_message:
    db "64-bit mode reached!", 0
custom_font_message:
    db "Custom font loaded successfully!", 0
vesa_status_message:
    db "Graphics mode initialized", 0


