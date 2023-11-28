#ifndef CO_H
#define CO_H

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>

// 设定每个协程对应堆栈的最大深度
#define KB *1024
#define STACK_SIZE (64 KB)
// 同时运行的协程的最大数量
#define CO_NUM_MAX 128

/*----------- 函数声明 -----------*/
struct co* co_start(const char *name, void (*func)(void *), void *arg);
void co_yield();
void co_wait(struct co *co);

/*----------- 设定状态变量 -----------*/
enum co_status 
{
  CO_NEW = 1, // 新创建，还未执行过
  CO_RUNNING, // 已经执行过
  CO_WAITING, // 在 co_wait 上等待
  CO_DEAD,    // 已经结束，但还未释放资源
};

/*----------- 设定协程的结构体 -----------*/
struct co 
{
    const char *name;
    void (*func)(void *);   // co_start 指定的⼊口地址
    void *arg;              // co_start 指定的参数

    void *stackptr;         // 协程堆栈的指针
    enum co_status status;  // 协程的状态
    jmp_buf context;        // 寄存器现场 (setjmp.h)
    struct co *waiter;      // 等待该协程的另一个协程
    uint8_t stack[STACK_SIZE] __attribute__ ((aligned(16)));  // 协程的堆栈
};

/*----------- 初始化全局变量 -----------*/
// 协程数量计数器
int co_num = 0;
// 当前协程的指针
struct co *current;
// 存储协程指针的数组
struct co *co_list[CO_NUM_MAX];


#endif
