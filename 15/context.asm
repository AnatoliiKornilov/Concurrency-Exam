global SwitchContext
global SetupContext

section .text

SwitchContext:
  ; rdi = &from_rsp
  ; rsi = &to_rsp

  push rbx
  push rbp
  push r12
  push r13
  push r14
  push r15

  mov [rdi], rsp
  mov rsp, [rsi]

  pop r15
  pop r14
  pop r13
  pop r12
  pop rbp
  pop rbx

  ret

SetupContext:
  ; rdi = &rsp
  ; rsi = &object
  ; rdx = &trampline

  mov rax, rdi

  sub rax, 8
  mov qword [rax], rsi

  sub rax, 8
  mov qword [rax], 0

  sub rax, 8
  mov qword [rax], rdx

  ; registers

  sub rax, 8
  mov qword [rax], 0

  sub rax, 8
  mov qword [rax], 0

  sub rax, 8
  mov qword [rax], 0

  sub rax, 8
  mov qword [rax], 0

  sub rax, 8
  mov qword [rax], 0

  sub rax, 8
  mov qword [rax], 0

  ret