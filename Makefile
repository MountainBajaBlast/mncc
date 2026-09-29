CC = clang
CFLAGS = -Wall -Wextra -Ilexer -Iparser -Isemantic -IIR -Ioptimizator -ILIRA -Icodegen -Icodegen/arm64

SRCS = main.c \
	lexer/lexer.c \
	parser/parser.c parser/parse_stmt.c parser/parse_expr.c parser/parse_branch.c parser/ast.c \
	semantic/semantic.c semantic/sem_checks_func.c semantic/sem_func_table.c semantic/sem_hash_func.c semantic/sem_sym_table.c \
	IR/ir.c IR/ir_graph.c IR/ir_basic_block.c IR/ir_operand_create.c IR/ir_phi.c IR/ir_quadriple_create.c IR/ir_reg.c IR/ir_ssa.c IR/ir_verman.c IR/dominator_ir.c \
	optimizator/opt.c \
	LIRA/lira.c LIRA/livenessanalysis.c LIRA/bit_vector_allocate.c LIRA/lira_intervals.c \
	codegen/enc.c codegen/emit.c codegen/instruction.c codegen/object.c \
	codegen/arm64/enc-func-arm64.c codegen/arm64/mach.c

OBJS = $(SRCS:.c=.o)

mncc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f mncc $(OBJS)