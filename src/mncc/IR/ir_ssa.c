#include "ir.h"

#include <stdlib.h>

#include "../semantic/semantic.h"


void df_add(BasicBlock *block, BasicBlock *x)
{
	FOR_EACH_DF(block, i) {
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
		       int *pushed_idx, int regs, int *needs_rename)
{
	int start = *pushed_idx;

	FOR_EACH_PHI(block, i) {
		int var = block->phis[i].orig_var;

		int already = 0;
		for (int k = 0; k < i; k++)
			if (block->phis[k].orig_var == var) {
				already = 1;
				break;
			}
		if (already) 
			continue;
		

		if (needs_rename[var]) 
			continue;
	

		int ver = graph->reg_count++;
		rename_push(versions, top, cap, var, ver);
		pushed[(*pushed_idx)++] = var;

		FOR_EACH_PHI(block, j)
			if (block->phis[j].orig_var == var)
				block->phis[j].result_reg = ver;
	}

	FOR_EACH_QUAD(block, q) {
		if (q->arg1 && q->arg1->type == VIR_REG && q->arg1->val.tmp_index < regs && needs_rename[q->arg1->val.tmp_index])
			q->arg1->val.tmp_index = rename_current(versions, top, q->arg1->val.tmp_index);

		if (q->arg2 && q->arg2->type == VIR_REG && q->arg2->val.tmp_index < regs && needs_rename[q->arg2->val.tmp_index])
			q->arg2->val.tmp_index = rename_current(versions, top, q->arg2->val.tmp_index);

		if (q->result && q->result->type == VIR_REG && needs_rename[q->result->val.tmp_index]) {
			int r = q->result->val.tmp_index;
			int ver = graph->reg_count++;
			q->result->val.tmp_index = ver;
			rename_push(versions, top, cap, r, ver);
			pushed[(*pushed_idx)++] = r;
		}
	}
	FOR_EACH_SUCC(block, i) {
		BasicBlock *succ = block->successors[i];
		FOR_EACH_PHI(succ, j) {
			if (succ->phis[j].from_label != block->id_block)
				continue;
			succ->phis[j].reg_index = rename_current(versions, top, succ->phis[j].orig_var);
		}
	}

	FOR_EACH_BLOCK(graph, c) {
		if (c != block && c->idom == block)
			ssa_rename(graph, c, versions, top, cap, pushed, pushed_idx, regs, needs_rename);
	}

	for (int i = start; i < *pushed_idx; i++)
		top[pushed[i]]--;
	*pushed_idx = start;
}

void into_ssa(IRGraph *graph)
{
	int regs = graph->reg_count;

	int *defs = calloc(regs, sizeof(int));
	FOR_EACH_BLOCK(graph, b) {
		FOR_EACH_QUAD(b, q)
			if (q->result && q->result->type == VIR_REG && q->result->val.tmp_index < regs)
				defs[q->result->val.tmp_index]++;
		FOR_EACH_PHI(b, j)
			if (b->phis[j].orig_var < regs)
				defs[b->phis[j].orig_var]++;
	}
	int *needs_rename = calloc(regs, sizeof(int));
	for (int i = 0; i < regs; i++)
		needs_rename[i] = defs[i] > 1;
	free(defs);


	DominatorTree *dt = create_dominator_tree(graph);
	build_dominators(dt);

	FOR_EACH_BLOCK(graph, b) {
		int d = dt->dfn[b->id_block];
		b->idom = (d > 1) ? dt->block_by_dfn[dt->idom[d]] : NULL;
	}

	compute_dominance_frontier(graph, dt);
	place_phi(graph, dt);

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
	ssa_rename(graph, graph->head, versions, top, cap, pushed, &pushed_idx, regs, needs_rename);

	for (int i = 0; i < regs; i++)
		free(versions[i]);
	free(versions);
	free(top);
	free(cap);
	free(pushed);
	free(needs_rename);

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
	FOR_EACH_BLOCK(graph, block) {
		if (block->phi_count == 0)
			continue;

		FOR_EACH_PRED(block, p) {
			BasicBlock *pred = block->predecessors[p];
			int srcs[1024];
			int dsts[1024];
			int temps[1024];
			int n = 0;

			FOR_EACH_PHI(block, j) {
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
								  create_operand_reg_ref(temps[k]), NULL, ASSIGN));

			for (int k = 0; k < n; k++)
				insert_before_terminator(pred,
						 create_quadriple(create_operand_reg_ref(temps[k]), NULL,
								  create_operand_reg_ref(dsts[k]), NULL, ASSIGN));
		}

		free(block->phis);
		block->phis = NULL;
		block->phi_count = 0;
		block->phi_cap = 0;
	}
}
