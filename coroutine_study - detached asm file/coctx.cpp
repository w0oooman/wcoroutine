#include <stdlib.h>
#include <string.h>
#ifdef __cplusplus
extern "C" 
{
#include "coctx.h"
}
#endif

#define _esp 0
#define _eip 4
;/* ------ */
#define _rsp 0
#define _rip 8
#define _rbx 16
#define _rdi 24
#define _rsi 32

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
// 64 bit linux/apple
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

//-------------
// 64 bit windows
//low | regs[0]: r15 |
//    | regs[1]: r14 |
//    | regs[2]: r13 |
//    | regs[3]: r12 |
//    | regs[4]: r11 |
//    | regs[5]: r10 |
//    | regs[6]: r9  |
//    | regs[7]: r8  | 
//    | regs[8]: rbp |
//    | regs[9]: rdi |
//    | regs[10]: rsi |
//    | regs[11]: ret |  //ret func addr
//    | regs[12]: rdx |
//    | regs[13]: rcx | 
//    | regs[14]: rbx |
//hig | regs[15]: rsp |
enum
{
	kRDI = 9,
	kRSI = 10,
	kRETAddr = 11,
	KRDX = 12,
	KRCX = 13,
	kRSP = 15,
};

/*
使用32位(Win32)配置才支持__declspec(naked)和__asm{}，使用64位(X64)并不支持,
因此编译64位时应该把汇编单独成汇编文件而不是嵌入.
*/

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
	ctx->regs[kESP] = (char*)(sp) - sizeof(void*);
	ctx->regs[kEIP] = (char*)pfn;

	return 0;
}

#elif defined(__x86_64__) || defined(_M_X64) || defined(__ia64) || defined(__itanium__) || defined(_M_IA64) || defined(__amd64__) || defined(_M_AMD64)
int co_makectx(coctx_t *ctx, coroutine_func pfn, const void *s, const void *s1)
{
	char *sp = ctx->sp + ctx->size;
	sp = (char*)((unsigned long)sp & -16LL);

	memset(ctx->regs, 0, sizeof(ctx->regs));

	ctx->regs[kRSP] = sp - 8;

	ctx->regs[kRETAddr] = (char*)pfn;

#ifdef _WIN32
	ctx->regs[KRCX] = (char*)s;
	ctx->regs[KRDX] = (char*)s1;
#else
	ctx->regs[KRDI] = (char*)s;
	ctx->regs[KRSI] = (char*)s1;
#endif

	return 0;
}

#endif



/* 因为使用64位编译时不支持__declspec(naked)和__asm，所以为了32位和64编译
** 时能够通用还是把汇编部分分离成**.asm文件，然后生成**.obj供主程序链接!!
*/

//#if defined(__i386__) || defined(_M_IX86) || defined(i386) || defined(_X86_)
///* for CPU32:
//*  when call co_swapctx：push para2 -> push para1 -> push "ret addr"
//*  when co_swapctx return,will jump to "ret addr" and cotinue to run. 
//*/
//__declspec(naked) void __cdecl co_swapctx(coctx_t */*para1*/, coctx_t */*para2*/)
//{
//	__asm
//	{
//#if defined(_MSC_VER)
//		lea eax, [esp + 4] //sp 
//		mov esp, [esp + 4] //parm 1 : &regs[0]
//		lea esp, [esp + 32]//parm 1 : &regs[7] + sizeof(void*)
//
//		push eax //parm 1 : regs[7] = eax = sp
//		push ebp //parm 1 : regs[6] = ebp
//		push esi //parm 1 : regs[5] = esi
//		push edi
//		push edx
//		push ecx
//		push ebx
//		push[eax - 4] //parm 1 : regs[0] = "ret addr"
//
//		mov esp, [eax + 4] //parm 2 : &regs[0]
//
//		pop eax  //eax = parm 2 : &regs[0] -> ret func addr
//		pop ebx  //ebx = parm 2 : &regs[1]
//		pop ecx  //ecx = parm 2 : &regs[2]
//		pop edx
//		pop edi
//		pop esi
//		pop ebp
//		pop esp
//		push eax //set ret func addr
//
//		xor eax, eax
//		ret      //pop ret func addr to eip and func para is s and s1
//#else
//		leal 4(%esp), %eax //sp 
//		movl 4(%esp), %esp
//		leal 32(%esp), %esp //parm 1 : &regs[7] + sizeof(void*)
//
//		pushl %eax //esp ->parm 1
//
//		pushl %ebp
//		pushl %esi
//		pushl %edi
//		pushl %edx
//		pushl %ecx
//		pushl %ebx
//		pushl - 4(%eax)
//
//		movl 4(%eax), %esp //parm 2 -> &regs[0]
//
//		popl %eax  //ret func addr
//		popl %ebx
//		popl %ecx
//		popl %edx
//		popl %edi
//		popl %esi
//		popl %ebp
//		popl %esp
//		pushl %eax //set ret func addr
//
//		xorl %eax, %eax
//		ret
//#endif
//	}
//}
//
//#elif defined(__x86_64__) || defined(_M_X64) || defined(__ia64) || defined(__itanium__) || defined(_M_IA64) || defined(__amd64__) || defined(_M_AMD64)
//
//// win环境64位并不支持__declspec(naked)和__asm，所以以下语句并没有什么用.编译会报错
//__declspec(naked) void __cdecl co_swapctx(coctx_t *, coctx_t *)
//{
//	__asm
//	{
//		// win环境64位并不支持__declspec(naked)和__asm，所以以下语句并没有什么用.编译会报错
//#if defined(_WIN32)
		//lea rax, [rsp + 8]
		//mov rsp, rcx
		//lea rsp, [rsp + 128];//parm 1 : &regs[15] + 8

		//push rax
		//push rbx
		//push rcx
		//push rdx
		//push[rax - 8]
		//push rsi
		//push rdi
		//push rbp
		//push r8
		//push r9
		//push r10
		//push r11
		//push r12
		//push r13
		//push r14
		//push r15

		//lea rsp, [rdx]

		//pop r15
		//pop r14
		//pop r13
		//pop r12
		//pop r11
		//pop r10
		//pop r9
		//pop r8
		//pop rbp
		//pop rdi
		//pop rsi
		//pop rax//ret addr
		//pop rdx
		//pop rcx
		//pop rbx
		//pop rsp
		//push rax

		//xor eax, eax
		//ret		
//#else
		//leaq - 8(%rsp), %rsp
		//pushq %rbp
		//pushq %r12
		//pushq %r13
		//pushq %r14
		//pushq %r15
		//pushq %rdx
		//pushq %rcx
		//pushq %r8
		//pushq %r9
		//leaq 80(%rsp), %rsp

		//movq %rbx, _rbx(%rdi)
		//movq %rdi, _rdi(%rdi)
		//movq %rsi, _rsi(%rdi)
		///* sp */
		//movq(%rsp), %rcx
		//movq %rcx, _rip(%rdi)
		//leaq 8(%rsp), %rcx
		//movq %rcx, _rsp(%rdi)

		///* sp */
		//movq _rip(%rsi), %rcx
		//movq _rsp(%rsi), %rsp
		//pushq %rcx

		//movq _rbx(%rsi), %rbx
		//movq _rdi(%rsi), %rdi
		//movq _rsi(%rsi), %rsi

		//leaq - 80(%rsp), %rsp
		//popq %r9
		//popq %r8
		//popq %rcx
		//popq %rdx
		//popq %r15
		//popq %r14
		//popq %r13
		//popq %r12
		//popq %rbp
		//leaq 8(%rsp), %rsp

		//xorl %eax, %eax
		//ret
//#endif
//	}
//}
//
//#endif
