# 编译器
CXX = g++
# 编译选项
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

# 目标文件
TARGET = main
OBJS = main.o

# 默认目标
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET)

# 生成 .o 文件规则
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理
clean:
	rm -f $(OBJS) $(TARGET)
	rm -f *.o