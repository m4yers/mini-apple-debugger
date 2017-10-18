  .section	__TEXT,__text,regular,pure_instructions
  .globl	_main
  .p2align	4, 0x90

_main:
  movq $0x2000004, %rax
  movq $2, %rdi
  movq msg(%rip), %rsi
  movq len(%rip), %rdx
  syscall

  movq $0x2000001, %rax
  movq $0, %rdi
  syscall

  .section	__TEXT,__cstring,cstring_literals
msg:
  .asciz	"Hello, World!\n"
len:
  .long . - msg
