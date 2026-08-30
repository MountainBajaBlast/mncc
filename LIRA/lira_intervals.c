#include "lira.h"

#include <stdlib.h>

CollectIntervals *create_intervals(size_t vreg_count)
{
	CollectIntervals *cointerval = calloc(1, sizeof(CollectIntervals));
	if (cointerval == NULL) {
		return NULL;
	}

	cointerval->intervals_count = vreg_count;

	cointerval->intervals = calloc(vreg_count, sizeof(LivenessInterval));
	if (cointerval->intervals == NULL) {
		free(cointerval);
		return NULL;
	}

	for (size_t i = 0; i < vreg_count; i++) {
		cointerval->intervals[i].start_time = -1;
		cointerval->intervals[i].end_time = -1;
		cointerval->intervals[i].phys_reg = -1;
		cointerval->intervals[i].vreg_id = (int)i;
	}

	return cointerval;
}

void free_intervals(CollectIntervals *cointerval)
{
	if (cointerval == NULL) {
		return;
	}

	if (cointerval->intervals != NULL) {
		free(cointerval->intervals);
	}

	if (cointerval != NULL) {
		free(cointerval);
	}
}


void build_intervals(IRGraph *graph, QuadLiveness *quad, BlockLiveness *block, CollectIntervals *cointerval)
{
	size_t total_quads = 0;

	int *block_start = calloc(graph->block_count, sizeof(int));
	int *block_end = calloc(graph->block_count, sizeof(int));

	BasicBlock *blk = graph->head;
	while (blk != NULL) {
		block_start[blk->id_block] = total_quads;
		Quadriple *q = blk->head;
		while (q != NULL) {
			total_quads++;
			if (q == blk->tail)
				break;
			q = q->next;
		}
		block_end[blk->id_block] = total_quads;
		blk = blk->next_block;
	}

	size_t vreg_count = graph->reg_count;

	for (size_t reg = 0; reg < vreg_count; reg++) {
		int start_time = -1;
		int end_time = -1;

		size_t idx = 0;
		blk = graph->head;
		while (blk != NULL) {
			Quadriple *q = blk->head;
			while (q != NULL) {
				if (check_bit(&quad[idx].def, reg)) {
					start_time = (int)idx;
				}

				if (check_bit(&quad[idx].use, reg) || check_bit(&quad[idx].def, reg)) {
					end_time = (int)idx + 1;
				}
				idx++;
				if (q == blk->tail)
					break;
				q = q->next;
			}
			blk = blk->next_block;
		}

		for (int i = 0; i < graph->block_count; i++) {
			if (check_bit(&block[i].live_in, reg) && (start_time == -1 || block_start[i] < start_time)) {
				start_time = block_start[i];
			}

			if (check_bit(&block[i].live_out, reg) && block_end[i] > end_time) {
				end_time = block_end[i];
			}
		}

		cointerval->intervals[reg].start_time = start_time;
		cointerval->intervals[reg].end_time = end_time;
		cointerval->intervals[reg].phys_reg = -1;
		cointerval->intervals[reg].vreg_id = (int)reg;
	}
	free(block_start);
	free(block_end);
}

int compare_intervals_by_weight(const void *a, const void *b)
{
	const LivenessInterval *interval_a = (const LivenessInterval *)a;
	const LivenessInterval *interval_b = (const LivenessInterval *)b;

	int len_a = interval_a->end_time - interval_a->start_time;
	int len_b = interval_b->end_time - interval_b->start_time;

	if (len_a <= 0)
		len_a = 1;
	if (len_b <= 0)
		len_b = 1;

	double weight_a = 1.0 / len_a;
	double weight_b = 1.0 / len_b;

	if (weight_b > weight_a)
		return 1;
	if (weight_b < weight_a)
		return -1;

	return 0;
}

void sort_intervals(CollectIntervals *cointerval)
{
	if (cointerval == NULL || cointerval->intervals == NULL) {
		return;
	}
	qsort(cointerval->intervals, cointerval->intervals_count, sizeof(LivenessInterval),
	      compare_intervals_by_weight);
}
