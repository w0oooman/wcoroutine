#include <stdlib.h>
#include <string.h>
//#ifdef __cplusplus
//extern "C" 
//{
//#include "coctx.h"
//}
//#endif
#include "coctx.h"
//----- --------
// 32 bit
// | regs[0]: ret |
// | regs[1]: ebx |
// | regs[2]: ecx |
// | regs[3]: edx |
// | regs[4]: edi |
// | regs[5]: esi |
// | regs[6]: ebp |
// | regs[7]: eax |  = esp
enum
{
	kEIP = 0,
	kESP = 7,
};

//-------------
// 64 bit
//low | regs[0]: r15 |
//    | regs[1]: r14 |
//    | regs[2]: r13 |
//    | regs[3]: r12 |
//    | regs[4]: r9  |
//    | regs[5]: r8  | 
//    | regs[6]: rbp |
//    | regs[7]: rdi |
//    | regs[8]: rsi |
//    | regs[9]: ret |  //ret func addr
//    | regs[10]: rdx |
//    | regs[11]: rcx | 
//    | regs[12]: rbx |
//hig | regs[13]: rsp |
enum
{
	kRDI = 7,
	kRSI = 8,
	kRETAddr = 9,
	kRSP = 13,
};


#if defined(__i386__) || defined(_M_IX86) || defined(i386) || defined(_X86_)

int co_makectx(coctx_t *ctx, coroutine_func pfn, const void *s, const void *s1)
{
	//make room for copara_t
	char *sp = ctx->sp + ctx->size - sizeof(copara_t);
	sp = (char*)((unsigned long)sp & -16L);

	/*the operation equal "push s/s1",so when
	 *co_swapctx ret,s/s1 is the func para.*/
	copara_t* param = (copara_t*)sp;
	param->para1 = s;
	param->para2 = s1;

	memset(ctx->regs, 0, sizeof(ctx->regs));

	//make room for "ret addr"
	ctx->regs[kESP] = (char*)(sp)-sizeof(void*);
	ctx->regs[kEIP] = (char*)pfn;

	return 0;
}

/* for CPU32:
*  when call co_swapctx£ºpush para2 -> push para1 -> push "ret addr"
*  when co_swapctx return,will jump to "ret addr" and cotinue to run. 
*/
__declspec(naked) void __cdecl co_swapctx(coctx_t */*para1*/, coctx_t */*para2*/)
{
	__asm
	{
#if defined(_WIN32)
		lea eax, [esp + 4] //sp 
		mov esp, [esp + 4] //parm 1 : &regs[0]
		lea esp, [esp + 32]//parm 1 : &regs[7] + sizeof(void*)

		push eax //parm 1 : regs[7] = eax = sp
		push ebp //parm 1 : regs[6] = ebp
		push esi //parm 1 : regs[5] = esi
		push edi
		push edx
		push ecx
		push ebx
		push[eax - 4] //parm 1 : regs[0] = "ret addr"

		mov esp, [eax + 4] //parm 2 : &regs[0]

		pop eax  //eax = parm 2 : &regs[0] -> ret func addr
		pop ebx  //ebx = parm 2 : &regs[1]
		pop ecx  //ecx = parm 2 : &regs[2]
		pop edx
		pop edi
		pop esi
		pop ebp
		pop esp
		push eax //set ret func addr

		xor eax, eax
		ret      //pop ret func addr to eip and func para is s and s1
#else
		leal 4(%esp), %eax //sp 
		movl 4(%esp), %esp
		leal 32(%esp), %esp //parm 1 : &regs[7] + sizeof(void*)

		pushl %eax //esp ->parm 1

		pushl %ebp
		pushl %esi
		pushl %edi
		pushl %edx
		pushl %ecx
		pushl %ebx
		pushl - 4(%eax)

		movl 4(%eax), %esp //parm 2 -> &regs[0]

		popl %eax  //ret func addr
		popl %ebx
		popl %ecx
		popl %edx
		popl %edi
		popl %esi
		popl %ebp
		popl %esp
		pushl %eax //set ret func addr

		xorl %eax, %eax
		ret
#endif
	}
}

#elif defined(__x86_64__) || defined(_M_X64) || defined(__ia64) || defined(__itanium__) || defined(_M_IA64) || defined(__amd64__) || defined(_M_AMD64)
int co_makectx(coctx_t *ctx, coroutine_func pfn, const void *s, const void *s1)
{
	char *sp = ctx->sp + ctx->size;
	sp = (char*)((unsigned long)sp & -16LL);

	memset(ctx->regs, 0, sizeof(ctx->regs));

	ctx->regs[kRSP] = sp - 8;

	ctx->regs[kRETAddr] = (char*)pfn;

	ctx->regs[kRDI] = (char*)s;
	ctx->regs[kRSI] = (char*)s1;
	return 0;
}

__declspec(naked) void __cdecl co_swapctx(coctx_t *, coctx_t *)
{
	__asm
	{
#if defined(_WIN32)
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

		/* sp */
		mov rcx, [rsp]
		mov [rdi+_rip], rcx
		lea rcx, [rsp+8]
		mov [rdi+_rsp],rcx

		/* sp */
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
#else
		leaq - 8(%rsp), %rsp
		pushq %rbp
		pushq %r12
		pushq %r13
		pushq %r14
		pushq %r15
		pushq %rdx
		pushq %rcx
		pushq %r8
		pushq %r9
		leaq 80(%rsp), %rsp

		movq %rbx, _rbx(%rdi)
		movq %rdi, _rdi(%rdi)
		movq %rsi, _rsi(%rdi)
		/* sp */
		movq(%rsp), %rcx
		movq %rcx, _rip(%rdi)
		leaq 8(%rsp), %rcx
		movq %rcx, _rsp(%rdi)

		/* sp */
		movq _rip(%rsi), %rcx
		movq _rsp(%rsi), %rsp
		pushq %rcx

		movq _rbx(%rsi), %rbx
		movq _rdi(%rsi), %rdi
		movq _rsi(%rsi), %rsi

		leaq - 80(%rsp), %rsp
		popq %r9
		popq %r8
		popq %rcx
		popq %rdx
		popq %r15
		popq %r14
		popq %r13
		popq %r12
		popq %rbp
		leaq 8(%rsp), %rsp

		xorl %eax, %eax
		ret
#endif
	}
}

#endif
