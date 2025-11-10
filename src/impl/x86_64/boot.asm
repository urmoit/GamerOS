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

    ; Try to set up VESA graphics mode before entering long mode
    call try_vesa_mode

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
    ; Identity map first 2MB
    mov eax, p3_table
    or eax, 0b11 ; Present + Writable
    mov [p4_table], eax
    
    mov eax, p2_table
    or eax, 0b11
    mov [p3_table], eax
    
    ; Map each P2 entry to a 2MB page
    mov ecx, 0
.map_p2_table:
    mov eax, 0x200000
    mul ecx
    or eax, 0b10000011 ; Present + Writable + Huge
    mov [p2_table + ecx * 8], eax
    
    inc ecx
    cmp ecx, 512
    jne .map_p2_table
    
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

try_vesa_mode:
    ; Try to set VESA mode 101h (640x480x256) in real mode
    ; This will set up the linear framebuffer before entering long mode

    ; For now, just mark as failed - real VESA setup needs more work
    mov byte [vesa_success], 0
    ret

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

    ; Clear screen to white for debugging (indicates we reached 64-bit mode)
    mov rdi, 0xB8000
    mov rcx, 2000  ; Clear 80x25 characters
    mov rax, 0x0F200F200F200F20  ; White spaces on black background
    rep stosq

    ; Print "64-bit mode reached" message
    mov dword [0xB8000], 0x0F360F34  ; "64"
    mov dword [0xB8004], 0x0F2D0F62  ; "-b"
    mov dword [0xB8008], 0x0F740F69  ; "it"
    mov dword [0xB800C], 0x0F200F20  ; "  "
    mov dword [0xB8010], 0x0F6F0F6D  ; "mo"
    mov dword [0xB8014], 0x0F650F64  ; "de"
    mov dword [0xB8018], 0x0F200F20  ; "  "
    mov dword [0xB801C], 0x0F630F72  ; "re"
    mov dword [0xB8020], 0x0F630F61  ; "ac"
    mov dword [0xB8024], 0x0F650F68  ; "he"
    mov dword [0xB8028], 0x0F210F64  ; "d!"

    ; Check VESA success and print graphics status
    cmp byte [vesa_success], 1
    jne .no_vesa
    ; Print "VESA OK" message
    mov dword [0xB8030], 0x0F450F56  ; "VE"
    mov dword [0xB8034], 0x0F410F53  ; "SA"
    mov dword [0xB8038], 0x0F200F20  ; "  "
    mov dword [0xB803C], 0x0F4B0F4F  ; "OK"
    jmp .vesa_done
.no_vesa:
    ; Print "VESA FAIL" message
    mov dword [0xB8030], 0x0F450F56  ; "VE"
    mov dword [0xB8034], 0x0F410F53  ; "SA"
    mov dword [0xB8038], 0x0F200F20  ; "  "
    mov dword [0xB803C], 0x0F4C0F46  ; "FA"
    mov dword [0xB8040], 0x0F4C0F49  ; "IL"
.vesa_done:

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
    resb 1

stack_bottom:
    resb 16384
stack_top:
stack_top_64 equ stack_top

section .rodata
gdt_64:
    dq 0 ; Null descriptor
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; Code segment
    dq (1 << 44) | (1 << 47) ; Data segment

gdt_descriptor:
    dw $ - gdt_64 - 1
    dq gdt_64

