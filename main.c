/*
 * mncc — AOT compiler for Apple Silicon.
 *
 * Copyright (C) 2026 MountainBajaBlast
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "IR/ir.h"
#include "LIRA/lira.h"
#include "codegen/codegen.h"
#include "lexer/lexer.h"
#include "optimizator/opt.h"
#include "parser/parser.h"
#include "semantic/semantic.h"

extern Lexer *lexer;

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <source-file> [-o <output>]\n",
			argv[0]);
		return EXIT_FAILURE;
	}

	const char *source_path = argv[1];
	const char *output_path = "output_program";

	for (int i = 2; i < argc; i++) {
		if (strcmp(argv[i], "-o") == 0) {
			if (i + 1 >= argc) {
				fprintf(stderr,
					"Error: -o requires an argument\n");
				return EXIT_FAILURE;
			}
			output_path = argv[++i];
		}
	}

	FILE *file = fopen(source_path, "rb");
	if (!file) {
		fprintf(stderr, "Error: cannot open file '%s'\n", source_path);
		return EXIT_FAILURE;
	}

	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		return EXIT_FAILURE;
	}

	long size = ftell(file);
	if (size < 0) {
		fclose(file);
		return EXIT_FAILURE;
	}

	rewind(file);

	char *source_code = malloc((size_t)size + 1);
	if (!source_code) {
		fclose(file);
		return EXIT_FAILURE;
	}

	size_t bytes_read = fread(source_code, 1, (size_t)size, file);
	fclose(file);

	if (bytes_read != (size_t)size) {
		free(source_code);
		return EXIT_FAILURE;
	}

	source_code[size] = '\0';
	Lexer l;
	l.source = source_code;
	l.pos = 0;
	lexer = &l;

	advance();

	ASTNode *root = parse_function();
	if (!root) {
		fprintf(stderr, "Error: parsing failed\n");
		free(source_code);
		return EXIT_FAILURE;
	}

	HashTable *global_sym_table = create_table(1024);
	if (!global_sym_table) {
		free_ast(root);
		free(source_code);
		return EXIT_FAILURE;
	}

	check_semantics(root, global_sym_table);

	IRGraph *ssa_graph = compile_to_ir(global_sym_table, root);
	if (!ssa_graph) {
		fprintf(stderr, "Error: IR generation failed\n");
		free_table(global_sym_table);
		free_ast(root);
		free(source_code);
		return EXIT_FAILURE;
	}

	build_cfg(ssa_graph);
	into_ssa(ssa_graph);

	optimize_ir(ssa_graph);
	out_of_ssa(ssa_graph);

	size_t total_quads = 0;
	BasicBlock *current_block = ssa_graph->head;
	while (current_block != NULL) {
		Quadriple *q = current_block->head;
		while (q != NULL) {
			total_quads++;
			if (q == current_block->tail)
				break;
			q = q->next;
		}
		current_block = current_block->next_block;
	}

	QuadLiveness *quad_liveness_array =
	    calloc(total_quads, sizeof(QuadLiveness));

	size_t block_count = 0;
	current_block = ssa_graph->head;
	while (current_block != NULL) {
		block_count++;
		current_block = current_block->next_block;
	}
	BlockLiveness *block_liveness_array =
	    calloc(block_count, sizeof(BlockLiveness));

	compute_local_liveness(ssa_graph, quad_liveness_array,
			       block_liveness_array);
	compute_global_liveness(ssa_graph, quad_liveness_array,
				block_liveness_array);

	CollectIntervals *cointerval = create_intervals(ssa_graph->reg_count);

	build_intervals(ssa_graph, quad_liveness_array, block_liveness_array,
			cointerval);
	sort_intervals(cointerval);

	PhysRegTrack reg_track;
	for (int i = 0; i < 31; i++)
		reg_track.phys_reg[i] = -1;
	greedy_allocate(cointerval, &reg_track);

	rewrite_registers(ssa_graph, cointerval);

	RISCinstruct *insns = NULL;
	int insn_count = 0;

	compile_to_binary(ssa_graph, &insns, &insn_count);

	write_object_file(insns, insn_count, "temp.o");
	link_object("temp.o", output_path);

	free(insns);

	free_intervals(cointerval);
	free(quad_liveness_array);
	free(block_liveness_array);

	free_ast(root);
	free_ir_graph(ssa_graph);
	free_table(global_sym_table);
	free(source_code);

	return EXIT_SUCCESS;
}
