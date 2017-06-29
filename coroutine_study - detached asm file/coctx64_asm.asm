; This program 64-bit integers.  
; Last update: 22/1/2017
  

INCLUDELIB kernel32.lib  

;.386 ;.486            ; create 32 bit code
;.MODEL flat,stdcall  
 
;C:   #define func(a) {}
;ASM: func MACRO a ENDM

;C:   #define abc 1
;ASM: abc equ 1

_esp  equ 0
_eip  equ 4
;/* ------ */
_rsp  equ 0
_rip  equ 8
_rbx  equ 16
_rdi  equ 24
_rsi  equ 32


.data  
.code
; para1:rcx, para2:rdx
co_swapctx PROC
		lea rax, [rsp+8]
		mov rsp, rcx
		lea rsp, [rsp+128] ;//parm 1 : &regs[15] + 8

		push rax
		push rbx
		push rcx
		push rdx
		push [rax-8]
		push rsi
		push rdi
		push rbp
		push r8
		push r9
		push r10
		push r11
		push r12
		push r13
		push r14
		push r15

		lea rsp, [rdx]

		pop r15
		pop r14
		pop r13
		pop r12
		pop r11
		pop r10
		pop r9
		pop r8
		pop rbp
		pop rdi
		pop rsi
		pop rax ;//ret addr
		pop rdx
		pop rcx
		pop rbx
		pop rsp
		push rax

		xor eax, eax
		ret
co_swapctx ENDP

END 