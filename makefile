_VERSION = sensor_bulider V1.0.0
# 指定编译器
CC=gcc

SHELL=powershell.exe
# 指定编写线程数量
THREADS = 12

# 指定头文件目录及进行字符串替换
IDIR=./include
CFLAGS=-I$(IDIR)

_DEPS = $(notdir $(wildcard $(IDIR)/*.h))
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

# 指定源文件目录
SDIR=./src

# 指定目标文件目录及其替换字符串
ODIR=obj
_OBJ = $(patsubst %.c,%.o,$(notdir $(wildcard $(SDIR)/*.c)))
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

# 隐式规则 自动构建
$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
# -o $@选项来指定输出文件的名称，$< $(CFLAGS)来指定输入文件的名称和编译选项。
	$(CC) -c -o $@ $< $(CFLAGS)

# 链接
test: $(OBJ)
	$(info $(_VERSION))
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
# 该命令删除尽在Windows下生效
	rm -r -fo $(ODIR)/*.o

print:
	$(info $(_DEPS))