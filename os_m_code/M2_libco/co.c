#include "co.h"

/*
* stack_switch_call - 完成函数的调用。
* @param1 void *sp - 堆栈指针
* @param2 void *entry - 函数的入口地址
* @param3 uintptr_t arg - 传递给函数的参数
* @return
*/
static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg) 
{
  asm volatile (
    #if __x86_64__
        "movq %0, %%rsp; movq %2, %%rdi; jmp *%1"
          : : "b"((uintptr_t)sp), "d"(entry), "a"(arg) : "memory"
    #else
        "movl %0, %%esp; movl %2, 4(%0); jmp *%1"
          : : "b"((uintptr_t)sp - 8), "d"(entry), "a"(arg) : "memory"
    #endif
  );
}

/*
* co_init - 在进入 main 函数前运行，初始化 main 协程，并将 main 协程加入协程列表中。
* @param1
* @return
*/
__attribute__((constructor)) void co_init() 
{
  struct co *co_ptr = malloc(sizeof(struct co));
  co_ptr->name = "main";
  // 堆栈指针要从栈底开始生长
  co_ptr->stackptr = co_ptr->stack + sizeof(co_ptr->stack);
  co_ptr->status = CO_RUNNING;
  memset(co_ptr->stack, 0, sizeof(co_ptr->stack));
  co_list[co_num] = co_ptr;
  current = co_ptr;
  co_num += 1;
}

/*
* co_start - 完成协程的初始化，并将初始化后的协程加入协程列表中。
* @param1 const char *name - 协程的名称。
* @param2 void (*func)(void *) - 协程调用的函数。
* @param3 void *arg - 给该函数传入的参数。
* @return struct co* - 返回创建的协程。
*/
struct co* co_start(const char *name, void (*func)(void *), void *arg)
{
    struct co *co_ptr = malloc(sizeof(struct co));
    co_ptr->name = name;
    co_ptr->func = func;
    co_ptr->arg = arg;
    co_ptr->status = CO_NEW;
    co_ptr->stackptr = co_ptr->stack + sizeof(co_ptr->stack);
    memset(co_ptr->stack, 0, sizeof(co_ptr->stack));
    co_list[co_num] = co_ptr;
    co_num += 1;
    return co_ptr;
};

/*
* wrapper - 完成新调用的函数的处理。
* @param1
* @return
*/
void wrapper()
{
    current->status = CO_RUNNING;
    current->func(current->arg);
    current->status = CO_DEAD;
    if(current->waiter != NULL)
    {
        current->waiter->status = CO_RUNNING;
        current->waiter = NULL;
    }
    co_yield();
 }

/*
* co_yield - 完成协程的切换。
* @param1
* @return
*/
void co_yield()
{
    int val = setjmp(current->context);
    if(val == 0)
    {
        // 随机选取一个状态是 CO_NEW 或者 CO_RUNNING 的协程
        int rand_co_index = 0; 
        do 
        {
            rand_co_index = rand() % co_num;
        }
        while (co_list[rand_co_index]->status == CO_WAITING || co_list[rand_co_index]->status == CO_DEAD);
        current = co_list[rand_co_index];

        // CO_NEW 就开始运行
        if(current->status == CO_NEW)
            stack_switch_call(current->stackptr, wrapper, (uintptr_t)NULL);
        // CO_RUNNING 就回复切换前的状态
        else
            longjmp(current->context, 1);
    }
};

/*
* co_wait - 完成协程的等待处理。
* @param1 struct co *co - 需要等待处理的协程。
* @return
*/
void co_wait(struct co *co)
{
    // 协程结束进行回收
    if(co->status == CO_DEAD)
    {
        int del_idx = 0;
        for(del_idx = 0; del_idx < co_num; del_idx++)
        {
            if(co_list[del_idx] == co)
                break;
        }

        co_num -= 1;
        
        for(int idx = del_idx; idx < co_num; idx++)
        {
            co_list[idx] = co_list[idx+1];
        }
        free(co);
    }
    // 运行当前协程
    else
    {
        co->waiter = current;
        current->status = CO_WAITING;
        co_yield();
    }
};

