CC = clang
CFLAGS = -Wall -Wextra -Isrc/mncc -Isrc/mncc/lexer -Isrc/mncc/parser -Isrc/mncc/semantic -Isrc/mncc/IR -Isrc/mncc/optimizator -Isrc/mncc/LIRA -Isrc/mncc/codegen -Isrc/mncc/codegen/arm64

SRCS = src/main.c \
	src/mncc/lexer/lexer.c \
	src/mncc/parser/parser.c src/mncc/parser/parse_stmt.c src/mncc/parser/parse_expr.c src/mncc/parser/parse_branch.c src/mncc/parser/ast.c \
	src/mncc/semantic/semantic.c src/mncc/semantic/sem_checks_func.c src/mncc/semantic/sem_func_table.c src/mncc/semantic/sem_hash_func.c src/mncc/semantic/sem_sym_table.c \
	src/mncc/IR/ir.c src/mncc/IR/ir_graph.c src/mncc/IR/ir_basic_block.c src/mncc/IR/ir_operand_create.c src/mncc/IR/ir_phi.c src/mncc/IR/ir_quadriple_create.c src/mncc/IR/ir_reg.c src/mncc/IR/ir_ssa.c src/mncc/IR/ir_verman.c src/mncc/IR/dominator_ir.c \
	src/mncc/optimizator/opt.c \
	src/mncc/LIRA/lira.c src/mncc/LIRA/livenessanalysis.c src/mncc/LIRA/bit_vector_allocate.c src/mncc/LIRA/lira_intervals.c \
	src/mncc/codegen/enc.c src/mncc/codegen/emit.c src/mncc/codegen/instruction.c src/mncc/codegen/object.c \
	src/mncc/codegen/arm64/enc-func-arm64.c src/mncc/codegen/arm64/mach.c

OBJS = $(SRCS:.c=.o)

PREFIX ?= /usr/local/bin

mncc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

install: mncc
	install -m 755 mncc $(PREFIX)/mncc

uninstall:
	rm -f $(PREFIX)/mncc

clean:
	rm -f mncc $(OBJS)

.PHONY: clean install uninstall
