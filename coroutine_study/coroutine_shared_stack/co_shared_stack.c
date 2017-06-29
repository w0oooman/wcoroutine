/*
共享栈 实现的 协程原语 2017-01-17
*/


/*
    windows 下 DEBUG模式或带优化的Release程序会崩溃，暂时没找到原因。。
	可能是堆栈被破坏了吧。。
	无优化Release版本则不会崩溃.
*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include "coroutine.h"
#include "coctx.h"

#define STACK_SIZE (1024*1024)
#define DEFAULT_COROUTINE 16
#define INVALID_COID  (-1)
struct coroutine;

struct schedule {
	char  stack[STACK_SIZE];
	coctx_t main;
	int   nco;
	int   cap;
	int   curid;
	int   previd;
	char *prevsp;
	struct coroutine **co;
};

struct coroutine {
	coroutine_func func;
	void *ud;
	coctx_t ctx;
	struct schedule * sch;
	int status;

	char *s_stack;
	ptrdiff_t s_cap;
	ptrdiff_t s_size;
};

void
co_delete(struct coroutine *co) {
	free(co);
}

static void
mainfunc(uint32_t low32, uint32_t hi32) {
	uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hi32 << 32);
	struct schedule *S = (struct schedule *)ptr;
	int id = S->curid;
	struct coroutine *C = S->co[id];
	C->func(S, C->ud);
	co_delete(C);
	S->co[id] = NULL;
	--S->nco;
	S->curid = INVALID_COID;
	co_swapctx(&C->ctx, &S->main);
}

struct coroutine *
	co_new(struct schedule *S, coroutine_func func, void *ud) {
	struct coroutine * co = malloc(sizeof(*co));
	co->func = func;
	co->ud = ud;
	co->sch = S;
	co->status = COROUTINE_READY;
	co->s_stack = NULL;
	co->s_cap = 0;
	co->s_size = 0;
	uintptr_t ptr = (uintptr_t)S;
	co->ctx.sp = S->stack;
	co->ctx.size = sizeof(S->stack);
	co_makectx(&co->ctx, mainfunc, (uint32_t)ptr, ((uint32_t)(ptr)) >> 32);
	return co;
}

struct schedule *
	coroutine_open(void) {
	struct schedule *S = malloc(sizeof(*S));
	S->nco = 0;
	S->cap = DEFAULT_COROUTINE;
	S->curid = INVALID_COID;
	S->previd = INVALID_COID;
	S->co = malloc(sizeof(struct coroutine *) * S->cap);
	memset(S->co, 0, sizeof(struct coroutine *) * S->cap);
	return S;
}

void
coroutine_close(struct schedule *S) {
	int i;
	for (i = 0; i < S->cap; i++) {
		struct coroutine * co = S->co[i];
		if (co) {
			co_delete(co);
		}
	}
	free(S->co);
	S->co = NULL;
	free(S);
}

int
coroutine_new(struct schedule *S, coroutine_func func, void *ud) {
	struct coroutine *co = co_new(S, func, ud);
	if (S->nco >= S->cap) {
		int id = S->cap;
		S->co = realloc(S->co, S->cap * 2 * sizeof(struct coroutine *));
		memset(S->co + S->cap, 0, sizeof(struct coroutine *) * S->cap);
		S->co[S->cap] = co;
		S->cap *= 2;
		++S->nco;
		return id;
	}
	else {
		int i;
		for (i = 0; i < S->cap; i++) {
			int id = (i + S->nco) % S->cap;
			if (S->co[id] == NULL) {
				S->co[id] = co;
				++S->nco;
				return id;
			}
		}
	}
	assert(0);
	return INVALID_COID;
}

static void
save_stack(struct coroutine *C, char *top, char *sp) {
	assert(top - sp <= STACK_SIZE);
	if (C->s_cap < top - sp) {
		free(C->s_stack);
		C->s_cap = top - sp;
		C->s_stack = malloc(C->s_cap);
	}
	C->s_size = top - sp;
	memcpy(C->s_stack, sp, C->s_size);
}

void 
coroutine_resume(struct schedule * S, int id) {
	assert(S->curid == INVALID_COID);
	assert(id >= 0 && id < S->cap);
	if (id < 0 || id >= S->cap) return;
	struct coroutine *C = S->co[id];
	if (C == NULL) return;
	int status = C->status;
	switch (status) {
	case COROUTINE_READY:
		if (S->previd != INVALID_COID && id != S->previd){
			save_stack(S->co[S->previd], S->stack + STACK_SIZE, S->prevsp);
			S->previd = INVALID_COID;
		}
		S->curid = id;
		C->status = COROUTINE_RUNNING;
		co_swapctx(&S->main, &C->ctx);
		break;
	case COROUTINE_SUSPEND:
		if (S->previd != INVALID_COID && id != S->previd){ /* save prev stack */
			save_stack(S->co[S->previd], S->stack + STACK_SIZE, S->prevsp);
			S->previd = INVALID_COID;
		}
		if (id != S->previd){ /* recovery current stack */
			memcpy(/*S->stack + STACK_SIZE - C->s_size*/S->prevsp, C->s_stack, C->s_size);/* S->stack + STACK_SIZE - C->s_size == S->prevsp */
		}
		S->curid = id;
		C->status = COROUTINE_RUNNING;
		co_swapctx(&S->main, &C->ctx);
		break;
	default:
		assert(0);
		break;
	}
}

void
coroutine_yield(struct schedule * S) {
	int id = S->curid;
	assert(id >= 0);
	if (id < 0) return;

	char cursp;
	S->prevsp = &cursp;
	S->previd = id;

	struct coroutine * C = S->co[id];
	C->status = COROUTINE_SUSPEND;
	S->curid = INVALID_COID;
	co_swapctx(&C->ctx, &S->main);
}

int
coroutine_status(struct schedule * S, int id) {
	assert(id >= 0 && id < S->cap);
	if (id < 0 || id >= S->cap) return COROUTINE_DEAD;
	if (S->co[id] == NULL) {
		return COROUTINE_DEAD;
	}
	return S->co[id]->status;
}

int
coroutine_running(struct schedule * S) {
	return S->curid;
}