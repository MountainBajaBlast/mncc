#include "ir.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../semantic/semantic.h"


IRGraph *create_ir_graph(char *function_name)
{
	IRGraph *graph = calloc(1, sizeof(IRGraph));
	graph->func_name = strdup(function_name);
	graph->current_block = NULL;
	graph->block_count = 0;
	graph->blocks = NULL;
	return graph;
}


void free_ir_graph(IRGraph *graph)
{
	if (graph == NULL) {
		return;
	}

	BasicBlock *curr_block = graph->head;
	while (curr_block != NULL) {
		BasicBlock *next_bb = curr_block->next_block;
		free_basic_block(curr_block);
		curr_block = next_bb;
	}

	if (graph->func_name != NULL) {
		free(graph->func_name);
	}

	free(graph->blocks);
	free(graph);
}

void build_cfg(IRGraph *graph)
{
	for (int i = 0; i < graph->block_count; i++) {
		BasicBlock *b = graph->blocks[i];
		switch (b->terminator) {
		case JE:
			bb_add_successor(b, b->true_target_je);
			bb_add_successor(b, b->false_target_je);
			bb_add_predecessor(b->true_target_je, b);
			bb_add_predecessor(b->false_target_je, b);
			break;
		case JMP:
			bb_add_successor(b, b->jmp_target);
			bb_add_predecessor(b->jmp_target, b);
			break;
		case RET:
			break;
		case TERMINATOR_NONE:
			if (b->next_block != NULL) {
				bb_add_successor(b, b->next_block);
				bb_add_predecessor(b->next_block, b);
			}
			break;
		default:
			printf("Error\n");
			exit(1);
		}
	}
}


