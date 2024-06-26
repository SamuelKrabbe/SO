#include <stdio.h>
#include <unistd.h>
#include <threadu.h>

#include <util.h>

void *f1(void *p)
{
	int i;
	int *one = (int *)p;
	
	for (i = 0; i < 3; i++) {
		printf("\n");
		printf("iteration %d\n", i);
		printf("Thread with param %d will sleep!\n", *one);
		sleep(1);
		printf("Thread with param %d will yield!\n", *one);
		printf("\n");
		thread_yield();
	}
	thread_exit(*one);
	printf("f1\n");
}

int main()
{
	thread_t thd[6];
	int params[6], rv[6], i;
	
	thread_init();
	printf("BEGIN: Time stamp: %ld\n", get_timer());
	for (i = 0; i < 6; i++) {
		params[i] = i;
		thread_create(&thd[i], f1, &params[i]);
	}

	for (i = 0; i < 6; i++) {
		thread_join(&thd[i], &rv[i]);
		printf("Thread %d finished with exit status %d\n", i, rv[i]);
	}
	printf("END: Time stamp: %ld\n", get_timer());
}
