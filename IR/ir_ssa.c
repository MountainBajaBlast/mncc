#include "ir.h"

#include <stdlib.h>

#include "../semantic/semantic.h"


void df_add(BasicBlock *block, BasicBlock *x)
{
	for (int i = 0; i < block->df_count; i++) {
		if (block->df[i] == x)
			return;
	}
	if (block->df_count == block->df_cap) {
		block->df_cap = block->df_cap ? block->df_cap * 2 : 4;
		block->df = realloc(block->df, block->df_cap * sizeof(BasicBlock *));
	}
	block->df[block->df_count++] = x;
}



static void rename_push(int **versions, int *top, int *cap, int var, int ver)
{
	if (top[var] >= cap[var]) {
		cap[var] *= 2;
		versions[var] = realloc(versions[var], cap[var] * sizeof(int));
	}
	versions[var][top[var]++] = ver;
}

static int rename_current(int **versions, int *top, int var) { return versions[var][top[var] - 1]; }

static void ssa_rename(IRGraph *graph, BasicBlock *block, int **versions, int *top, int *cap, int *pushed,
		       int *pushed_idx, int regs)
{
	int start = *pushed_idx;

	for (int i = 0; i < block->phi_count; i++) {
		int var = block->phis[i].orig_var;

		int already = 0;
		for (int k = 0; k < i; k++)
			if (block->phis[k].orig_var == var) {
				already = 1;
				break;
			}
		if (already)
			continue;

		int ver = graph->reg_count++;
		rename_push(versions, top, cap, var, ver);
		pushed[(*pushed_idx)++] = var;

		for (int j = 0; j < block->phi_count; j++)
			if (block->phis[j].orig_var == var)
				block->phis[j].result_reg = ver;
	}

	for (Quadriple *q = block->head; q; q = q->next) {
		if (q->arg1 && q->arg1->type == VIR_REG && q->arg1->val.tmp_index < regs)
			q->arg1->val.tmp_index = rename_current(versions, top, q->arg1->val.tmp_index);

		if (q->arg2 && q->arg2->type == VIR_REG && q->arg2->val.tmp_index < regs)
			q->arg2->val.tmp_index = rename_current(versions, top, q->arg2->val.tmp_index);

		if (q->result && q->result->type == VIR_REG) {
			int r = q->result->val.tmp_index;
			int ver = graph->reg_count++;
			q->result->val.tmp_index = ver;
			rename_push(versions, top, cap, r, ver);
			pushed[(*pushed_idx)++] = r;
		}
	}
	for (int i = 0; i < block->succ_count; i++) {
		BasicBlock *succ = block->successors[i];
		for (int j = 0; j < succ->phi_count; j++) {
			if (succ->phis[j].from_label != block->id_block)
				continue;
			succ->phis[j].reg_index = rename_current(versions, top, succ->phis[j].orig_var);
		}
	}

	for (int i = 0; i < graph->block_count; i++) {
		BasicBlock *c = graph->blocks[i];
		if (c != block && c->idom == block)
			ssa_rename(graph, c, versions, top, cap, pushed, pushed_idx, regs);
	}

	for (int i = start; i < *pushed_idx; i++)
		top[pushed[i]]--;
	*pushed_idx = start;
}

void into_ssa(IRGraph *graph)
{
	DominatorTree *dt = create_dominator_tree(graph);
	build_dominators(dt);

	for (int i = 0; i < graph->block_count; i++) {
		BasicBlock *b = graph->blocks[i];
		int d = dt->dfn[b->id_block];
		b->idom = (d > 1) ? dt->block_by_dfn[dt->idom[d]] : NULL;
	}

	compute_dominance_frontier(graph, dt);
	place_phi(graph, dt);

	int regs = graph->reg_count;
	int **versions = calloc(regs, sizeof(int *));
	int *top = calloc(regs, sizeof(int));
	int *cap = calloc(regs, sizeof(int));
	int *pushed = calloc(regs, sizeof(int));
	for (int i = 0; i < regs; i++) {
		versions[i] = malloc(4 * sizeof(int));
		versions[i][0] = i;
		top[i] = 1;
		cap[i] = 4;
	}

	int pushed_idx = 0;
	ssa_rename(graph, graph->head, versions, top, cap, pushed, &pushed_idx, regs);

	for (int i = 0; i < regs; i++)
		free(versions[i]);
	free(versions);
	free(top);
	free(cap);
	free(pushed);

	free_domtree(dt);
}

static void insert_before_terminator(BasicBlock *block, Quadriple *quad)
{
	Quadriple *term = block->tail;
	if (term == NULL) {
		block->head = quad;
		block->tail = quad;
		return;
	}

	if (term->operation == JMP || term->operation == JE || term->operation == RET) {
		quad->next = term;
		quad->prev = term->prev;

		if (term->prev) {
			term->prev->next = quad;
		} else {
			block->head = quad;
		}
		term->prev = quad;
	} else {
		term->next = quad;
		quad->prev = term;
		quad->next = NULL;
		block->tail = quad;
	}
}

void out_of_ssa(IRGraph *graph)
{
	for (int i = 0; i < graph->block_count; i++) {
		BasicBlock *block = graph->blocks[i];
		if (block->phi_count == 0)
			continue;

		for (int p = 0; p < block->pred_count; p++) {
			BasicBlock *pred = block->predecessors[p];
			int srcs[1024];
			int dsts[1024];
			int temps[1024];
			int n = 0;

			for (int j = 0; j < block->phi_count; j++) {
				if (block->phis[j].from_label != pred->id_block)
					continue;
				srcs[n] = block->phis[j].reg_index;
				dsts[n] = block->phis[j].result_reg;
				n++;
			}

			for (int k = 0; k < n; k++)
				temps[k] = graph->reg_count++;

			for (int k = 0; k < n; k++)
				insert_before_terminator(pred,
							 create_quadriple(create_operand_reg_ref(srcs[k]), NULL,
									  create_operand_reg_ref(temps[k]), ASSIGN));

			for (int k = 0; k < n; k++)
				insert_before_terminator(pred,
							 create_quadriple(create_operand_reg_ref(temps[k]), NULL,
									  create_operand_reg_ref(dsts[k]), ASSIGN));
		}

		free(block->phis);
		block->phis = NULL;
		block->phi_count = 0;
		block->phi_cap = 0;
	}
}
