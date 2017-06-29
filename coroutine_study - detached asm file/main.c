#include <stdio.h>
#include <stdlib.h>
#include "coroutine.h"


const int loop_num = 3;
void test1(schedule *s, void *ud)
{
	int *data = (int*)ud;
	for (int i = 0; i < loop_num; i++)
	{
		printf("test3: will yield co data = %d, i=%d\n",*data,i);
		coroutine_yield(s);
	}
}
void test2(schedule *s, void *ud)
{
	int *data = (int*)ud;
	for (int i = 0; i < 1; i++)
	{
		printf("test3: will yield co data = %d, i=%d\n", *data, i);
		coroutine_yield(s);
	}
}

void coroutine_test()
{
	printf("coroutine_test3 begin\n");
	schedule *s = coroutine_open();


	/*********************test 1************************/
	printf("\n\n\n/*********************test 1************************/\n");
	int a = 11;
	int id1 = coroutine_new(s, (coroutine_func)test1, &a);
	int id2 = coroutine_new(s, (coroutine_func)test1, &a);
	int num = 0;

	while (coroutine_status(s, id1) && coroutine_status(s, id2))
	{
		printf("\nresume co id = ¡¾ %d ¡¿. num =  ¡¾ %d ¡¿.\n",id1, num);
		coroutine_resume(s, id1);
		printf("resume co id = ¡¾ %d ¡¿. num =  ¡¾ %d ¡¿.\n", id2, num);
		coroutine_resume(s, id2);
		num++;
	}

	/*********************test 2************************/
	printf("\n\n\n/*********************test 2************************/\n");
	num = 0;
	int id3 = coroutine_new(s, (coroutine_func)test1, &a);
	while (coroutine_status(s, id3))
	{
		printf("\nresume co id = ¡¾ %d ¡¿. num =  ¡¾ %d ¡¿.\n", id3, num);
		coroutine_resume(s, id3);
		num++;
	}

	/*********************test 3************************/
	printf("\n\n\n/*********************test 3************************/\n");
	num = 0;
	int id4 = coroutine_new(s, (coroutine_func)test1, &a);
	int id5 = coroutine_new(s, (coroutine_func)test2, &a);
	while (coroutine_status(s, id4))
	{
		printf("\nresume co id = ¡¾ %d ¡¿. num =  ¡¾ %d ¡¿.\n", id3, num);
		coroutine_resume(s, id4);
		if (coroutine_status(s, id5))
			coroutine_resume(s, id5);
		num++;
	}


	printf("coroutine_test3 end\n");
	coroutine_close(s);
}


int main()
{
	//coroutine_switch();
	//coroutine_setjmp();
	coroutine_test();

	system("pause");
	return 0;
}
