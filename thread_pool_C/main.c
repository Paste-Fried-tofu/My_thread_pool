#include "threads_pool.h"

void taskFunc(void *arg)
{
    int num = *(int *)arg;
    printf("thread %ld is working, number = %d\n",
           pthread_self(), num);
    sleep(1);
}

int main()
{
    Threads_Pool *pool = thread_Pool_Create(3, 10, 100);
    for (int i = 0; i < 100; ++i)
    {
        int *num = (int *)malloc(sizeof(int));
        *num = i + 100;
        thread_Pool_addTask(pool, taskFunc, num);
    }

    sleep(3);

    thread_Pool_Destroy(pool);
    return 0;
}
