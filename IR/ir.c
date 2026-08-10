#include "ir.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../semantic/semantic.h"

Operand *create_operand_num(long long value)
{
	Operand *op = calloc(1, sizeof(Operand));
	op->type = NUM;
	op->val.val_int = value;
	return op;
}

Operand *create_operand_var(char *name)
{
	Operand *vop = calloc(1, sizeof(Operand));
	vop->type = VAR;
	vop->val.var_name = strdup(name);
	return vop;
}

Operand *create_operand_reg(IRGraph *graph)
{
	Operand *rop = calloc(1, sizeof(Operand));
	rop->type = VIR_REG;
	rop->val.tmp_index = graph->reg_count++;
	return rop;
}

Operand *create_operand_reg_ref(int reg)
{
	Operand *op = calloc(1, sizeof(Operand));
	op->type = VIR_REG;
	op->val.tmp_index = reg;
	return op;
}

Quadriple *create_quadriple(Operand *argum1, Operand *argum2, Operand *res,
			    OpCode oper)
{
	Quadriple *quad = calloc(1, sizeof(Quadriple));
	quad->arg1 = argum1;
	quad->arg2 = argum2;
	quad->result = res;
	quad->operation = oper;
	return quad;
}

void append_quadriple(BasicBlock *block, Quadriple *quad)
{
	if (block->head == NULL) {
		block->head = quad;
		block->tail = quad;
	} else {
		block->tail->next = quad;
		quad->prev = block->tail;
		block->tail = quad;
	}
}

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

IRGraph *create_ir_graph(char *function_name)
{
	IRGraph *graph = calloc(1, sizeof(IRGraph));
	graph->func_name = strdup(function_name);
	graph->current_block = NULL;
	graph->block_count = 0;
	graph->blocks = NULL;
	return graph;
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

void set_current_block(IRGraph *graph, BasicBlock *block)
{
	graph->current_block = block;
}

BasicBlock *append_new_block(IRGraph *graph, char *label)
{
	BasicBlock *block = create_basic_block(label);
	block->id_block = graph->block_count;
	graph->blocks = realloc(graph->blocks, (graph->block_count + 1) *
						   sizeof(BasicBlock *));
	if (graph->block_count > 0) {
		graph->blocks[graph->block_count - 1]->next_block = block;
	}
	graph->blocks[graph->block_count] = block;
	graph->current_block = block;
	graph->block_count++;
	return block;
}

void init_arr(RegArr *reg, size_t init_cap)
{
	reg->data = calloc(init_cap, sizeof(int));

	for (size_t i = 0; i < init_cap; i++)
		reg->data[i] = -1;

	reg->size = 0;
	reg->capacity = init_cap;
}

VerMan *init_verman(int symbols, RegArr *regs)
{
	VerMan *vregs = calloc(1, sizeof(VerMan));
	vregs->last_regs = regs;
	vregs->symbol_count = symbols;

	return vregs;
}

void free_verman(VerMan *vregs)
{
	if (vregs == NULL) {
		return;
	}

	if (vregs->last_regs != NULL) {
		if (vregs->last_regs->data != NULL) {
			free(vregs->last_regs->data);
		}

		free(vregs->last_regs);
	}

	free(vregs);
}
int read_reg(VerMan *var, const char *name, HashTable *table)
{
	int found = 0;
	VarSym sum = search(table, name, &found);
	if (!found) {
		fprintf(stderr, "Unknown variable %s\n", name);
		exit(EXIT_FAILURE);
	}

	int variant = var->last_regs->data[sum.symbol_id];

	return variant;
}

void write_reg(VerMan *var, const char *name, HashTable *table, int tmp_index)
{
	int found = 0;

	VarSym sum = search(table, name, &found);

	if (!found) {
		fprintf(stderr, "Unknown variable %s\n", name);
		exit(EXIT_FAILURE);
	}

	var->last_regs->data[sum.symbol_id] = tmp_index;
}

void bb_add_phi(BasicBlock *block, int result_reg, int reg_index,
		int from_block)
{
	if (block->phi_cap <= block->phi_count) {
		block->phi_cap = block->phi_cap ? block->phi_cap * 2 : 4;
		block->phis =
		    realloc(block->phis, block->phi_cap * sizeof(PhiEntry));
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

void bb_add_predecessor(BasicBlock *block, BasicBlock *pred)
{
	if (block->pred_cap <= block->pred_count) {
		block->pred_cap = block->pred_cap ? block->pred_cap * 2 : 4;
		block->predecessors =
		    realloc(block->predecessors,
			    block->pred_cap * sizeof(BasicBlock *));
	}
	block->predecessors[block->pred_count] = pred;
	block->pred_count++;
}

DominatorTree *create_dominator_tree(IRGraph *graph)
{
	DominatorTree *domtree = calloc(1, sizeof(DominatorTree));
	domtree->entry = graph->head;
	int *dfn = calloc(graph->block_count + 1, sizeof(int));
	int *semi = calloc(graph->block_count + 1, sizeof(int));
	int *parent = calloc(graph->block_count + 1, sizeof(int));
	int *ancestor = calloc(graph->block_count + 1, sizeof(int));
	int *label = calloc(graph->block_count + 1, sizeof(int));
	int *idom = calloc(graph->block_count + 1, sizeof(int));

	BasicBlock **bucket =
	    calloc(graph->block_count + 1, sizeof(BasicBlock *));
	BasicBlock **block_by_dfn =
	    calloc(graph->block_count + 1, sizeof(BasicBlock *));

	domtree->dfn = dfn;
	domtree->semi = semi;
	domtree->parent = parent;
	domtree->bucket = bucket;
	domtree->ancestor = ancestor;
	domtree->label = label;
	domtree->idom = idom;
	domtree->block_by_dfn = block_by_dfn;
	domtree->block_count = graph->block_count;

	return domtree;
}

void free_domtree(DominatorTree *domtree)
{
	if (domtree == NULL)
		return;
	free(domtree->dfn);
	free(domtree->semi);
	free(domtree->parent);
	free(domtree->bucket);
	free(domtree->ancestor);
	free(domtree->label);
	free(domtree->idom);
	free(domtree->block_by_dfn);
	free(domtree);
}

void bb_add_successor(BasicBlock *block, BasicBlock *succ)
{
	if (block->succ_cap <= block->succ_count) {
		block->succ_cap = block->succ_cap ? block->succ_cap * 2 : 4;
		block->successors = realloc(
		    block->successors, block->succ_cap * sizeof(BasicBlock *));
	}
	block->successors[block->succ_count] = succ;
	block->succ_count++;
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

static int dsu_find(DominatorTree *domtree, int v)
{
	if (domtree->ancestor[v] == 0) {
		return v;
	}

	int a = domtree->ancestor[v];

	if (domtree->ancestor[a] != 0) {
		int root = dsu_find(domtree, a);

		if (domtree->semi[domtree->label[a]] <
		    domtree->semi[domtree->label[v]]) {
			domtree->label[v] = domtree->label[a];
		}

		domtree->ancestor[v] = root;
	}

	return domtree->ancestor[v];
}

static int dsu_eval(DominatorTree *domtree, int v)
{
	if (domtree->ancestor[v] == 0) {
		return v;
	}

	dsu_find(domtree, v);

	return domtree->label[v];
}

static void dsu_link(DominatorTree *domtree, int v, int w)
{
	domtree->ancestor[w] = v;
}

static void dfs_pass(BasicBlock *node, DominatorTree *domtree, int *next_num)
{
	domtree->dfn[node->id_block] = (*next_num)++;
	domtree->block_by_dfn[domtree->dfn[node->id_block]] = node;

	for (int i = 0; i < node->succ_count; i++) {
		BasicBlock *succ = node->successors[i];
		if (domtree->dfn[succ->id_block] == 0) {
			domtree->parent[*next_num] =
			    domtree->dfn[node->id_block];
			dfs_pass(succ, domtree, next_num);
		}
	}
}

void build_dominators(DominatorTree *domtree)
{
	for (int v = 0; v <= domtree->block_count; v++) {
		domtree->semi[v] = v;
		domtree->label[v] = v;
		domtree->ancestor[v] = 0;
	}

	int next_num = 1;
	dfs_pass(domtree->entry, domtree, &next_num);

	next_num--;

	int *bucket_head = (int *)calloc(next_num + 1, sizeof(int));
	int *bucket_next = (int *)calloc(next_num + 1, sizeof(int));

	for (int i = next_num; i >= 2; i--) {
		BasicBlock *w_block = domtree->block_by_dfn[i];
		int p = domtree->parent[i];

		for (int j = 0; j < w_block->pred_count; j++) {
			BasicBlock *pred_block = w_block->predecessors[j];
			int v = domtree->dfn[pred_block->id_block];
			if (v == 0)
				continue;

			int u = dsu_eval(domtree, v);
			if (domtree->semi[u] < domtree->semi[i]) {
				domtree->semi[i] = domtree->semi[u];
			}
		}

		bucket_next[i] = bucket_head[domtree->semi[i]];
		bucket_head[domtree->semi[i]] = i;

		dsu_link(domtree, p, i);

		int v = bucket_head[p];
		while (v != 0) {
			int u = dsu_eval(domtree, v);
			domtree->idom[v] =
			    (domtree->semi[u] < domtree->semi[v]) ? u : p;
			v = bucket_next[v];
		}
		bucket_head[p] = 0;
	}

	for (int i = 2; i <= next_num; i++) {
		if (domtree->idom[i] != domtree->semi[i]) {
			domtree->idom[i] = domtree->idom[domtree->idom[i]];
		}
	}

	free(bucket_head);
	free(bucket_next);
}

BasicBlock *get_idom(DominatorTree *domtree, BasicBlock *block)
{
	return domtree
	    ->block_by_dfn[domtree->idom[domtree->dfn[block->id_block]]];
}

int dominates(DominatorTree *domtree, BasicBlock *a, BasicBlock *b)
{
	int da = domtree->dfn[a->id_block];
	if (da == 0)
		return 0;

	int curr = domtree->dfn[b->id_block];
	while (curr != 0) {
		if (curr == da)
			return 1;
		curr = domtree->idom[curr];
	}
	return 0;
}

static void df_add(BasicBlock *block, BasicBlock *x)
{
	for (int i = 0; i < block->df_count; i++) {
		if (block->df[i] == x)
			return;
	}
	if (block->df_count == block->df_cap) {
		block->df_cap = block->df_cap ? block->df_cap * 2 : 4;
		block->df =
		    realloc(block->df, block->df_cap * sizeof(BasicBlock *));
	}
	block->df[block->df_count++] = x;
}

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
		if (q->operation != PHIN && q->result &&
		    q->result->type == VIR_REG &&
		    q->result->val.tmp_index == reg)
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
		BasicBlock **queue =
		    calloc(graph->block_count + 1, sizeof(BasicBlock *));
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
						bb_add_phi(y, reg, 0,
							   y->predecessors[p]
							       ->id_block);
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

static void rename_push(int **versions, int *top, int *cap, int var, int ver)
{
	if (top[var] >= cap[var]) {
		cap[var] *= 2;
		versions[var] = realloc(versions[var], cap[var] * sizeof(int));
	}
	versions[var][top[var]++] = ver;
}

static int rename_current(int **versions, int *top, int var)
{
	return versions[var][top[var] - 1];
}

static void ssa_rename(IRGraph *graph, BasicBlock *block, int **versions,
		       int *top, int *cap, int *pushed, int *pushed_idx,
		       int regs)
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
		if (q->arg1 && q->arg1->type == VIR_REG &&
		    q->arg1->val.tmp_index < regs)
			q->arg1->val.tmp_index = rename_current(
			    versions, top, q->arg1->val.tmp_index);

		if (q->arg2 && q->arg2->type == VIR_REG &&
		    q->arg2->val.tmp_index < regs)
			q->arg2->val.tmp_index = rename_current(
			    versions, top, q->arg2->val.tmp_index);

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
			succ->phis[j].reg_index = rename_current(
			    versions, top, succ->phis[j].orig_var);
		}
	}

	for (int i = 0; i < graph->block_count; i++) {
		BasicBlock *c = graph->blocks[i];
		if (c != block && c->idom == block)
			ssa_rename(graph, c, versions, top, cap, pushed,
				   pushed_idx, regs);
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
	ssa_rename(graph, graph->head, versions, top, cap, pushed, &pushed_idx,
		   regs);

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

	if (term->operation == JMP || term->operation == JE ||
	    term->operation == RET) {
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
				insert_before_terminator(
				    pred,
				    create_quadriple(
					create_operand_reg_ref(srcs[k]), NULL,
					create_operand_reg_ref(temps[k]),
					ASSIGN));

			for (int k = 0; k < n; k++)
				insert_before_terminator(
				    pred,
				    create_quadriple(
					create_operand_reg_ref(temps[k]), NULL,
					create_operand_reg_ref(dsts[k]),
					ASSIGN));
		}

		free(block->phis);
		block->phis = NULL;
		block->phi_count = 0;
		block->phi_cap = 0;
	}
}

void dump_graph(IRGraph *graph)
{
	static const char *opnames[] = {"RET",
					"DIV",
					"MULT",
					"ADD",
					"SUB",
					"ASSIGN",
					"JMP",
					"JE",
					"PHIN",
					"CMP",
					"TERMINATOR_NONE"};
	printf("=== reg_count=%d blocks=%d ===\n", graph->reg_count,
	       graph->block_count);
	for (BasicBlock *b = graph->head; b; b = b->next_block) {
		printf("block %d succ:", b->id_block);
		for (int i = 0; i < b->succ_count; i++)
			printf(" %d", b->successors[i]->id_block);
		printf(" pred:");
		for (int i = 0; i < b->pred_count; i++)
			printf(" %d", b->predecessors[i]->id_block);
		printf(" phi:");
		for (int i = 0; i < b->phi_count; i++)
			printf(" [%d<-%d r=%d o=%d]", b->phis[i].reg_index,
			       b->phis[i].from_label, b->phis[i].result_reg,
			       b->phis[i].orig_var);
		printf("\n");
		for (Quadriple *q = b->head; q; q = q->next) {
			const char *t1 =
			    q->arg1 ? (q->arg1->type == VIR_REG	   ? "vr"
				       : q->arg1->type == NUM	   ? "num"
				       : q->arg1->type == REG_PHYS ? "phys"
								   : "?")
				    : "null";
			const char *t2 =
			    q->arg2 ? (q->arg2->type == VIR_REG	   ? "vr"
				       : q->arg2->type == NUM	   ? "num"
				       : q->arg2->type == REG_PHYS ? "phys"
								   : "?")
				    : "null";
			const char *tr =
			    q->result ? (q->result->type == VIR_REG    ? "vr"
					 : q->result->type == NUM      ? "num"
					 : q->result->type == REG_PHYS ? "phys"
								       : "?")
				      : "null";
			long long v1 = q->arg1 ? (q->arg1->type == NUM
						      ? q->arg1->val.val_int
						      : q->arg1->val.tmp_index)
					       : -1;
			long long v2 = q->arg2 ? (q->arg2->type == NUM
						      ? q->arg2->val.val_int
						      : q->arg2->val.tmp_index)
					       : -1;
			long long vr = q->result
					   ? (q->result->type == NUM
						  ? q->result->val.val_int
						  : q->result->val.tmp_index)
					   : -1;
			printf("   %-10s a1=%s(%lld) a2=%s(%lld) r=%s(%lld)\n",
			       opnames[q->operation], t1, v1, t2, v2, tr, vr);
		}
	}
}

IRResult gen_ir_branching(ASTNode *tree, VerMan *manager, HashTable *table,
			  IRGraph *graph)
{
	BasicBlock *entry_bb = graph->current_block;

	BasicBlock *then_bb = append_new_block(graph, "then_bb");
	BasicBlock *else_bb = append_new_block(graph, "else_bb");
	BasicBlock *merge_bb = append_new_block(graph, "merge_bb");

	set_current_block(graph, entry_bb);
	IRResult cond =
	    gen_ir_expr(tree->branching.condition, manager, table, graph);

	Quadriple *je_quad = create_quadriple(cond.operand, NULL, NULL, JE);
	append_quadriple(entry_bb, je_quad);

	entry_bb->terminator = JE;
	entry_bb->true_target_je = then_bb;
	entry_bb->false_target_je = else_bb;

	set_current_block(graph, then_bb);
	if (tree->branching.if_body != NULL) {
		gen_ir_stmt(tree->branching.if_body, manager, table, graph);
	}
	then_bb->terminator = JMP;
	then_bb->jmp_target = merge_bb;

	set_current_block(graph, else_bb);
	if (tree->branching.else_body != NULL) {
		gen_ir_stmt(tree->branching.else_body, manager, table, graph);
	}
	else_bb->terminator = JMP;
	else_bb->jmp_target = merge_bb;

	set_current_block(graph, merge_bb);

	IRResult res;
	res.operand = NULL;
	res.block = merge_bb;
	return res;
}

IRResult gen_ir_expr(ASTNode *tree, VerMan *manager, HashTable *table,
		     IRGraph *graph)
{
	if (tree == NULL) {
		printf("Error: it s null\n");
		exit(1);
	}

	IRResult res;
	res.operand = NULL;

	switch (tree->type) {
	case ND_NUMBER: {
		res.operand = create_operand_num(tree->Numbers.value);
		break;
	}
	case ND_VARIABLES: {
		char *var_name = tree->variables.name;
		int reg = read_reg(manager, var_name, table);
		res.operand = create_operand_reg_ref(reg);
		break;
	}
	case ND_BINARY_OP: {
		ASTNode *left = tree->BinaryOp.left;
		ASTNode *right = tree->BinaryOp.right;

		IRResult left_res = gen_ir_expr(left, manager, table, graph);
		IRResult right_res = gen_ir_expr(right, manager, table, graph);

		Operand *res_op = create_operand_reg(graph);
		OpCode op;

		switch (tree->BinaryOp.op) {
		case TK_ADD:
			op = ADD;
			break;
		case TK_SUBTRACTION:
			op = SUB;
			break;
		case TK_MULTYPLY:
			op = MULT;
			break;
		case TK_DIVISION:
			op = DIV;
			break;
		case TK_CMP:
			op = CMP;
			break;
		default:
			fprintf(stderr, "Error: Unknown binary operator\n");
			exit(1);
		}

		Quadriple *new_quad = create_quadriple(
		    left_res.operand, right_res.operand, res_op, op);
		append_quadriple(graph->current_block, new_quad);
		res.operand = res_op;
		break;
	}
	default:
		break;
	}

	return res;
}

IRResult gen_ir_stmt(ASTNode *tree, VerMan *manager, HashTable *table,
		     IRGraph *graph)
{
	if (tree == NULL) {
		printf("Error: it s null\n");
		exit(1);
	}

	IRResult res;
	res.operand = NULL;

	switch (tree->type) {
	case ND_VAR_DECL: {
		Operand *target_reg = create_operand_reg(graph);

		if (tree->var_decl.initializer) {
			IRResult var_decl_init = gen_ir_stmt(
			    tree->var_decl.initializer, manager, table, graph);
			Quadriple *new_quad_assign = create_quadriple(
			    var_decl_init.operand, NULL, target_reg, ASSIGN);
			append_quadriple(graph->current_block, new_quad_assign);
		}

		write_reg(manager, tree->var_decl.name, table,
			  target_reg->val.tmp_index);
		res.operand = target_reg;
		break;
	}
	case ND_FILE: {
		int count = tree->file.statement_count;

		for (int i = 0; i < count; i++) {
			ASTNode *stmt = tree->file.statements[i];
			gen_ir_stmt(stmt, manager, table, graph);
		}
		break;
	}
	case ND_RETURN: {
		ASTNode *ret_expr = tree->ret.expression;
		IRResult ret_val = gen_ir_expr(ret_expr, manager, table, graph);
		Quadriple *ret_quad =
		    create_quadriple(ret_val.operand, NULL, NULL, RET);
		append_quadriple(graph->current_block, ret_quad);
		graph->current_block->terminator = RET;
		res.operand = ret_val.operand;
		break;
	}
	case ND_ASSIGMENT: {
		Operand *target_reg = create_operand_reg_ref(
		    read_reg(manager, tree->assigment.name, table));

		if (tree->assigment.expression) {
			IRResult var_assigment = gen_ir_stmt(
			    tree->assigment.expression, manager, table, graph);
			Quadriple *new_quad_assign = create_quadriple(
			    var_assigment.operand, NULL, target_reg, ASSIGN);
			append_quadriple(graph->current_block, new_quad_assign);
		}

		write_reg(manager, tree->assigment.name, table,
			  target_reg->val.tmp_index);
		res.operand = target_reg;
		break;
	}
	case ND_BRANCH:
		res = gen_ir_branching(tree, manager, table, graph);
		break;

	case ND_NUMBER:
	case ND_VARIABLES:
	case ND_BINARY_OP:

		res = gen_ir_expr(tree, manager, table, graph);
		break;
	default:
		break;
	}
	return res;
}

IRGraph *compile_to_ir(HashTable *table, ASTNode *node)
{
	IRGraph *graph = create_ir_graph("main");
	BasicBlock *block = create_basic_block("entry");
	block->id_block = 0;
	graph->head = block;
	graph->blocks = realloc(graph->blocks, sizeof(BasicBlock *));
	graph->blocks[graph->block_count] = block;
	graph->block_count++;
	graph->current_block = block;
	int sym_count = table->symbol_count;
	RegArr *array = calloc(1, sizeof(RegArr));
	init_arr(array, table->symbol_count);
	VerMan *manager = init_verman(sym_count, array);
	gen_ir_stmt(node, manager, table, graph);
	free_verman(manager);
	return graph;
}
