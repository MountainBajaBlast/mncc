#include "ir.h"

#include <stdlib.h>
#include <string.h>

#include "../semantic/semantic.h"



BasicBlock *create_basic_block(char *labe)
{
	BasicBlock *basic_block = calloc(1, sizeof(BasicBlock));
	basic_block->label = strdup(labe);
	basic_block->phis = NULL;
	basic_block->phi_count = 0;
	basic_block->phi_cap = 0;
	basic_block->terminator = TERMINATOR_NONE;
	basic_block->true_target_je = NULL;
	basic_block->false_target_je = NULL;
	basic_block->jmp_target = NULL;
	basic_block->predecessors = NULL;
	basic_block->pred_count = 0;
	basic_block->pred_cap = 0;
	basic_block->successors = NULL;
	basic_block->succ_count = 0;
	basic_block->succ_cap = 0;
	return basic_block;
}

void free_basic_block(BasicBlock *block)
{
	if (block == NULL)
		return;

	Quadriple *curr = block->head;

	while (curr != NULL) {
		Quadriple *next = curr->next;

		free(curr);
		curr = next;
	}

	free(block->phis);
	free(block->predecessors);
	free(block->successors);
	free(block->df);
	if (block->label) {
		free(block->label);
	}
	free(block);
}

void set_current_block(IRGraph *graph, BasicBlock *block) { graph->current_block = block; }

BasicBlock *append_new_block(IRGraph *graph, char *label)
{
	BasicBlock *block = create_basic_block(label);
	block->id_block = graph->block_count;
	graph->blocks = realloc(graph->blocks, (graph->block_count + 1) * sizeof(BasicBlock *));
	if (graph->block_count > 0) {
		graph->blocks[graph->block_count - 1]->next_block = block;
	}
	graph->blocks[graph->block_count] = block;
	graph->current_block = block;
	graph->block_count++;
	return block;
}
