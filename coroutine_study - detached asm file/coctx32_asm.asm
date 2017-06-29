; This program 32-bit integers.  
; Last update: 22/1/2017
  

INCLUDELIB kernel32.lib  

PUBLIC co_swapctx
;.386 ;.486            ; create 32 bit code
;.MODEL flat,stdcall   ; generate "funcname" to "_funcname"
.MODEL flat,c;         ; 32 bit memory model AND call type(generate "funcname" to "funcname")
 
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
		push [eax - 4] ;//parm 1 : regs[0] = "ret addr"

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
co_swapctx ENDP

END