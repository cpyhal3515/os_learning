#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h> 
#include <regex.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>

// 输出前 5 个耗时百分比
#define MAX_PRINT_CMD 5
// 定义 read 的缓冲区的大小
#define MAX_LINE 4096

// 最多存储的命令数量
#define MAX_CMD_NUM 100
// 最长命令名字的字符长度
#define MAX_NAME_LEN 20
// 命令存储索引
int command_index = 0;

// 存储命令结构体，包括命令名称 name 以及命令运行的时间 time
struct command_info
{
    char   name[MAX_NAME_LEN];
    double time;
} command_info_vec[MAX_CMD_NUM];
// 命令结构体的比较方式，qsort 中会用到
int compare(const void *a, const void *b) 
{
    struct command_info *c1 = (struct command_info *)a;
    struct command_info *c2 = (struct command_info *)b;
    if(c1->time < c2->time)
        return 1;
    else if(c1->time > c2->time)
        return -1;
    else
        return 0;
}

// 正则表达式匹配方式
int pick_regex(const char* string,const char* pattern);
void display();


int main(int argc, char *argv[]) {
    // 定义正则表达式的格式
    const char *pattern = "([a-zA-Z_][a-zA-Z0-9_]*)[^<]*<([^>]*)>";

    int size = argc + 2;
    // 设定参数以及环境变量
    char** exec_argv = malloc(size * sizeof(char*));
    char*  exec_envp[] = { "PATH=/bin:/usr/bin:/usr/local/bin:.", NULL, };

    pid_t pid;
    int fildes[2];
    int n;

    int status;

    // 根据 main 输入构建 exec_argv
    exec_argv[0] = "strace";
    exec_argv[1] = "-T";
    for(int i = 2; i < argc + 1; ++i) 
    {
        exec_argv[i] = argv[i-1];
    }
    // 添加NULL结尾
    exec_argv[argc + 1] = NULL;

    // 建立管道
    if (pipe(fildes) != 0) 
    {
        // 出错处理
        perror("pipe error!");
    }
    // 创建子进程
    pid = fork();
    if(pid < 0)
    {
        perror("fork error!");
    }
    // 对于子进程
    else if (pid == 0) 
    {
        #ifdef DEBUG
            printf("Child Origin:fildes[0] = %d, fildes[1] = %d\n", fildes[0], fildes[1]);
        #endif
        // 关闭子进程的读部分
        close(fildes[0]);
        // 将 strace 的输出重定向到子进程的写部分
        dup2(fildes[1], STDERR_FILENO);
        // 丢弃 strace 调用的函数本身的输出
        int nullfd = open("/dev/null", O_WRONLY); 
        dup2(nullfd, STDOUT_FILENO);
        #ifdef DEBUG
            printf("Child:fildes[0] = %d, fildes[1] = %d\n", fildes[0], fildes[1]);
        #endif
        // 子进程，执行strace命令
        execve("/bin/strace", exec_argv, exec_envp);

    } 
    // 对于父进程
    else 
    {
        #ifdef DEBUG
            printf("Parent:fildes[0] = %d, fildes[1] = %d\n", fildes[0], fildes[1]);
        #endif
        // 关闭父进程的写部分
        close(fildes[1]);

        char buf[MAX_LINE];
        dup2(fildes[0], STDIN_FILENO);

        // 计时实现对较长时间的 IO 操作的计时输出
        time_t begin, end;
        begin = time(NULL);
        while(fgets(buf, MAX_LINE, stdin) != NULL)
        {
            pick_regex(buf, pattern);

            end = time(NULL);
            if((end - begin) > 1)
            {
                display();
                begin = time(NULL);
            }

        }
        display();
    }

    free(exec_argv);

    return 0;
}


int pick_regex(const char* string,const char* pattern)
{

    regex_t regex;
    regmatch_t matches[3];

    // 构建正则表达式
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) 
    {
        printf("Failed to compile regex pattern\n");
        return -1;
    }
    // 进行正则匹配
    if (regexec(&regex, string, 3, matches, 0) == 0) {
        char function_name[100];
        char time_value[100];
        // 将匹配到的字符串拷贝出来
        strncpy(function_name, string + matches[1].rm_so, matches[1].rm_eo - matches[1].rm_so);
        strncpy(time_value, string + matches[2].rm_so, matches[2].rm_eo - matches[2].rm_so);

        function_name[matches[1].rm_eo - matches[1].rm_so] = '\0';
        time_value[matches[2].rm_eo - matches[2].rm_so] = '\0';

        // 匹配结果存储在结构体中，如果之前存过该 key，就增加 value
        int find_flag = 0;
        for(int idx = 0; idx < command_index; ++idx)
        {
            if(strcmp(command_info_vec[idx].name, function_name) == 0)
            {
                command_info_vec[idx].time += strtod(time_value, NULL);
                find_flag = 1;
            }
        }

        if(find_flag == 0)
        {
            strcpy(command_info_vec[command_index].name, function_name);
            command_info_vec[command_index].time = strtod(time_value, NULL);
            ++command_index;
        }

    }


    regfree(&regex);
    return 0;

}

void display()
{
    // 对不同命令的耗时进行排序
    qsort(command_info_vec, command_index, sizeof(struct command_info), compare);
    // 计算总的耗时时间
    double total_time_cost = 0.0;
    printf("/*-------- Command Time Cost --------*/\n");
    for(int i = 0; i < command_index; ++i)
    {
        total_time_cost += command_info_vec[i].time;
    }
    for(int i = 0; i < MAX_PRINT_CMD; ++i)
    {
        printf("%d:%s %3.1f%%\n", i, command_info_vec[i].name, 100 * command_info_vec[i].time/total_time_cost);
    }
    return;
}