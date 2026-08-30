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

void dump_graph(IRGraph *graph)
{
	static const char *opnames[] = {"RET",	"DIV", "MULT",		 "ADD", "SUB", "ASSIGN", "JMP", "JE",
					"PHIN", "CMP", "TERMINATOR_NONE"};
	printf("=== reg_count=%d blocks=%d ===\n", graph->reg_count, graph->block_count);
	for (BasicBlock *b = graph->head; b; b = b->next_block) {
		printf("block %d succ:", b->id_block);
		for (int i = 0; i < b->succ_count; i++)
			printf(" %d", b->successors[i]->id_block);
		printf(" pred:");
		for (int i = 0; i < b->pred_count; i++)
			printf(" %d", b->predecessors[i]->id_block);
		printf(" phi:");
		for (int i = 0; i < b->phi_count; i++)
			printf(" [%d<-%d r=%d o=%d]", b->phis[i].reg_index, b->phis[i].from_label,
			       b->phis[i].result_reg, b->phis[i].orig_var);
		printf("\n");
		for (Quadriple *q = b->head; q; q = q->next) {
			const char *t1 = q->arg1 ? (q->arg1->type == VIR_REG	? "vr"
						    : q->arg1->type == NUM	? "num"
						    : q->arg1->type == REG_PHYS ? "phys"
										: "?")
						 : "null";
			const char *t2 = q->arg2 ? (q->arg2->type == VIR_REG	? "vr"
						    : q->arg2->type == NUM	? "num"
						    : q->arg2->type == REG_PHYS ? "phys"
										: "?")
						 : "null";
			const char *tr = q->result ? (q->result->type == VIR_REG    ? "vr"
						      : q->result->type == NUM	    ? "num"
						      : q->result->type == REG_PHYS ? "phys"
										    : "?")
						   : "null";
			long long v1 =
			    q->arg1 ? (q->arg1->type == NUM ? q->arg1->val.val_int : q->arg1->val.tmp_index) : -1;
			long long v2 =
			    q->arg2 ? (q->arg2->type == NUM ? q->arg2->val.val_int : q->arg2->val.tmp_index) : -1;
			long long vr =
			    q->result ? (q->result->type == NUM ? q->result->val.val_int : q->result->val.tmp_index)
				      : -1;
			printf("   %-10s a1=%s(%lld) a2=%s(%lld) r=%s(%lld)\n", opnames[q->operation], t1, v1, t2, v2,
			       tr, vr);
		}
	}
}
