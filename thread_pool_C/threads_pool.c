#include "threads_pool.h"

const int NUM = 2;

Threads_Pool *thread_Pool_Create(
    int threads_min_num,
    int threads_max_num,
    int tasks_capacity)
{
    Threads_Pool *pool = (Threads_Pool *)malloc(sizeof(Threads_Pool));
    do
    {
        if (!pool)
        {
            printf("malloc threads pool failed......\n");
            break;
        }

        pool->worker_ID = (pthread_t *)malloc(sizeof(pthread_t) * threads_max_num);
        if (!pool->worker_ID)
        {
            printf("malloc worker ID failed......\n");
            break;
        }
        pool->max_NUM = threads_max_num;
        pool->min_NUM = threads_min_num;
        memset(pool->worker_ID, 0, sizeof(pthread_t) * pool->max_NUM);
        pool->busy_NUM = 0;
        pool->kill_NUM = 0;
        pool->live_NUM = pool->min_NUM;

        if (pthread_mutex_init(&pool->mutex_pool, NULL) != 0 ||
            pthread_mutex_init(&pool->mutex_busy_NUM, NULL) != 0 ||
            pthread_cond_init(&pool->tasks.notEmpty, NULL) != 0 ||
            pthread_cond_init(&pool->tasks.notFull, NULL) != 0)
        {
            printf("mutex or condition init failed.....\n");
            break;
        }

        pool->tasks.taskQ = (Task *)malloc(sizeof(Task) * tasks_capacity);
        if (!pool->tasks.taskQ)
        {
            printf("malloc task queue failed......\n");
            break;
        }

        pool->tasks.task_capacity = tasks_capacity;
        pool->tasks.front = 0;
        pool->tasks.rear = 0;
        pool->tasks.task_Size = 0;

        pool->shutdown = 0;

        pthread_create(&pool->manager_ID, NULL, manager, pool);
        for (int i = 0; i < pool->live_NUM; i++)
        {
            pthread_create(&pool->worker_ID[i], NULL, worker, pool);
        }

        return pool;
    } while (0);

    if (pool && pool->worker_ID)
        free(pool->worker_ID);

    if (pool && pool->tasks.taskQ)
        free(pool->tasks.taskQ);

    if (pool)
        free(pool);

    return NULL;
}

int thread_Pool_Destroy(Threads_Pool *pool)
{
    if (!pool)
    {
        return -1;
    }

    pool->shutdown = 1;

    pthread_join(pool->manager_ID, NULL);
    for (int i = 0; i < pool->live_NUM; i++)
    {
        pthread_cond_signal(&pool->tasks.notEmpty);
    }

    while (pool->live_NUM != 0)
    {
        sleep(1);
    }

    if (pool->tasks.taskQ)
        free(pool->tasks.taskQ);
    if (pool->worker_ID)
        free(pool->worker_ID);

    pthread_mutex_destroy(&pool->mutex_busy_NUM);
    pthread_mutex_destroy(&pool->mutex_pool);
    pthread_cond_destroy(&pool->tasks.notEmpty);
    pthread_cond_destroy(&pool->tasks.notFull);

    free(pool);
    return 0;
}

void thread_Pool_addTask(Threads_Pool *pool, void (*func)(void *), void *arg)
{
    pthread_mutex_lock(&pool->mutex_pool);
    while (pool->tasks.task_capacity == pool->tasks.task_Size && !pool->shutdown)
    {
        pthread_cond_wait(&pool->tasks.notFull, &pool->mutex_pool);
    }
    if (pool->shutdown)
    {
        pthread_mutex_unlock(&pool->mutex_pool);
        return;
    }

    pool->tasks.taskQ[pool->tasks.rear].function = func;
    pool->tasks.taskQ[pool->tasks.rear].arg = arg;
    pool->tasks.rear = (pool->tasks.rear + 1) % pool->tasks.task_capacity;
    pool->tasks.task_Size++;

    pthread_cond_signal(&pool->tasks.notEmpty);
    pthread_mutex_unlock(&pool->mutex_pool);
}

int get_Work_Num(Threads_Pool *pool)
{
    pthread_mutex_lock(&pool->mutex_busy_NUM);
    int temp = pool->busy_NUM;
    pthread_mutex_unlock(&pool->mutex_busy_NUM);
    return temp;
}

int get_Live_Num(Threads_Pool *pool)
{
    pthread_mutex_lock(&pool->mutex_pool);
    int temp = pool->live_NUM;
    pthread_mutex_unlock(&pool->mutex_pool);
    return temp;
}

void *worker(void *arg)
{
    Threads_Pool *pool = (Threads_Pool *)arg;
    while (1)
    {
        pthread_mutex_lock(&pool->mutex_pool);

        while (pool->tasks.task_Size == 0 && !pool->shutdown)
        {
            pthread_cond_wait(&pool->tasks.notEmpty, &pool->mutex_pool);

            if (pool->kill_NUM > 0)
            {
                pool->kill_NUM--;
                if (pool->live_NUM > pool->min_NUM)
                {
                    pthread_mutex_unlock(&pool->mutex_pool);
                    thread_Exit(pool);
                }
            }
        }

        if (pool->shutdown)
        {
            pthread_mutex_unlock(&pool->mutex_pool);
            thread_Exit(pool);
        }

        Task task;
        task.function = pool->tasks.taskQ[pool->tasks.front].function;
        task.arg = pool->tasks.taskQ[pool->tasks.front].arg;
        pool->tasks.front = (pool->tasks.front + 1) % pool->tasks.task_capacity;
        pool->tasks.task_Size--;

        pthread_cond_signal(&pool->tasks.notFull);
        pthread_mutex_unlock(&pool->mutex_pool);

        printf("thread %ld is start working......\n", pthread_self());

        pthread_mutex_lock(&pool->mutex_busy_NUM);
        pool->busy_NUM++;
        pthread_mutex_unlock(&pool->mutex_busy_NUM);

        task.function(task.arg);
        free(task.arg);

        printf("thread %ld is end working......\n", pthread_self());

        pthread_mutex_lock(&pool->mutex_busy_NUM);
        pool->busy_NUM--;
        pthread_mutex_unlock(&pool->mutex_busy_NUM);
    }
    return NULL;
}

void *manager(void *arg)
{
    Threads_Pool *pool = (Threads_Pool *)arg;
    while (!pool->shutdown)
    {
        sleep(3);

        pthread_mutex_lock(&pool->mutex_pool);
        int task_Size = pool->tasks.task_Size;
        int live_Num = pool->live_NUM;
        pthread_mutex_unlock(&pool->mutex_pool);

        pthread_mutex_lock(&pool->mutex_busy_NUM);
        int busy_Num = pool->busy_NUM;
        pthread_mutex_unlock(&pool->mutex_busy_NUM);

        if (task_Size > live_Num && live_Num < pool->max_NUM)
        {
            pthread_mutex_lock(&pool->mutex_pool);
            int counter = 0;
            for (int i = 0; i < pool->max_NUM && counter < NUM && pool->live_NUM < pool->max_NUM; ++i)
            {
                if (pool->worker_ID[i] == 0)
                {
                    pthread_create(&pool->worker_ID[i], NULL, worker, pool);
                    counter++;
                    pool->live_NUM++;
                }
            }
            pthread_mutex_unlock(&pool->mutex_pool);
        }

        if (busy_Num * 2 < live_Num && live_Num > pool->min_NUM)
        {
            pthread_mutex_lock(&pool->mutex_pool);
            pool->kill_NUM = NUM;
            pthread_mutex_unlock(&pool->mutex_pool);
            for (int i = 0; i < NUM; i++)
            {
                pthread_cond_signal(&pool->tasks.notEmpty);
            }
        }
    }
    return NULL;
}

void thread_Exit(Threads_Pool *pool)
{
    pthread_t id = pthread_self();
    for (int i = 0; i < pool->max_NUM; i++)
    {
        if (pool->worker_ID[i] == id)
        {
            pthread_mutex_lock(&pool->mutex_pool);
            pool->worker_ID[i] = 0;
            pool->live_NUM--;
            pthread_mutex_unlock(&pool->mutex_pool);

            printf("thread_Exit() called, %ld exit......\n", id);
            break;
        }
    }
    pthread_exit(NULL);
}
