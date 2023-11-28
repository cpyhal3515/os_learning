# 设置名称
NAME := libco
ifeq ($(NAME),)
$(error Should make in each lab's directory)
endif

# 用于获取当前目录中的所有源代码文件
SRCS   := $(shell find . -maxdepth 1 -name "*.c")
# 用于获取当前目录中的所有头文件
DEPS   := $(shell find . -maxdepth 1 -name "*.h") $(SRCS)
# CFLAGS 指定编译器的选项
# CFLAGS += -O1 -std=gnu11 -ggdb -Wall -Werror -Wno-unused-result -Wno-unused-value -Wno-unused-variable -U_FORTIFY_SOURCE
# 调试的时候不要打开编译器优化选项
CFLAGS += -O0 -std=gnu11 -ggdb -Wall -Werror -Wno-unused-result -Wno-unused-value -Wno-unused-variable

# 声明一些伪目标
.PHONY: all test clean reg_shift gdb_test

# 目标规则：表示 all 依赖于 $(NAME)-64.so 这个目标，在执行目标 all 时，将构建 $(NAME)-64.so 。
all: $(NAME)-64.so

# 生成 64bit 共享库
$(NAME)-64.so: $(DEPS) 
	gcc -fPIC -shared -m64 $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)

# test 测试 main 中的示例
libco-test-64: ./tests/main.c
	gcc -I. -L. -m64 $(CFLAGS) ./tests/main.c -o libco-test-64 -lco-64

test: libco-test-64
	@echo "==== TEST 64 bit ===="
	@LD_LIBRARY_PATH=. ./libco-test-64

# test1 测试 main1 中的示例
libco-test1-64: ./tests/main1.c
	gcc $(CFLAGS) -I. -L. -m64 ./tests/main1.c -o libco-test1-64 -lco-64

test1: libco-test1-64
	@echo "==== TEST1 64 bit ===="
	@LD_LIBRARY_PATH=. ./libco-test1-64

gdb_test1: libco-test1-64
	@LD_LIBRARY_PATH=. gdb ./libco-test1-64


clean:
	rm -f $(NAME)-64.so libco-test* 