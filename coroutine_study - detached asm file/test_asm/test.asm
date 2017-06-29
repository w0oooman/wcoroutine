TITLE Add and Subtract              (AddSub.asm)  
  
; This program adds and subtracts 32-bit integers.  
; Last update: 2/1/02  
  
  
INCLUDELIB kernel32.lib  
  
.MODEL flat,stdcall  
  
;.code  
ExitProcess PROTO,                      ; exit program  
    dwExitCode:DWORD        ; return code  
  
  
.data  
.code  
main PROC  
  
    mov eax,10000h      ; EAX = 10000h  
    add eax,40000h      ; EAX = 50000h  
    sub eax,20000h      ; EAX = 30000h  
  
    push 0h  
    call ExitProcess  
  
main ENDP  
END main 