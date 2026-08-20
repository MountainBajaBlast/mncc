CC := clang
CFLAGS := -Wall -g
SRCS := main.c lexer/lexer.c parser/parser.c parser/ast.c semantic/semantic.c IR/ir.c optimizator/opt.c LIRA/lira.c codegen/codegen.c codegen/enc.c
TARGET := mncc

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

asan: $(SRCS)
	$(CC) $(CFLAGS) -fsanitize=address $(SRCS) -o $(TARGET)_asan

clean:
	rm -f $(TARGET) $(TARGET)_asan

.PHONY: all asan clean
