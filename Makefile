CC := cc
CFLAGS := -Wall -g

OS := $(shell uname -s)
ARCH := $(shell uname -m)

ifeq ($(OS),Darwin)
	CC := clang
endif

ifeq ($(ARCH),x86_64)
	LIRA_BACKEND := LIRA/lira-GNU-Linux-x64.c
	CODEGEN_BACKEND := codegen/codegen-GNU-Linux-x64.c codegen/encode_one_functions-GNU-Linux-x64.c codegen/elf-GNU-Linux-x64.c codegen/enc-GNU-Linux-x64.c
        DRIVER_OF_COMPILER := main-GNU-Linux-x64.c
else
	LIRA_BACKEND := LIRA/lira.c
	CODEGEN_BACKEND := codegen/codegen.c codegen/enc.c codegen/encode_one_functions.c codegen/mach.c
        DRIVER_OF_COMPILER := main.c
endif

COMMON := lexer/lexer.c \
	parser/parser.c parser/ast.c parser/parse_expr.c parser/parse_stmt.c parser/parse_branch.c \
	semantic/semantic.c semantic/sem_checks_func.c semantic/sem_sym_table.c semantic/sem_hash_func.c \
	IR/ir.c IR/ir_ssa.c IR/ir_verman.c IR/dominator_ir.c IR/ir_quadriple_create.c IR/ir_operand_create.c IR/ir_phi.c IR/ir_graph.c IR/ir_basic_block.c IR/ir_reg.c \
	optimizator/opt.c \
	LIRA/lira_intervals.c LIRA/livenessanalysis.c LIRA/bit_vector_allocate.c

SRCS := $(DRIVER_OF_COMPILER) $(COMMON) $(LIRA_BACKEND) $(CODEGEN_BACKEND)
TARGET := mncc

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

asan: $(SRCS)
	$(CC) $(CFLAGS) -fsanitize=address $(SRCS) -o $(TARGET)_asan

clean:
	rm -f $(TARGET) $(TARGET)_asan

.PHONY: all asan clean
