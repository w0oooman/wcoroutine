//#include "switch.h"
//
//typedef struct
//{
//	int i;
//	int num;
//	int state;
//} task;
//
//int cb(task *t)
//{
//	switch (t->state) {
//	case 0:
//		for (;;) {
//			t->num = 1;
//			for (t->i = 0; t->i < 20; t->i++) {
//				t->state = 10;
//				return t->num;
//
//	case 10:
//		t->num += 1;
//			}
//		}
//	}
//}
//
//void coroutine_switch()
//{
//	task t;
//
//	t.state = 0;
//
//	for (int i = 0; i < 100; i++) {
//		printf("%d ", cb(&t));
//	}
//}

int main()
{
	return 0;
}