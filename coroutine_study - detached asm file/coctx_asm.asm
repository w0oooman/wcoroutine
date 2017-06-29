; This program adds and subtracts 32-bit integers.  
; Last update: 2/1/02  
  

INCLUDELIB kernel32.lib  

.386
.MODEL flat,stdcall  
 
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
co_swapctx PROC
  IFDEF _rsi OR (__i386__) OR defined(_M_IX86) OR defined(i386) OR defined(_X86_)
  		lea eax, [esp + 4] ;//sp 
		mov esp, [esp + 4] ;//parm 1 : &regs[0]
		lea esp, [esp + 32];//parm 1 : &regs[7] + sizeof(void*)

		push eax ;//parm 1 : regs[7] = eax = sp
		push ebp ;//parm 1 : regs[6] = ebp
		push esi ;//parm 1 : regs[5] = esi
		push edi
		push edx
		push ecx
		push ebx
		push[eax - 4] ;//parm 1 : regs[0] = "ret addr"

		mov esp, [eax + 4] ;//parm 2 : &regs[0]

		pop eax  ;//eax = parm 2 : &regs[0] -> ret func addr
		pop ebx  ;//ebx = parm 2 : &regs[1]
		pop ecx  ;//ecx = parm 2 : &regs[2]
		pop edx
		pop edi
		pop esi
		pop ebp
		pop esp
		push eax ;//set ret func addr

		xor eax, eax
		ret      ;//pop ret func addr to eip and func para is s and s1
  ;ELSEDEF (__x86_64__) OR defined(_M_X64) OR defined(__ia64) OR defined(__itanium__) OR defined(_M_IA64) OR defined(__amd64__) OR defined(_M_AMD64)
  ELSE
		lea rsp, [rsp-8]
		push rbp
		push r12
		push r13
		push r14
		push r15
		push rdx
		push rcx
		push r8
		push r9
		lea rsp, [rsp+80]


		mov [rdi+_rbx], rbx
		mov [rdi+_rdi], rdi
		mov [rdi+_rsi], rsi

		mov rcx, [rsp]
		mov [rdi+_rip], rcx
		lea rcx, [rsp+8]
		mov [rdi+_rsp],rcx

		;/* sp */
		mov rcx, [rsi+_rip]
		mov rsp, [rsi+_rsp]
		push rcx

		mov rbx, [rsi+_rbx]
		mov rdi, [rsi+_rdi]
		mov rsi, [rsi+_rsi]

		lea rsp, [rsp-80]
		pop r9
		pop r8
		pop rcx
		pop rdx
		pop r15
		pop r14
		pop r13
		pop r12
		pop rbp
		lea rsp, [rsp+8]

		xor eax, eax
		ret
	ENDIF
co_swapctx ENDP

END 