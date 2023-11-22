# 定义生成可执行文件的名称
export NAME=pstree_user

# ----- 只用于本地 64 位系统的 Makefile -----

ifeq ($(NAME),)
$(error Should make in each lab's directory)
endif

# SRCS   := $(shell find . -maxdepth 1 -name "*.c")
SRCS   := $(shell find . -maxdepth 1 -name "*.cpp")
DEPS   := $(shell find . -maxdepth 1 -name "*.h") $(SRCS)
# CFLAGS += -O1 -std=gnu11 -ggdb -Wall -Werror -Wno-unused-result -Wno-unused-value -Wno-unused-variable
# for gdb debug
# CFLAGS += -O0 -std=gnu11 -ggdb -Wall -Werror -Wno-unused-result -Wno-unused-value -Wno-unused-variable
CXXFLAGS += -O0 -std=c++11 -g -Wall -Werror -Wno-unused-result -Wno-unused-value -Wno-unused-variable

# $(NAME)-64: $(DEPS) # 64bit binary
# 	g++ -m64 $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)
 
# $(NAME)-64.so: $(DEPS) # 64bit shared library
# 	g++ -fPIC -shared -m64 $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)

$(NAME)-64: $(DEPS) # 64bit binary
	g++ -m64 $(CXXFLAGS) $(SRCS) -o $@ $(LDFLAGS)
 
$(NAME)-64.so: $(DEPS) # 64bit shared library
	g++ -fPIC -shared -m64 $(CXXFLAGS) $(SRCS) -o $@ $(LDFLAGS)

clean:
	rm -f $(NAME)-64 $(NAME)-64.so

test:
	./$(NAME)-64
