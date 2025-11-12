; context_switch.asm

section .text
    global switch_context
    global idt_load

; switch_context(old_rsp, new_rsp)
; Saves the current context to old_rsp and loads the new context from new_rsp
switch_context:
    ; Save current context
    push r15            ; Save R15
    push r14            ; Save R14
    push r13            ; Save R13
    push r12            ; Save R12
    push r11            ; Save R11
    push r10            ; Save R10
    push r9             ; Save R9
    push r8             ; Save R8
    push rdi            ; Save RDI
    push rsi            ; Save RSI
    push rbp            ; Save RBP
    push rdx            ; Save RDX
    push rcx            ; Save RCX
    push rbx            ; Save RBX
    push rax            ; Save RAX

    mov [rdi], rsp      ; Save current RSP to old_rsp

    ; Load new context
    mov rsp, rsi        ; Load new RSP from new_rsp
    pop rax             ; Restore RAX
    pop rbx             ; Restore RBX
    pop rcx             ; Restore RCX
    pop rdx             ; Restore RDX
    pop rbp             ; Restore RBP
    pop rsi             ; Restore RSI
    pop rdi             ; Restore RDI
    pop r8              ; Restore R8
    pop r9              ; Restore R9
    pop r10             ; Restore R10
    pop r11             ; Restore R11
    pop r12             ; Restore R12
    pop r13             ; Restore R13
    pop r14             ; Restore R14
    pop r15             ; Restore R15

    ret

; idt_load(idt_ptr)
; Loads the IDT pointer into the IDTR register
idt_load:
    lidt [rdi]          ; Load IDT pointer from first argument
    ret