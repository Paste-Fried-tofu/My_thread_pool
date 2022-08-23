#ifndef __THREADS_POOL_H__
#define __THREADS_POOL_H__

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 任务结构体
typedef struct Task
{
    void (*function)(void *arg);
    void *arg;
} Task;

// 任务队列结构体
typedef struct Task_Queue
{
    Task *taskQ;
    int task_capacity;
    int task_Size;
    int front;
    int rear;

    pthread_cond_t notFull;  // 任务队列是不是满了
    pthread_cond_t notEmpty; //任务队列是不是空了
} Task_Queue;

// 线程池结构体
typedef struct Threads_Pool
{
    Task_Queue tasks; //任务队列

    pthread_t manager_ID; // 管理者线程ID
    pthread_t *worker_ID; // 工作线程ID
    int min_NUM;          // 最小线程数
    int max_NUM;          // 最大线程数
    int busy_NUM;         // 忙线程数
    int live_NUM;         // 存活线程数
    int kill_NUM;         // 要杀死的线程数

    pthread_mutex_t mutex_pool;     // 锁整个线程池
    pthread_mutex_t mutex_busy_NUM; // 锁busy_NUM

    int shutdown; //是否要销毁线程池，1销毁，0不销毁
} Threads_Pool;

// 创建线程池并初始化
Threads_Pool *thread_Pool_Create(
    int threads_min_num,
    int threads_max_num,
    int tasks_capacity);
// 销毁线程池
int thread_Pool_Destroy(Threads_Pool *);
// 给线程池添加任务
void thread_Pool_addTask(Threads_Pool *, void (*function)(void *), void *arg);
// 获取线程池中工作线程的个数
int get_Work_Num(Threads_Pool *);
// 获取活着的线程个数
int get_Live_Num(Threads_Pool *);

/**********************/

// 工作线程任务
void *worker(void *arg);
// 管理者线程任务
void *manager(void *arg);
// 单个线程推出
void thread_Exit(Threads_Pool *);
#endif