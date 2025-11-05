; context_switch.asm

section .text
    global switch_context

; switch_context(old_rsp, new_rsp)
; Saves the current context to old_rsp and loads the new context from new_rsp
switch_context:
    ; Save current context
    mov [rdi], rsp      ; Save current RSP to old_rsp
    push rax            ; Save RAX
    push rbx            ; Save RBX
    push rcx            ; Save RCX
    push rdx            ; Save RDX
    push rbp            ; Save RBP
    push rsi            ; Save RSI
    push rdi            ; Save RDI
    push r8             ; Save R8
    push r9             ; Save R9
    push r10            ; Save R10
    push r11            ; Save R11
    push r12            ; Save R12
    push r13            ; Save R13
    push r14            ; Save R14
    push r15            ; Save R15

    ; Load new context
    mov rsp, rsi        ; Load new RSP from new_rsp
    pop r15             ; Restore R15
    pop r14             ; Restore R14
    pop r13             ; Restore R13
    pop r12             ; Restore R12
    pop r11             ; Restore R11
    pop r10             ; Restore R10
    pop r9              ; Restore R9
    pop r8              ; Restore R8
    pop rdi             ; Restore RDI
    pop rsi             ; Restore RSI
    pop rbp             ; Restore RBP
    pop rdx             ; Restore RDX
    pop rcx             ; Restore RCX
    pop rbx             ; Restore RBX
    pop rax             ; Restore RAX

    ret