#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "co_routine.h"
#include "coctx.h"
#include "co_routine_inner.h"
#include "main.h"

//extern "C"
//{
//	extern void coctx_swap(coctx_t *, coctx_t*) asm("coctx_swap");
//};
using namespace std;
stCoRoutine_t *GetCurrCo(stCoRoutineEnv_t *env);
struct stCoEpoll_t;

struct stCoRoutineEnv_t
{
	stCoRoutine_t *pCallStack[128];
	int iCallStackSize;
	stCoEpoll_t *pEpoll;

	//for copy stack log lastco and nextco
	stCoRoutine_t* pending_co;
	stCoRoutine_t* ocupy_co;
};
//int socket(int domain, int type, int protocol);
void co_log_err(const char *fmt, ...)
{
}

__declspec(naked) void __cdecl coctx_swap(coctx_t *, coctx_t *)
{
#if defined(__i386__) || defined(_M_IX86) || defined(i386) || defined(_X86_)
	__asm
	{
		lea eax, [esp+4] //sp 
		mov esp, [esp+4]
		lea esp, [esp+32]//parm a : &regs[7] + sizeof(void*)

		push eax //esp ->parm a 

		push ebp
		push esi
		push edi
		push edx
		push ecx
		push ebx
		push [eax-4]


		mov esp, [eax+4] //parm b -> &regs[0]

		pop eax  //ret func addr
		pop ebx  
		pop ecx
		pop edx
		pop edi
		pop esi
		pop ebp
		pop esp
		push eax //set ret func addr

		xor eax, eax
		ret
	}
#elif defined(__x86_64__) || defined(_M_X64) || defined(__ia64) || defined(__itanium__) || defined(_M_IA64) || defined(__amd64__) || defined(_M_AMD64)

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
int coctx_init(coctx_t *ctx)
{
	memset(ctx, 0, sizeof(*ctx));
	return 0;
}

int coctx_make(coctx_t *ctx, coctx_pfn_t pfn, const void *s, const void *s1)
{
	//make room for coctx_param
	char *sp = ctx->ss_sp + ctx->ss_size - sizeof(coctx_param_t);
	sp = (char*)((unsigned long)sp & -16L);


	coctx_param_t* param = (coctx_param_t*)sp;
	param->s1 = s;
	param->s2 = s1;

	memset(ctx->regs, 0, sizeof(ctx->regs));

	ctx->regs[kESP] = (char*)(sp)-sizeof(void*);
	ctx->regs[kEIP] = (char*)pfn;

	return 0;
}
#elif defined(__x86_64__) || defined(_M_X64) || defined(__ia64) || defined(__itanium__) || defined(_M_IA64) || defined(__amd64__) || defined(_M_AMD64)
int coctx_make(coctx_t *ctx, coctx_pfn_t pfn, const void *s, const void *s1)
{
	char *sp = ctx->ss_sp + ctx->ss_size;
	sp = (char*)((unsigned long)sp & -16LL);

	memset(ctx->regs, 0, sizeof(ctx->regs));

	ctx->regs[kRSP] = sp - 8;

	ctx->regs[kRETAddr] = (char*)pfn;

	ctx->regs[kRDI] = (char*)s;
	ctx->regs[kRSI] = (char*)s1;
	return 0;
}

int coctx_init(coctx_t *ctx)
{
	memset(ctx, 0, sizeof(*ctx));
	return 0;
}

#endif

struct stCoRoutine_t *co_create_env(stCoRoutineEnv_t * env, const stCoRoutineAttr_t* attr,
	pfn_co_routine_t pfn, void *arg);
static int GetPid()
{
	return 11;
}

static stCoRoutineEnv_t* g_arrCoEnvPerThread[204800] = { 0 };
void co_init_curr_thread_env()
{
	int/*pid_t*/ pid = GetPid();
	g_arrCoEnvPerThread[pid] = (stCoRoutineEnv_t*)calloc(1, sizeof(stCoRoutineEnv_t));
	stCoRoutineEnv_t *env = g_arrCoEnvPerThread[pid];

	env->iCallStackSize = 0;
	struct stCoRoutine_t *self = co_create_env(env, NULL, NULL, NULL);
	self->cIsMain = 1;

	env->pending_co = NULL;
	env->ocupy_co = NULL;

	coctx_init(&self->ctx);

	env->pCallStack[env->iCallStackSize++] = self;

	//stCoEpoll_t *ev = AllocEpoll();
	//SetEpoll(env, ev);
}
stCoRoutineEnv_t *co_get_curr_thread_env()
{
	return g_arrCoEnvPerThread[GetPid()];
}

/////////////////for copy stack //////////////////////////
stStackMem_t* co_alloc_stackmem(unsigned int stack_size)
{
	stStackMem_t* stack_mem = (stStackMem_t*)malloc(sizeof(stStackMem_t));
	stack_mem->ocupy_co = NULL;
	stack_mem->stack_size = stack_size;
	stack_mem->stack_buffer = (char*)malloc(stack_size);
	stack_mem->stack_bp = stack_mem->stack_buffer + stack_size;
	return stack_mem;
}

stShareStack_t* co_alloc_sharestack(int count, int stack_size)
{
	stShareStack_t* share_stack = (stShareStack_t*)malloc(sizeof(stShareStack_t));
	share_stack->alloc_idx = 0;
	share_stack->stack_size = stack_size;

	//alloc stack array
	share_stack->count = count;
	stStackMem_t** stack_array = (stStackMem_t**)calloc(count, sizeof(stStackMem_t*));
	for (int i = 0; i < count; i++)
	{
		stack_array[i] = co_alloc_stackmem(stack_size);
	}
	share_stack->stack_array = stack_array;
	return share_stack;
}

static stStackMem_t* co_get_stackmem(stShareStack_t* share_stack)
{
	if (!share_stack)
	{
		return NULL;
	}
	int idx = share_stack->alloc_idx % share_stack->count;
	share_stack->alloc_idx++;

	return share_stack->stack_array[idx];
}



static int CoRoutineFunc(stCoRoutine_t *co, void *)
{
	if (co->pfn)
	{
		co->pfn(co->arg);
	}
	co->cEnd = 1;

	stCoRoutineEnv_t *env = co->env;

	co_yield_env(env);

	return 0;
}



struct stCoRoutine_t *co_create_env(stCoRoutineEnv_t * env, const stCoRoutineAttr_t* attr,
	pfn_co_routine_t pfn, void *arg)
{

	stCoRoutineAttr_t at;
	if (attr)
	{
		memcpy(&at, attr, sizeof(at));
	}
	if (at.stack_size <= 0)
	{
		at.stack_size = 128 * 1024;
	}
	else if (at.stack_size > 1024 * 1024 * 8)
	{
		at.stack_size = 1024 * 1024 * 8;
	}

	if (at.stack_size & 0xFFF)
	{
		at.stack_size &= ~0xFFF;
		at.stack_size += 0x1000;
	}

	stCoRoutine_t *lp = (stCoRoutine_t*)malloc(sizeof(stCoRoutine_t));


	lp->env = env;
	lp->pfn = pfn;
	lp->arg = arg;

	stStackMem_t* stack_mem = NULL;
	if (at.share_stack)
	{
		stack_mem = co_get_stackmem(at.share_stack);
		at.stack_size = at.share_stack->stack_size;
	}
	else
	{
		stack_mem = co_alloc_stackmem(at.stack_size);
	}
	lp->stack_mem = stack_mem;

	lp->ctx.ss_sp = stack_mem->stack_buffer;
	lp->ctx.ss_size = at.stack_size;

	lp->cStart = 0;
	lp->cEnd = 0;
	lp->cIsMain = 0;
	lp->cEnableSysHook = 0;
	lp->cIsShareStack = at.share_stack != NULL;

	lp->save_size = 0;
	lp->save_buffer = NULL;

	return lp;
}

int co_create(stCoRoutine_t **ppco, const stCoRoutineAttr_t *attr, pfn_co_routine_t pfn, void *arg)
{
	if (!co_get_curr_thread_env())
	{
		co_init_curr_thread_env();
	}
	stCoRoutine_t *co = co_create_env(co_get_curr_thread_env(), attr, pfn, arg);
	*ppco = co;
	return 0;
}
void co_free(stCoRoutine_t *co)
{
	free(co);
}
void co_release(stCoRoutine_t *co)
{
	if (co->cEnd)
	{
		free(co);
	}
}

void co_swap(stCoRoutine_t* curr, stCoRoutine_t* pending_co);

void co_resume(stCoRoutine_t *co)
{
	stCoRoutineEnv_t *env = co->env;
	stCoRoutine_t *lpCurrRoutine = env->pCallStack[env->iCallStackSize - 1];
	if (!co->cStart)
	{
		coctx_make(&co->ctx, (coctx_pfn_t)CoRoutineFunc, co, 0);
		co->cStart = 1;
	}
	env->pCallStack[env->iCallStackSize++] = co;
	co_swap(lpCurrRoutine, co);


}
void co_yield_env(stCoRoutineEnv_t *env)
{

	stCoRoutine_t *last = env->pCallStack[env->iCallStackSize - 2];
	stCoRoutine_t *curr = env->pCallStack[env->iCallStackSize - 1];

	env->iCallStackSize--;

	co_swap(curr, last);
}

void co_yield_ct()
{

	co_yield_env(co_get_curr_thread_env());
}
void co_yield(stCoRoutine_t *co)
{
	co_yield_env(co->env);
}

void save_stack_buffer(stCoRoutine_t* ocupy_co)
{
	///copy out
	stStackMem_t* stack_mem = ocupy_co->stack_mem;
	int len = stack_mem->stack_bp - ocupy_co->stack_sp;

	if (ocupy_co->save_buffer)
	{
		free(ocupy_co->save_buffer), ocupy_co->save_buffer = NULL;
	}

	ocupy_co->save_buffer = (char*)malloc(len); //malloc buf;
	ocupy_co->save_size = len;

	memcpy(ocupy_co->save_buffer, ocupy_co->stack_sp, len);
}

void co_swap(stCoRoutine_t* curr, stCoRoutine_t* pending_co)
{
	stCoRoutineEnv_t* env = co_get_curr_thread_env();

	//get curr stack sp
	char c;
	curr->stack_sp = &c;

	if (!pending_co->cIsShareStack)
	{
		env->pending_co = NULL;
		env->ocupy_co = NULL;
	}
	else
	{
		env->pending_co = pending_co;
		//get last occupy co on the same stack mem
		stCoRoutine_t* ocupy_co = pending_co->stack_mem->ocupy_co;
		//set pending co to ocupy thest stack mem;
		pending_co->stack_mem->ocupy_co = pending_co;

		env->ocupy_co = ocupy_co;
		if (ocupy_co && ocupy_co != pending_co)
		{
			save_stack_buffer(ocupy_co);
		}
	}

	//swap context
	coctx_swap(&(curr->ctx), &(pending_co->ctx));

	//stack buffer may be overwrite, so get again;
	stCoRoutineEnv_t* curr_env = co_get_curr_thread_env();
	stCoRoutine_t* update_ocupy_co = curr_env->ocupy_co;
	stCoRoutine_t* update_pending_co = curr_env->pending_co;

	if (update_ocupy_co && update_pending_co && update_ocupy_co != update_pending_co)
	{
		//resume stack buffer
		if (update_pending_co->save_buffer && update_pending_co->save_size > 0)
		{
			memcpy(update_pending_co->stack_sp, update_pending_co->save_buffer, update_pending_co->save_size);
		}
	}
}


const int loop_num = 1000;
void *test(void *para)
{
	stCoRoutine_t *co = *(stCoRoutine_t **)para;
	for (int i = 0; i < loop_num; i++)
	{
		printf("-----test i=%d\n", i);
		co_yield(co);
	}
	return 0;
}

void unshared_stack()
{
	stCoRoutine_t *co1, *co2;
	co_create(&co1, NULL, test, &co1);
	co_create(&co2, NULL, test, &co2);
	printf("addr:co1 = %08X,co2 = %08X\n", (int)co1, (int)co2);

	int num = loop_num;
	while (num-- >= 0){
		co_resume(co1);
		co_resume(co2);
	}
}

void shared_stack()
{
	stShareStack_t* share_stack = co_alloc_sharestack(1, 1024 * 128);
	stCoRoutineAttr_t attr;
	attr.stack_size = 0;
	attr.share_stack = share_stack;

	stCoRoutine_t *co1, *co2;
	co_create(&co1, &attr, test, &co1);
	co_create(&co2, &attr, test, &co2);
	printf("addr:co1 = %08X,co2 = %08X\n", (int)co1, (int)co2);

	int num = loop_num;
	while (num-- >= 0){
		co_resume(co1);
		co_resume(co2);
	}
		
}

#include <windows.h>
int main()
{
	DWORD begin = GetTickCount();

	//unshared_stack();
	shared_stack();

	printf("run time = %d\n",GetTickCount()-begin);
	return 0;
}