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



## 资料
* [Understanding Concept of Shared Libraries](https://tbhaxor.com/understanding-concept-of-shared-libraries/)
* [GCC-Inline-Assembly-HOWTO](https://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html)
* [CS360 Lecture notes -- Setjmp](https://web.eecs.utk.edu/~jplank/plank/classes/cs360/360/notes/Setjmp/lecture.html)
* [CS356: A short guide to x86-64 assembly](https://hackmd.io/@paolieri/x86_64)

* [如何生成 core 文件](https://askubuntu.com/questions/1349047/where-do-i-find-core-dump-files-and-how-do-i-view-and-analyze-the-backtrace-st/1349048)

* [__x86_64__ 宏定义的位置](https://unix.stackexchange.com/questions/19633/which-header-defines-the-macro-that-specifies-the-machine-architecture)

* [__attribute__((constructor)) and __attribute__((destructor)) syntaxes in C](https://www.geeksforgeeks.org/__attribute__constructor-__attribute__destructor-syntaxes-c/)







