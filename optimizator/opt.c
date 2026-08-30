#include "opt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int if_two_nums(Quadriple *curr);
int if_num_null(Quadriple *curr);
int if_num_one(Quadriple *curr);
int if_same_regs(Quadriple *curr);
void run_combiner(BasicBlock *block);
void run_dce(BasicBlock *block, int *use_tab);
int if_var_null(Quadriple *curr);
int is_var_assigned_to_zero(Quadriple *curr, Operand *op);
int if_two_nums_cmp(Quadriple *curr);
int if_cmp_with_null(Quadriple *curr);
void run_phi_dce(BasicBlock *block, int *use_tab);

void fill_use_table(BasicBlock *block, int *use_tab)
{
	if (block == NULL)
		return;
	Quadriple *curr = block->head;

	while (curr != NULL) {
		if (curr->arg1 != NULL && curr->arg1->type == VIR_REG) {
			int idx = curr->arg1->val.tmp_index;
			if (idx >= 0) {
				use_tab[idx]++;
			}
		}

		if (curr->arg2 != NULL && curr->arg2->type == VIR_REG) {
			int idx = curr->arg2->val.tmp_index;
			if (idx >= 0) {
				use_tab[idx]++;
			}
		}

		curr = curr->next;
	}

	for (int j = 0; j < block->phi_count; j++) {
		int idx = block->phis[j].reg_index;
		if (idx >= 0) {
			use_tab[idx]++;
		}
	}
}

typedef struct Pattern {
	OpCode op;
	int (*strategy)(Quadriple *);
} Pattern;

static const Pattern pattern_table[] = {
    {ADD, if_var_null},	    {ADD, if_two_nums},	    {ADD, if_num_null},

    {MULT, if_two_nums},    {MULT, if_num_null},    {MULT, if_num_one}, {MULT, if_var_null},

    {SUB, if_two_nums},	    {SUB, if_same_regs},    {SUB, if_num_null}, {SUB, if_var_null},

    {DIV, if_two_nums},	    {DIV, if_num_one},

    {CMP, if_two_nums_cmp}, {CMP, if_cmp_with_null}};

static const int pattern_table_size = sizeof(pattern_table) / sizeof(Pattern);

int if_num_null(Quadriple *curr)
{
	switch (curr->operation) {
	case ADD: {
		if (curr->arg2 != NULL && curr->arg2->type == NUM && curr->arg2->val.val_int == 0) {
			curr->operation = ASSIGN;
			curr->arg2 = NULL;
			return 1;
		}

		if (curr->arg1 != NULL && curr->arg1->type == NUM && curr->arg1->val.val_int == 0) {
			curr->operation = ASSIGN;

			curr->arg1->type = curr->arg2->type;

			if (curr->arg2->type == VAR) {
				curr->arg1->val.var_name = strdup(curr->arg2->val.var_name);
			} else {
				curr->arg1->val = curr->arg2->val;
			}

			curr->arg2 = NULL;
			return 1;
		}
		break;
	}

	case SUB: {
		if (curr->arg2 != NULL && curr->arg2->type == NUM && curr->arg2->val.val_int == 0) {
			curr->operation = ASSIGN;
			curr->arg2 = NULL;
			return 1;
		}
		break;
	}

	case MULT: {
		if ((curr->arg2 != NULL && curr->arg2->type == NUM && curr->arg2->val.val_int == 0) ||
		    (curr->arg1 != NULL && curr->arg1->type == NUM && curr->arg1->val.val_int == 0)) {
			curr->operation = ASSIGN;

			curr->arg1->type = NUM;
			curr->arg1->val.val_int = 0;

			if (curr->arg2 != NULL) {
				curr->arg2 = NULL;
			}
			return 1;
		}
		break;
	}
	default:
		break;
	}
	return 0;
}

int if_two_nums(Quadriple *curr)
{
	if (curr->arg1 == NULL || curr->arg1->type != NUM || curr->arg2 == NULL || curr->arg2->type != NUM) {
		return 0;
	}

	switch (curr->operation) {
	case ADD: {
		curr->operation = ASSIGN;
		curr->arg1->val.val_int = curr->arg1->val.val_int + curr->arg2->val.val_int;

		curr->arg2 = NULL;
		return 1;
	}
	case SUB: {
		curr->operation = ASSIGN;
		curr->arg1->val.val_int = curr->arg1->val.val_int - curr->arg2->val.val_int;

		curr->arg2 = NULL;
		return 1;
	}
	case MULT: {
		curr->operation = ASSIGN;
		curr->arg1->val.val_int = curr->arg1->val.val_int * curr->arg2->val.val_int;

		curr->arg2 = NULL;
		return 1;
	}
	case DIV: {
		if (curr->arg2->val.val_int == 0) {
			return 0;
		}
		curr->operation = ASSIGN;
		curr->arg1->val.val_int = curr->arg1->val.val_int / curr->arg2->val.val_int;

		curr->arg2 = NULL;
		return 1;
	}
	default:
		break;
	}
	return 0;
}

int if_same_regs(Quadriple *curr)
{
	if (curr->arg1 != NULL && curr->arg1->type == VIR_REG && curr->arg2 != NULL && curr->arg2->type == VIR_REG &&
	    curr->arg1->val.tmp_index == curr->arg2->val.tmp_index) {
		curr->operation = ASSIGN;

		curr->arg1->type = NUM;
		curr->arg1->val.val_int = 0;

		curr->arg2 = NULL;

		return 1;
	}
	return 0;
}

int if_num_one(Quadriple *curr)
{
	switch (curr->operation) {
	case DIV: {
		if (curr->arg2 != NULL && curr->arg2->type == NUM && curr->arg2->val.val_int == 1) {
			curr->operation = ASSIGN;

			curr->arg2 = NULL;
			return 1;
		}
		break;
	}
	case MULT: {
		if (curr->arg2 != NULL && curr->arg2->type == NUM && curr->arg2->val.val_int == 1) {
			curr->operation = ASSIGN;

			curr->arg2 = NULL;
			return 1;
		}

		if (curr->arg1 != NULL && curr->arg1->type == NUM && curr->arg1->val.val_int == 1) {
			curr->operation = ASSIGN;

			curr->arg1->type = curr->arg2->type;

			if (curr->arg2->type == VAR) {
				curr->arg1->val.var_name = strdup(curr->arg2->val.var_name);
			} else {
				curr->arg1->val = curr->arg2->val;
			}

			if (curr->arg2->type == VAR) {
				free(curr->arg2->val.var_name);
			}

			curr->arg2 = NULL;

			return 1;
		}
		break;
	}

	default:
		break;
	}
	return 0;
}

int is_var_assigned_to_zero(Quadriple *curr, Operand *op)
{
	if (op == NULL || curr == NULL)
		return 0;

	Quadriple *scan = curr->prev;
	while (scan != NULL) {
		if (scan->operation == ASSIGN && scan->result != NULL) {
			if (op->type == VIR_REG && scan->result->type == VIR_REG &&
			    op->val.tmp_index == scan->result->val.tmp_index) {
				return (scan->arg1 != NULL && scan->arg1->type == NUM && scan->arg1->val.val_int == 0);
			}

			if (op->type == VAR && scan->result->type == VAR && scan->result->val.var_name != NULL &&
			    op->val.var_name != NULL && strcmp(op->val.var_name, scan->result->val.var_name) == 0) {
				return (scan->arg1 != NULL && scan->arg1->type == NUM && scan->arg1->val.val_int == 0);
			}
		}
		scan = scan->prev;
	}
	return 0;
}

int if_var_null(Quadriple *curr)
{
	int arg1_is_zero = is_var_assigned_to_zero(curr, curr->arg1);
	int arg2_is_zero = is_var_assigned_to_zero(curr, curr->arg2);

	if (!arg1_is_zero && !arg2_is_zero) {
		return 0;
	}

	switch (curr->operation) {
	case ADD:
		if (arg1_is_zero) {
			curr->arg1->type = curr->arg2->type;
			curr->arg1->val = curr->arg2->val;
		}

		curr->operation = ASSIGN;
		curr->arg2 = NULL;
		return 1;

	case SUB:

		if (arg2_is_zero) {
			curr->operation = ASSIGN;
			curr->arg2 = NULL;
			return 1;
		}
		break;

	case MULT:

		curr->operation = ASSIGN;
		curr->arg1->type = NUM;
		curr->arg1->val.val_int = 0;
		curr->arg2 = NULL;
		return 1;

	default:
		break;
	}
	return 0;
}

int if_two_nums_cmp(Quadriple *curr)
{
	if (curr->operation != CMP)
		return 0;

	if (curr->arg1 == NULL || curr->arg1->type != NUM || curr->arg2 == NULL || curr->arg2->type != NUM) {
		return 0;
	}

	long long result = (curr->arg1->val.val_int == curr->arg2->val.val_int) ? 1 : 0;

	curr->operation = ASSIGN;
	curr->arg1->type = NUM;
	curr->arg1->val.val_int = result;
	curr->arg2 = NULL;
	return 1;
}

int if_cmp_with_null(Quadriple *curr)
{
	if (curr->operation != CMP)
		return 0;

	int arg1_is_zero = is_var_assigned_to_zero(curr, curr->arg1);
	int arg2_is_zero = is_var_assigned_to_zero(curr, curr->arg2);

	if (!arg1_is_zero || !arg2_is_zero) {
		return 0;
	}

	curr->operation = ASSIGN;
	curr->arg1->type = NUM;
	curr->arg1->val.val_int = 1;
	curr->arg2 = NULL;
	return 1;
}

void run_copy_propagation_phi(IRGraph *graph)
{
	if (graph == NULL)
		return;

	int changed;
	do {
		changed = 0;
		for (int i = 0; i < graph->block_count; i++) {
			BasicBlock *block = graph->blocks[i];
			for (int j = 0; j < block->phi_count; j++) {
				int src_reg = block->phis[j].reg_index;

				BasicBlock *pred = NULL;
				for (int p = 0; p < block->pred_count; p++) {
					if (block->predecessors[p]->id_block == block->phis[j].from_label) {
						pred = block->predecessors[p];
						break;
					}
				}
				if (pred == NULL)
					continue;

				Quadriple *scan = pred->tail;
				while (scan != NULL) {
					if (scan->operation == ASSIGN && scan->result != NULL &&
					    scan->result->type == VIR_REG && scan->result->val.tmp_index == src_reg &&
					    scan->arg1 != NULL && scan->arg1->type == VIR_REG) {
						block->phis[j].reg_index = scan->arg1->val.tmp_index;
						changed = 1;
						break;
					}
					scan = scan->prev;
				}
			}
		}
	} while (changed);
}

static void run_copy_propagation(BasicBlock *block)
{
	if (block == NULL || block->head == NULL)
		return;

	int changed;
	do {
		changed = 0;
		Quadriple *curr = block->head;
		while (curr != NULL) {
			if (curr->operation == ASSIGN && curr->result != NULL && curr->result->type == VIR_REG &&
			    curr->arg1 != NULL && curr->arg1->type == VIR_REG) {
				int dst = curr->result->val.tmp_index;
				int src = curr->arg1->val.tmp_index;

				if (dst == src) {
					curr = curr->next;
					continue;
				}

				Quadriple *scan = curr->next;
				while (scan != NULL) {
					if (scan->arg1 != NULL && scan->arg1->type == VIR_REG &&
					    scan->arg1->val.tmp_index == dst) {
						scan->arg1->val.tmp_index = src;
						changed = 1;
					}
					if (scan->arg2 != NULL && scan->arg2->type == VIR_REG &&
					    scan->arg2->val.tmp_index == dst) {
						scan->arg2->val.tmp_index = src;
						changed = 1;
					}
					scan = scan->next;
				}
			}
			curr = curr->next;
		}
	} while (changed);
}

void fold_conditional_branch(BasicBlock *block)
{
	if (block == NULL || block->terminator != JE)
		return;

	Quadriple *je = block->tail;
	if (je == NULL || je->operation != JE)
		return;

	Operand *cond = je->arg1;
	if (cond == NULL || cond->type != VIR_REG)
		return;

	Quadriple *scan = je->prev;
	while (scan != NULL) {
		if (scan->operation == ASSIGN && scan->result != NULL && scan->result->type == VIR_REG &&
		    scan->result->val.tmp_index == cond->val.tmp_index) {
			if (scan->arg1 == NULL || scan->arg1->type != NUM) {
				return;
			}

			BasicBlock *target =
			    (scan->arg1->val.val_int != 0) ? block->true_target_je : block->false_target_je;

			BasicBlock *dead =
			    (target == block->true_target_je) ? block->false_target_je : block->true_target_je;

			if (dead != NULL) {
				for (int i = 0; i < dead->pred_count; i++) {
					if (dead->predecessors[i] == block) {
						for (int j = i; j < dead->pred_count - 1; j++)
							dead->predecessors[j] = dead->predecessors[j + 1];
						dead->pred_count--;
						break;
					}
				}
			}

			for (int i = 0; i < block->succ_count; i++) {
				if (block->successors[i] == dead) {
					for (int j = i; j < block->succ_count - 1; j++)
						block->successors[j] = block->successors[j + 1];
					block->succ_count--;
					break;
				}
			}

			block->terminator = JMP;
			block->jmp_target = target;
			block->true_target_je = NULL;
			block->false_target_je = NULL;

			je->operation = JMP;
			je->arg1 = NULL;
			return;
		}
		scan = scan->prev;
	}
}

void remove_unreachable_blocks(IRGraph *graph)
{
	if (graph == NULL || graph->head == NULL)
		return;

	int n = graph->block_count;

	int *reach = calloc(n, sizeof(int));
	BasicBlock **queue = calloc(n, sizeof(BasicBlock *));
	int qh = 0, qt = 0;

	queue[qt++] = graph->head;
	reach[graph->head->id_block] = 1;

	while (qh < qt) {
		BasicBlock *b = queue[qh++];
		for (int i = 0; i < b->succ_count; i++) {
			BasicBlock *s = b->successors[i];
			if (s != NULL && !reach[s->id_block]) {
				reach[s->id_block] = 1;
				queue[qt++] = s;
			}
		}
	}

	BasicBlock **live = calloc(n, sizeof(BasicBlock *));
	int *new_id = calloc(n, sizeof(int));
	for (int i = 0; i < n; i++)
		new_id[i] = -1;

	int live_count = 0;
	for (int i = 0; i < n; i++) {
		BasicBlock *b = graph->blocks[i];
		if (b->id_block >= 0 && b->id_block < n && reach[b->id_block]) {
			new_id[b->id_block] = live_count;
			live[live_count++] = b;
		}
	}

	/* Free dead blocks while their ids are still the original ones. */
	for (int i = 0; i < n; i++) {
		BasicBlock *b = graph->blocks[i];
		if (b->id_block < 0 || b->id_block >= n || new_id[b->id_block] == -1) {
			free_basic_block(b);
		}
	}

	for (int i = 0; i < live_count; i++) {
		live[i]->id_block = i;
	}

	for (int i = 0; i < live_count; i++) {
		BasicBlock *b = live[i];
		for (int j = 0; j < b->phi_count; j++) {
			int from = b->phis[j].from_label;
			b->phis[j].from_label = (from >= 0 && from < n) ? new_id[from] : from;
		}
	}

	for (int i = 0; i < live_count; i++) {
		live[i]->next_block = (i + 1 < live_count) ? live[i + 1] : NULL;
	}

	free(graph->blocks);
	graph->blocks = live;
	graph->block_count = live_count;
	graph->head = (live_count > 0) ? live[0] : NULL;

	free(reach);
	free(queue);
	free(new_id);
}

void run_combiner(BasicBlock *block)
{
	if (block == NULL || block->head == NULL)
		return;

	int changed;

	do {
		changed = 0;
		Quadriple *curr = block->head;

		while (curr != NULL) {
			for (int i = 0; i < pattern_table_size; i++) {
				if (curr->operation == pattern_table[i].op) {
					if (pattern_table[i].strategy(curr)) {
						changed = 1;

						break;
					}
				}
			}
			curr = curr->next;
		}
	} while (changed);
}

void optimize_ir(IRGraph *graph)
{
	if (graph == NULL || graph->head == NULL) {
		return;
	}

	BasicBlock *curr = graph->head;
	while (curr != NULL) {
		run_combiner(curr);
		fold_conditional_branch(curr);
		run_copy_propagation(curr);
		curr = curr->next_block;
	}

	run_copy_propagation_phi(graph);

	int regs = graph->reg_count;
	int *use_tab = calloc(regs, sizeof(int));
	if (use_tab == NULL) {
		fprintf(stderr, "Ошибка памяти\n");
		exit(1);
	}

	curr = graph->head;
	while (curr != NULL) {
		fill_use_table(curr, use_tab);
		curr = curr->next_block;
	}

	curr = graph->head;
	while (curr != NULL) {
		run_dce(curr, use_tab);
		run_phi_dce(curr, use_tab);
		curr = curr->next_block;
	}

	remove_unreachable_blocks(graph);

	free(use_tab);
}

void run_phis_dce(BasicBlock *block, int *use_tab)
{
	if (block == NULL || block->phi_count == 0)
		return;

	int w = 0;

	for (int i = 0; i < block->phi_count; i++) {
		if (use_tab[block->phis[i].result_reg] == 0) {
			continue;
		}

		if (w != i) {
			block->phis[w] = block->phis[i];
		}
		w++;
	}
	block->phi_count = w;
}

void run_dce(BasicBlock *block, int *use_tab)
{
	if (block == NULL || block->head == NULL)
		return;

	Quadriple *curr = block->head;

	while (curr != NULL) {
		Quadriple *next_var = curr->next;

		if (curr->result != NULL && curr->result->type == VIR_REG) {
			int res_reg = curr->result->val.tmp_index;

			if (use_tab[res_reg] == 0) {
				if (curr->prev != NULL) {
					curr->prev->next = curr->next;
				} else {
					block->head = curr->next;
				}

				if (curr->next != NULL) {
					curr->next->prev = curr->prev;
				} else {
					block->tail = curr->prev;
				}

				free(curr);
			}
		}

		curr = next_var;
	}
}

void run_phi_dce(BasicBlock *block, int *use_tab)
{
	if (block == NULL || block->phi_count == 0)
		return;
	int w = 0;
	for (int i = 0; i < block->phi_count; i++) {
		if (use_tab[block->phis[i].result_reg] == 0) {
			continue;
		}
		if (w != i) {
			block->phis[w] = block->phis[i];
		}
		w++;
	}
	block->phi_count = w;
}
