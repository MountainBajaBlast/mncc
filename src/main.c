#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantic/semantic.h"
#include "IR/ir.h"
#include "optimizator/opt.h"
#include "LIRA/lira.h"
#include "codegen/enc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Lexer *lexer;

static char *read_source(const char *path)
{
	FILE *file = fopen(path, "rb");
	if (!file) {
		fprintf(stderr, "Не удалось открыть %s\n", path);
		exit(1);
	}

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	char *source = malloc((size_t)size + 1);
	if (!source) {
		fprintf(stderr, "Ошибка памяти\n");
		exit(1);
	}

	size_t read = fread(source, 1, (size_t)size, file);
	fclose(file);
	source[read] = '\0';

	return source;
}

static void alloc_registers(IRGraph *graph)
{
	int total_quads = 0;
	for (BasicBlock *block = graph->head; block; block = block->next_block)
		for (Quadriple *quad = block->head; quad; quad = quad->next)
			total_quads++;

	QuadLiveness *quad = calloc((size_t)total_quads, sizeof(QuadLiveness));
	BlockLiveness *block_live = calloc((size_t)graph->block_count, sizeof(BlockLiveness));

	if (!quad || !block_live) {
		fprintf(stderr, "Ошибка памяти\n");
		exit(1);
	}

	for (int i = 0; i < total_quads; i++)
		init_quad_liveness(&quad[i], (size_t)graph->reg_count);
	for (int i = 0; i < graph->block_count; i++)
		init_block_liveness(&block_live[i], (size_t)graph->reg_count);

	compute_local_liveness(graph, quad, block_live);
	compute_global_liveness(graph, quad, block_live);

	CollectIntervals *cointerval = create_intervals((size_t)graph->reg_count);
	build_intervals(graph, quad, block_live, cointerval);
	sort_intervals(cointerval);

	PhysRegTrack reg_track = {0};
	greedy_allocate(cointerval, &reg_track);
	rewrite_registers(graph, cointerval);

	for (int i = 0; i < total_quads; i++)
		free_quadliveness(&quad[i]);
	for (int i = 0; i < graph->block_count; i++)
		free_blockliveness(&block_live[i]);
	free(quad);
	free(block_live);
	free_intervals(cointerval);
}

static void usage(void)
{
	fprintf(stderr, "Использование: mncc input.txt -o output\n");
}

int main(int argc, char **argv)
{
	char *input = NULL;
	char *output = NULL;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
			output = argv[++i];
		} else {
			input = argv[i];
		}
	}

	if (input == NULL) {
		usage();
		return 1;
	}
	if (output == NULL)
		output = "a.out";

	Lexer lex = {read_source(input), 0};
	lexer = &lex;
	advance();

	struct ASTNode *ast = parse_file();

	HashTable *table = create_table(1024);
	check_semantics(ast, table);

	IRProgram *program = compile_to_ir(table, ast);

	for (int i = 0; i < program->count; i++) {
		IRGraph *graph = program->graphs[i];
		build_cfg(graph);
		into_ssa(graph);
		optimize_ir(graph);
		out_of_ssa(graph);
		alloc_registers(graph);
	}

	Instruction *insns = NULL;
	int count = 0;
	int *starts = NULL;
	char **names = NULL;
	int func_count = 0;

	compile_program_to_binary(program, &insns, &count, &starts, &names, &func_count);

	char obj_path[512];
	snprintf(obj_path, sizeof obj_path, "%s.o", output);
	write_object_file(insns, count, obj_path, starts, names, func_count);
	link_object(obj_path, output);

	return 0;
}