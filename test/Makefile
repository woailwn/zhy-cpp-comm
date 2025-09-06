# 定义编译器和编译选项
CXX = g++
CXXFLAGS = -g -Wall -I../include

# 获取构建根目录
BUILD_ROOT = $(shell cd ../ && pwd)

# 源文件和目标文件
SRCS = test_conf.cpp ../app/zhy_conf.cpp ../app/zhy_func.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = test_conf

# 主目标
all: $(TARGET)

# 链接目标文件
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -std=c++11

# 编译源文件为目标文件
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ -std=c++11

# 运行测试
run:
	./$(TARGET)

# 使用gdb调试
debug:
	gdb ./$(TARGET)

# 清理生成的文件
clean:
	rm -f $(OBJS) $(TARGET)

# 伪目标
.PHONY: all clean run debug