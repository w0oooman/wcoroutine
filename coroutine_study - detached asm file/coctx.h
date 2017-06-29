#ifndef __COCTX_H__
#define __COCTX_H__
#include <stdio.h>
#include "coroutine.h"

struct coctx_t
{
#if defined(__i386__) || defined(_M_IX86) || defined(i386) || defined(_X86_)
	void *regs[8];
#else
#ifdef _MSC_VER 
	void *regs[16];
#else
	void *regs[14];
#endif
#endif
	size_t  size;
	char   *sp;
};

struct copara_t
{
	const void *para1;
	const void *para2;
};

typedef struct coctx_t coctx_t;
typedef struct copara_t copara_t;

//shared stack type
struct coshared_t
{
	coctx_t ctx;

	unsigned char ismain;
	unsigned char isshared;
	char *s_stack;         //stack pointer
	ptrdiff_t s_cap;       //stack capacity
	ptrdiff_t s_size;      //stack size
};

typedef struct coshared_t coshared_t;

extern int co_makectx(coctx_t *ctx, coroutine_func pfn, const void *s, const void *s1);

#ifdef __cplusplus
extern "C"
{
#endif
	void __cdecl co_swapctx(coctx_t *, coctx_t *);
#ifdef __cplusplus
}
#endif

#endif