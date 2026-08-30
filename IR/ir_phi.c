#include "ir.h"

#include <stdlib.h>

#include "../semantic/semantic.h"


void bb_add_phi(BasicBlock *block, int result_reg, int reg_index, int from_block)
{
	if (block->phi_cap <= block->phi_count) {
		block->phi_cap = block->phi_cap ? block->phi_cap * 2 : 4;
		block->phis = realloc(block->phis, block->phi_cap * sizeof(PhiEntry));
	}
	block->phis[block->phi_count].result_reg = result_reg;
	block->phis[block->phi_count].orig_var = result_reg;
	block->phis[block->phi_count].reg_index = reg_index;
	block->phis[block->phi_count].from_label = from_block;
	block->phi_count++;
}

void free_phis(BasicBlock *block)
{
	if (!block)
		return;
	free(block->phis);
	block->phis = NULL;
	block->phi_count = 0;
	block->phi_cap = 0;
}

int dsu_find(DominatorTree *domtree, int v)
{
	if (domtree->ancestor[v] == 0) {
		return v;
	}

	int a = domtree->ancestor[v];

	if (domtree->ancestor[a] != 0) {
		int root = dsu_find(domtree, a);

		if (domtree->semi[domtree->label[a]] < domtree->semi[domtree->label[v]]) {
			domtree->label[v] = domtree->label[a];
		}

		domtree->ancestor[v] = root;
	}

	return domtree->ancestor[v];
}

int dsu_eval(DominatorTree *domtree, int v)
{
	if (domtree->ancestor[v] == 0) {
		return v;
	}

	dsu_find(domtree, v);

	return domtree->label[v];
}

void dsu_link(DominatorTree *domtree, int v, int w) { domtree->ancestor[w] = v; }

void compute_dominance_frontier(IRGraph *graph, DominatorTree *dt)
{
	for (int i = 0; i < graph->block_count; i++) {
		BasicBlock *x = graph->blocks[i];
		int dx = dt->dfn[x->id_block];
		if (dx == 0)
			continue;

		for (int j = 0; j < x->succ_count; j++) {
			BasicBlock *y = x->successors[j];
			int dy = dt->dfn[y->id_block];
			if (dy == 0)
				continue;

			int y_idom = dt->idom[dy];
			int runner = dx;
			while (runner != y_idom) {
				df_add(dt->block_by_dfn[runner], y);
				runner = dt->idom[runner];
			}
		}
	}
}

static int block_defines(BasicBlock *b, int reg)
{
	for (Quadriple *q = b->head; q; q = q->next) {
		if (q->operation != PHIN && q->result && q->result->type == VIR_REG && q->result->val.tmp_index == reg)
			return 1;
	}
	return 0;
}




static int block_has_phi(BasicBlock *b, int reg)
{
	for (int i = 0; i < b->phi_count; i++)
		if (b->phis[i].result_reg == reg)
			return 1;
	return 0;
}

void place_phi(IRGraph *graph, DominatorTree *dt)
{
	int *in_queue = calloc(graph->block_count + 1, sizeof(int));
	for (int reg = 0; reg < graph->reg_count; reg++) {
		BasicBlock **queue = calloc(graph->block_count + 1, sizeof(BasicBlock *));
		int qh = 0, qt = 0;

		for (int i = 0; i < graph->block_count; i++)
			if (block_defines(graph->blocks[i], reg))
				queue[qt++] = graph->blocks[i];

		while (qh < qt) {
			BasicBlock *b = queue[qh++];
			for (int i = 0; i < b->df_count; i++) {
				BasicBlock *y = b->df[i];
				if (!block_has_phi(y, reg)) {
					for (int p = 0; p < y->pred_count; p++)
						bb_add_phi(y, reg, 0, y->predecessors[p]->id_block);
					if (!in_queue[y->id_block]) {
						in_queue[y->id_block] = 1;
						queue[qt++] = y;
					}
				}
			}
		}

		for (int i = 0; i < graph->block_count; i++)
			in_queue[graph->blocks[i]->id_block] = 0;
		free(queue);
	}
	free(in_queue);
}
