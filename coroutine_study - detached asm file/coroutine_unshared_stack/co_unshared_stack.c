/*
    非共享栈 实现的 协程原语 2017-01-17
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include "coroutine.h"
#include "coctx.h"

#define DEFAULT_COROUTINE 16
struct coroutine;

struct schedule {
	coctx_t main;
	int nco;
	int cap;
	int curID;
	struct coroutine **co;
};

struct coroutine {
	coroutine_func func;
	void *ud;
	coctx_t ctx;
	struct schedule * sch;
	int status;
	char stack[1024 * 128];
};

static void
mainfunc(uint32_t para1, uint32_t para2) {
	struct schedule *S = (struct schedule *)para1;
	int id = S->curID;
	struct coroutine *C = S->co[id];
	C->func(S, C->ud);
	S->co[id] = NULL;
	--S->nco;
	S->curID = -1;
	co_swapctx(&C->ctx, &S->main);
}

struct coroutine *
	co_new(struct schedule *S, coroutine_func func, void *ud) {
	struct coroutine * co = malloc(sizeof(*co));
	co->func = func;
	co->ud = ud;
	co->sch = S;
	co->status = COROUTINE_READY;
	co->ctx.sp = co->stack;
	co->ctx.size = sizeof(co->stack);
	uintptr_t ptr = (uintptr_t)S;
	co_makectx(&co->ctx, mainfunc, S, 0);
	return co;
}

void
co_delete(struct coroutine *co) {
	free(co);
}

struct schedule *
	coroutine_open(void) {
	struct schedule *S = malloc(sizeof(*S));
	S->nco = 0;
	S->cap = DEFAULT_COROUTINE;
	S->curID = -1;
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
	return -1;
}

void
coroutine_resume(struct schedule * S, int id) {
	assert(S->curID == -1);
	assert(id >= 0 && id < S->cap);
	if (id < 0 || id >= S->cap) return;
	struct coroutine *C = S->co[id];
	if (C == NULL) return;
	int status = C->status;
	switch (status) {
	case COROUTINE_READY:case COROUTINE_SUSPEND:
		S->curID = id;
		C->status = COROUTINE_RUNNING;
		co_swapctx(&S->main, &C->ctx);
		if (!S->co[id]) co_delete(C);
		break;
	default:
		assert(0);
		break;
	}
}

void
coroutine_yield(struct schedule * S) {
	int id = S->curID;
	assert(id >= 0);
	if (id < 0) return;
	struct coroutine * C = S->co[id];
	
	C->status = COROUTINE_SUSPEND;
	S->curID = -1;
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
	return S->curID;
}