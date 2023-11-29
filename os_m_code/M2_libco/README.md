# M1: 实现携程库 (libco)
## 实验要求


## 解决流程
### Step1：分析 Makefile 文件
> 这次的 Makefile 涉及到共享库的知识点，这里首先对 Makefile 文件进行合并以及分析。
* 共享库的编译：共享库中不需要入口 (main 函数)，在编译选项中的 `-fPIC -fshared` 代表编译成位置无关代码的共享库。除此之外，共享库和普通的二进制文件没有特别的区别。
    ```shell
    # 目标规则：表示 libco 依赖于 $(NAME)-64.so 这个目标。
    libco: $(NAME)-64.so

    # 生成 64bit 共享库
    $(NAME)-64.so: $(DEPS) 
        gcc -fPIC -shared -m64 $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)
    ```
* 用编译好的共享库：`-I` 选项代表 include path，这里使用 . 表示在当前目录下寻找 include 文件；`-L` 选项代表增加 link search path，这里使用 . 表示在当前目录下寻找链接文件；`-l` 选项代表链接某个库，链接时会自动加上 lib 的前缀，即 -lco-64 会依次在库函数的搜索路径中查找 libco-64.so 和 libco-64.a，直到找到为止。同时，这里的 `LD_LIBRARY_PATH` 环境变量给出了加载共享库链接可以查找的位置。
    ```shell
    libco-test-64: ./tests/main.c
        gcc -I. -L. -m64 ./tests/main.c -o libco-test-64 -lco-64

    test: libco-test-64
        @echo "==== TEST 64 bit ===="
        @LD_LIBRARY_PATH=. ./libco-test-64
    ```
### Step2：分析 setjmp 以及 longjmp 函数
* 之前并没有接触过这两个函数，通过 Google 找到一个资料：[CS360 Lecture notes -- Setjmp](https://web.eecs.utk.edu/~jplank/plank/classes/cs360/360/notes/Setjmp/lecture.html)

### Step3：分析 stack_switch_call 对应的代码
* 在编程实现指南中给出了如下所示的一个关键函数，我目前只关注 `__x86_64__` 部分的代码含义：
    ```c
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
    ```
* 代码的作用是切换堆栈并调用一个函数。首先，函数接受三个参数：`void *sp`、`void *entry` 和 `uintptr_t arg`。这些参数用于指定新的堆栈指针、函数入口地址和传递给函数的参数。接下来，代码使用了内联汇编语法，并使用 `asm volatile` 来嵌入汇编代码。`volatile` 关键字用于告诉编译器不要对这段汇编代码进行优化。
    * `movq %0, %%rsp`：表示将参数 sp 的值加载到 %rsp 寄存器中，即将堆栈指针指向新的堆栈，`rsp` 表示堆栈指针。
    * `movq %2, %%rdi`：将参数 arg 的值加载到 %rdi 寄存器中，作为传递给函数的第一个参数。
    * `jmp *%1`：跳转到 %1 寄存器指向的地址，即跳转到函数入口地址 entry。

### Step4: 综合应用 setjmp/longjmp 以及 stack_switch_call
* 下面给出了一小段代码，用来把 `setjmp/longjmp` 这两个函数以及 `stack_switch_call` 这个函数用起来。
    ```c
    jmp_buf env;
    void hello_world(uintptr_t arg) {
        printf("Hello, World! arg = %lu\n", arg);
        longjmp(env, 1);
    }

    int main() {
        __attribute__((aligned(16))) uint8_t  stack[100]; // 协程的堆栈
        if(!setjmp(env))
            stack_switch_call(stack, hello_world, (uintptr_t)123);
        else
        {
            printf("Hello world 结束！\n");
        }


        return 0;
    }
    ```
* 实现的功能就是调用一次 `hello_world` 函数，运行这个代码可以得到如下的结果：
    ```c
    Hello, World! arg = 123
    Hello world 结束！
    ```
* 如果将上面的代码进行修改，去掉 `setjmp/longjmp` 部分，改成如下所示的代码，就会产生 Segmentation fault 的错误。
    ```c
    jmp_buf env;
    void hello_world(uintptr_t arg) {
        printf("Hello, World! arg = %lu\n", arg);
        return;
    }

    int main() {
        __attribute__((aligned(16))) uint8_t  stack[100]; // 协程的堆栈
        stack_switch_call(stack, hello_world, (uintptr_t)123);
        return 0;
    }
    ```
* 使用 gdb 调试可以得到如下的问题：

    ```c
    (gdb) 
    0x0000000000000001 in ?? ()
    (gdb) 
    Cannot find bounds of current function
    Program received signal SIGSEGV, Segmentation fault.
    0x0000000000000001 in ?? ()
    ```
    也就是说在函数返回的过程中找不到堆栈了，这也就是引入 `setjmp/longjmp` 的原因，因为函数不能直接返回。
### Step5: 代码分析
#### 1、 main 是一个协程
* main 是一个协程，因此在真正开始执行 main 函数之前需要将 main 这个协程初始化，同时设置 main 函数的运行状态为 `CO_RUNNING` ，将 main 函数添加到协程列表 `co_list` 中，并设置当前协程 `current` 为 main 函数：
    ```c
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
    ```
* 这里用到 `__attribute__((constructor))` 函数可以在进入 main 函数之前执行一些代码。
#### 2、 协程的初始化
* 协程的初始化类似上面对 main 的操作，只是状态需要设置成为 `CO_NEW`。
    ```c
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
    ```
#### 3、 协程的等待处理
* 协程的等待处理部分要实现两个功能，一个是当协程的状态为 `CO_DEAD` 的时候需要对协程资源进行回收，另一个是将当前协程 `current` 暂停并放在函数输入协程 `co` 的等待序列 `waiter` 中，并使用 `co_yield()` 进行协程的切换。
    ```c
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
    ```
#### 4、 协程的切换
* 进入到 `co_yield` 后首先需要保存当前程序的状态，之后从协程列表中随机选取一个状态是 `CO_NEW` 或者 `CO_RUNNING` 的协程：
    * 如果状态是 `CO_NEW` 说明这个协程对应的函数还未开始执行，因此使用 `stack_switch_call` 开始执行 `current->func`，当函数执行完毕后需要修改对应的状态，并将 `current->waiter` 中的函数唤醒。
    * 如果状态是 `CO_RUNNING` 说明这个协程对应的函数正在被执行，那使用 `longjmp(current->context, 1)` 继续执行就可以了。
    ```c
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

    ```
#### 5、 总结
* 实际上我感觉这个协程的题目很好的体现了程序是个状态机这个思想，当创建新的协程之后并不马上执行它，而是将这个协程对应的状态置为 NEW，并放在协程列表中。当进入等待序列中后，将当前协程的状态置为 WAITING，给与新协程执行机会。当选中某个协程之后，将状态置为 RUNNING，并执行这个协程，如果函数执行结束，正常退出就将协程的状态置为 DEAD 方便在之后回收，如果函数在执行的过程中被切换出去，则这个状态没有发生变化，之后再切换回来之后可以继续执行。

## 资料
* [Understanding Concept of Shared Libraries](https://tbhaxor.com/understanding-concept-of-shared-libraries/)
* [GCC-Inline-Assembly-HOWTO](https://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html)
* [CS360 Lecture notes -- Setjmp](https://web.eecs.utk.edu/~jplank/plank/classes/cs360/360/notes/Setjmp/lecture.html)
* [CS356: A short guide to x86-64 assembly](https://hackmd.io/@paolieri/x86_64)

* [如何生成 core 文件](https://askubuntu.com/questions/1349047/where-do-i-find-core-dump-files-and-how-do-i-view-and-analyze-the-backtrace-st/1349048)

* [__x86_64__ 宏定义的位置](https://unix.stackexchange.com/questions/19633/which-header-defines-the-macro-that-specifies-the-machine-architecture)

* [__attribute__((constructor)) and __attribute__((destructor)) syntaxes in C](https://www.geeksforgeeks.org/__attribute__constructor-__attribute__destructor-syntaxes-c/)







