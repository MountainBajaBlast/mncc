#include "ir.h"

#include <stdlib.h>

#include "../semantic/semantic.h"



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


