# test目录的Makefile

# 设置构建根目录
BUILD_ROOT := $(shell cd ../ && pwd)

# 包含项目配置
include $(BUILD_ROOT)/config.mk
include $(BUILD_ROOT)/common.mk

# 目标可执行文件名
BIN = test_conf

# 源文件
SRCS = test_conf.cpp

# 目标文件
OBJS = $(SRCS:.cpp=.o)

# 依赖文件
DEPS = $(SRCS:.cpp=.d)

# 头文件路径
INCLUDE_PATH = $(INCLUDE_ROOT)

# 编译选项
CXXFLAGS = -std=c++11 -g -I$(INCLUDE_PATH)

# 链接选项
LDFLAGS = -lpthread

# 链接对象文件
LINK_OBJ = $(wildcard $(LINK_OBJ_DIR)/*.o)

all: $(DEPS) $(OBJS) $(BIN)

# 包含依赖文件
-include $(DEPS)

# 生成可执行文件
$(BIN): $(OBJS) $(LINK_OBJ)
	@echo "Building $@..."
	$(CC) -o $@ $^ $(LDFLAGS)

# 编译源文件
%.o: %.cpp
	@echo "Compiling $<..."
	$(CC) $(CXXFLAGS) -o $@ -c $<

# 生成依赖文件
%.d: %.cpp
	@echo "Generating dependencies for $<..."
	@g++ $(CXXFLAGS) -MM $< > $@.$$$$; \ 
	sed 's,\($*\)\.o[ :]*,$(LINK_OBJ_DIR)/\1.o $@ : ,g' < $@.$$$$ > $@; \ 
	rm -f $@.$$$$

# 运行程序
run: $(BIN)
	./$(BIN)

# 清理生成的文件
clean:
	@echo "Cleaning up..."
	rm -f $(OBJS) $(DEPS) $(BIN) *.d.*

# 声明伪目标
.PHONY: all clean run