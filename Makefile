CC     := gcc
CFLAGS := -Wall -Wextra -g -std=gnu11 -Isrc -Iinclude -I. -MMD -MP
TARGET := ashx-bin
OBJS   := $(addprefix build/, alloc.o line.o builtins.o stb_ds.o lexer.o exec.o parser.o main.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $^ -o $@

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build $(TARGET)

-include $(OBJS:.o=.d)

.PHONY: all clean
