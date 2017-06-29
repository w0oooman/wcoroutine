//#include "mysetjmp.h"
//
//#define PAGE_SIZE  (4096)
//#define STACK_SIZE (2048*PAGE_SIZE)
//#define JB(a,b) ((a)[0].__jmpbuf[(b)])
//
//enum jb { JB_RBX, JB_RBP, JB_R12, JB_R13, JB_R14, JB_R15, JB_RSP, JB_PC };
//typedef long long int jb_int_t;
//
//typedef struct co {
//	struct co *prev, *next;
//	jmp_buf jmp_buf;
//	void *stack;
//} co_t;
//
//co_t exit_co = { 0 }, *curr_co = NULL;
//jmp_buf main_buf;
//
//void co_yield() {
//	if (setjmp(curr_co->jmp_buf))
//		return;
//	curr_co = curr_co->next;
//	longjmp(curr_co->jmp_buf, 1);
//}
//
//void co_main() {
//	if (setjmp(exit_co.jmp_buf))
//		return;
//	if (setjmp(main_buf)) {
//		co_t *last_co = curr_co;
//		curr_co->next->prev = curr_co->prev;
//		curr_co->prev->next = curr_co->next;
//		curr_co = (last_co != last_co->next) ? curr_co->next : &exit_co;
//		munmap(last_co->stack, STACK_SIZE);
//	}
//	longjmp(curr_co->jmp_buf, 1);
//}
//
//void co_exit() {
//	longjmp(main_buf, 1);
//}
//
//void co_start() {
//	register void(*f)(void *)asm("rbx");
//	register void *arg asm("r12");
//	f(arg);
//	co_exit();
//}
//
//static inline jb_int_t translate_address(jb_int_t addr) {
//	jb_int_t ret;
//	asm volatile("xor %%fs:0x30,%0\nrol $0x11,%0\n" :"=g"(ret) : "0"(addr));
//	return ret;
//}
//
//void co_create(void(*f)(void *), void *arg) {
//	void *stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
//		MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
//	mprotect(stack, PAGE_SIZE, PROT_NONE);
//	mprotect(stack + STACK_SIZE - PAGE_SIZE, PAGE_SIZE, PROT_NONE);
//	co_t *new_co = (co_t *)(stack + STACK_SIZE - PAGE_SIZE - sizeof(co_t));
//	new_co->stack = stack;
//	if (!curr_co) {
//		new_co->prev = new_co;
//		new_co->next = new_co;
//		curr_co = new_co;
//	}
//	else {
//		new_co->next = curr_co;
//		new_co->prev = curr_co->prev;
//		curr_co->prev->next = new_co;
//		curr_co->prev = new_co;
//	}
//	JB(new_co->jmp_buf, JB_PC) = translate_address((jb_int_t)co_start);
//	JB(new_co->jmp_buf, JB_RSP) = translate_address((jb_int_t)new_co);
//	JB(new_co->jmp_buf, JB_RBX) = (jb_int_t)f;
//	JB(new_co->jmp_buf, JB_R12) = (jb_int_t)arg;
//}
//
//#include <stdint.h>
//#include <stdio.h>
//
//void my_coro(void *arg) {
//	uintptr_t n = (uintptr_t)arg;
//	unsigned int i;
//	for (i = 0; i < n; i++) {
//		printf("%lu\n", n);
//		co_yield();
//	}
//}
//
//void coroutine_setjmp() {
//	co_create(my_coro, (void *)3);
//	co_create(my_coro, (void *)4);
//	co_create(my_coro, (void *)5);
//	co_main();
//}
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
///*
//* Multithread demo program.
//* Hebrew University OS course.
//*
//* Questions: os@cs.huji.ac.il
//*/
//
//#include <stdio.h>
//#include <setjmp.h>
//#include <signal.h>
//#include <string.h>
//#include <unistd.h>
//#include <sys/time.h>
//
//#define SECOND 1000000
//#define STACK_SIZE 40096
//
//char stack1[STACK_SIZE];
//char stack2[STACK_SIZE];
//
//sigjmp_buf jbuf[2];
//
//////////////////////////////////////////////////////////////////////////
//#ifdef __x86_64__
//// code for 64 bit Intel arch
//
//typedef unsigned long address_t;
//#define JB_SP 6
//#define JB_PC 7
//
////A translation required when using an address of a variable
////Use this as a black box in your code.
//address_t translate_address(address_t addr)
//{
//	address_t ret;
//	asm volatile("xor    %%fs:0x30,%0\n"
//		"rol    $0x11,%0\n"
//		: "=g" (ret)
//		: "0" (addr));
//	return ret;
//}
//
//#else
//// code for 32 bit Intel arch
//
//typedef unsigned int address_t;
//#define JB_SP 4
//#define JB_PC 5 
//
////A translation required when using an address of a variable
////Use this as a black box in your code.
//address_t translate_address(address_t addr)
//{
//	address_t ret;
//	asm volatile("xor    %%gs:0x18,%0\n"
//		"rol    $0x9,%0\n"
//		: "=g" (ret)
//		: "0" (addr));
//	return ret;
//}
//
//#endif
//
//////////////////////////////////////////////////////////////////////////
//
//void switchThreads();
//
//void f(void)
//{
//	printf("b\n");
//	int a = 1;
//	fprintf(stderr, "%d\n", a); //XXX
//	printf("c\n");
//	int i = 0;
//	while (1) {
//		++i;
//		printf("in f (%d)\n", i);
//		if (i % 3 == 0) {
//			printf("f: switching\n");
//			switchThreads();
//		}
//		usleep(SECOND);
//	}
//}
//
//void g(void)
//{
//	int i = 0;
//	while (1){
//		++i;
//		printf("in g (%d)\n", i);
//		if (i % 5 == 0) {
//			printf("g: switching\n");
//			switchThreads();
//		}
//		usleep(SECOND);
//	}
//}
//
//
//
//
//void setup()
//{
//	address_t sp, pc;
//
//	sp = (address_t)stack1 + STACK_SIZE - sizeof(address_t);
//	pc = (address_t)f;
//
//
//	sigsetjmp(jbuf[0], 1);
//	(jbuf[0]->__jmpbuf)[JB_SP] = translate_address(sp);
//	(jbuf[0]->__jmpbuf)[JB_PC] = translate_address(pc);
//	sigemptyset(&jbuf[0]->__saved_mask);
//
//
//	sp = (address_t)stack2 + STACK_SIZE - sizeof(address_t);
//	pc = (address_t)g;
//
//	sigsetjmp(jbuf[1], 1);
//	(jbuf[1]->__jmpbuf)[JB_SP] = translate_address(sp);
//	(jbuf[1]->__jmpbuf)[JB_PC] = translate_address(pc);
//	sigemptyset(&jbuf[1]->__saved_mask);
//}
//
//
//void switchThreads()
//{
//	static int currentThread = 0;
//
//	int ret_val = sigsetjmp(jbuf[currentThread], 1);
//	printf("SWITCH: ret_val=%d\n", ret_val);
//	if (ret_val == 1) {
//		return;
//	}
//
//	currentThread = 1 - currentThread;
//	siglongjmp(jbuf[currentThread], 1);
//}
//
//
//int main()
//{
//	setup();
//	siglongjmp(jbuf[0], 1);
//	return 0;
//}
//
//
//
//
//













//#include <stdio.h>
//#include <stdlib.h>
//#include <setjmp.h>
//#include <assert.h>
//#include "coroutine.h"
//
//// 默认容量
//#define DEFAULT_CAP   8
//
//typedef struct schedule schedule;
//typedef struct coroutine coroutine;
//typedef struct coroutine_para coroutine_para;
//
//struct schedule
//{
//	int  cap;     // 容量
//	int  conums;
//	int  curID;   // 当前协程ID
//	jmp_buf main;
//	coroutine **co;
//};
//
//struct coroutine
//{
//	int     status;
//	jmp_buf ctx;
//	coroutine_func func;
//};
//
//struct coroutine_para
//{
//	schedule *s;
//	void     *ud;
//	coroutine_func func;
//};
//
//static int put_co_in(schedule *s, coroutine *co)
//{
//	if (s->conums >= s->cap)
//	{
//		int id = s->cap;
//		s->co = realloc(co, sizeof(coroutine *)* s->cap * 2);
//		memset(s->co + s->cap, 0, s->cap);
//		s->co[s->cap] = co;
//		s->cap *= 2;
//		return id;
//	}
//	else
//	{
//		for (int i = 0; i < s->cap; i++)
//		{
//			int id = (i + s->conums) % s->cap;
//			if (s->co[id] == NULL)
//			{
//				s->co[id] = co;
//				++s->conums;
//				return id;
//			}
//		}
//	}
//	assert(0);
//	return -1;
//}
//
//schedule *coroutine_open()
//{
//	schedule *s = malloc(sizeof(schedule));
//	s->cap = DEFAULT_CAP;
//	s->conums = 0;
//	s->curID = -1;
//	s->co = malloc(sizeof(coroutine *) * s->cap);
//	memset(s->co, 0, sizeof(coroutine *) * s->cap);
//	if (setjmp(s->main)) return NULL;
//	return s;
//}
//
//void coroutine_close(schedule *s)
//{
//	free(s);
//}
//
//void __stdcall coroutine_main(void *lpParameter)
//{
//	coroutine_para* para = (coroutine_para*)lpParameter;
//	schedule * s = para->s;
//
//	(para->func)(para->s, para->ud);
//
//	int id = s->curID;
//	s->curID = -1;
//	--s->conums;
//	free(s->co[id]);
//	s->co[id] = NULL;
//	free(para);
//
//	longjmp(s->main, 1);
//}
//
//int coroutine_new(schedule *s, coroutine_func *func, void *ud)
//{
//	coroutine *co = malloc(sizeof(coroutine));
//	co->status = COROUTINE_READY;
//	co->func = func;
//	int id = put_co_in(s, co);
//
//	coroutine_para *para = malloc(sizeof(coroutine_para));
//	para->s = s;
//	para->func = func;
//	para->ud = ud;
//
//	if (setjmp(co->jmp_buf)) return -1;
//	co->status = COROUTINE_READY;
//
//	return id;
//}
//
//void coroutine_resume(schedule *s, int id)
//{
//	assert(id >= 0 && id < s->cap);
//	coroutine *co = s->co[id];
//	if (co == NULL) return;
//
//	co->status = COROUTINE_RUNNING;
//	s->curID = id;
//	SwitchToFiber(co->ctx);
//}
//
//void coroutine_yield(schedule *s)
//{
//	int id = s->curID;
//	assert(id >= 0 && id < s->cap);
//	if (id < 0) return;
//
//	coroutine *co = s->co[id];
//	co->status = COROUTINE_SUSPEND;
//	s->curID = -1;
//	SwitchToFiber(s->main);
//}
//
//int coroutine_status(schedule *s, int id)
//{
//	assert(id >= 0 && id < s->cap);
//	if (s->co[id] == NULL) {
//		return COROUTINE_DEAD;
//	}
//	return s->co[id]->status;
//}
//
//int coroutine_running(schedule *s)
//{
//	return s->curID;
//}

int main()
{
	return 0;
}