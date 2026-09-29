#include "lira.h"

#include <stdlib.h>

#define SET_INTERVAL(interval, vreg, start, end, crosses) \
	do {                                             \
		(interval)->start_time = (start);        \
		(interval)->end_time = (end);            \
		(interval)->phys_reg = -1;               \
		(interval)->vreg_id = (int)(vreg);       \
		(interval)->is_call = (crosses);         \
	} while (0)

CollectIntervals *create_intervals(size_t vreg_count)
{
	CollectIntervals *cointerval = calloc(1, sizeof(CollectIntervals));
	if (!cointerval)
		return NULL;

	cointerval->intervals_count = vreg_count;
	cointerval->intervals = calloc(vreg_count, sizeof(LivenessInterval));
	if (!cointerval->intervals) {
		free(cointerval);
		return NULL;
	}

	for (size_t virtual_reg = 0; virtual_reg < vreg_count; virtual_reg++)
		SET_INTERVAL(&cointerval->intervals[virtual_reg], virtual_reg, -1, -1, 0);

	return cointerval;
}


static Quadriple **collect_calls(IRGraph *graph, size_t *call_count)
{
	size_t calls_count = 0;
	EACH_QUAD(basic_block, current_quad)
		if (current_quad->operation == CALL)
			calls_count++;

	Quadriple **calls = calloc(calls_count ? calls_count : 1, sizeof(Quadriple *));
	size_t index = 0;
	EACH_QUAD(basic_block, current_quad)
		if (current_quad->operation == CALL)
			calls[index++] = current_quad;

	*call_count = calls_count;
	return calls;
}


void free_intervals(CollectIntervals *cointerval)
{
	if (cointerval) {
		free(cointerval->intervals);
		free(cointerval);
	}
}

static void build_one_interval(IRGraph *graph, QuadLiveness *quad, BlockLiveness *block,
			       int *block_start, int *block_end, size_t virtual_reg,
			       LivenessInterval *interval)
{
	int start_time = -1, end_time = -1, is_call = 0;
	size_t quad_index = 0;

	EACH_QUAD(basic_block, current_quad) {
		int used = check_bit(&quad[quad_index].use, virtual_reg);
		int defined = check_bit(&quad[quad_index].def, virtual_reg);
		if (defined)
			start_time = (int)quad_index;
		if (used || defined)
			end_time = (int)quad_index + 1;
		if (current_quad->operation == CALL && (used || defined))
			is_call = 1;
		quad_index++;
	}

	for (int block_number = 0; block_number < graph->block_count; block_number++) {
		if (check_bit(&block[block_number].live_in, virtual_reg) &&
		    (start_time == -1 || block_start[block_number] < start_time))
			start_time = block_start[block_number];
		if (check_bit(&block[block_number].live_out, virtual_reg) && block_end[block_number] > end_time)
			end_time = block_end[block_number];
	}

	SET_INTERVAL(interval, virtual_reg, start_time, end_time, is_call);
}

void build_intervals(IRGraph *graph, QuadLiveness *quad, BlockLiveness *block, CollectIntervals *cointerval)
{
	int *block_start = calloc(graph->block_count, sizeof(int));
	int *block_end = calloc(graph->block_count, sizeof(int));
	size_t total_quads = 0;

	for (BasicBlock *basic_block = graph->head; basic_block; basic_block = basic_block->next_block) {
		block_start[basic_block->id_block] = (int)total_quads;
		for (Quadriple *current_quad = basic_block->head; current_quad; current_quad = current_quad->next)
			total_quads++;
		block_end[basic_block->id_block] = (int)total_quads;
	}

	for (size_t virtual_reg = 0; virtual_reg < (size_t)graph->reg_count; virtual_reg++)
		build_one_interval(graph, quad, block, block_start, block_end, virtual_reg,
				   &cointerval->intervals[virtual_reg]);

	size_t call_count = 0;
	free(collect_calls(graph, &call_count));

	free(block_start);
	free(block_end);
}



int compare_intervals_by_weight(const void *left, const void *right)
{
	int left_length = ((const LivenessInterval *)left)->end_time - ((const LivenessInterval *)left)->start_time;
	int right_length = ((const LivenessInterval *)right)->end_time - ((const LivenessInterval *)right)->start_time;
	double left_weight = 1.0 / (left_length > 0 ? left_length : 1);
	double right_weight = 1.0 / (right_length > 0 ? right_length : 1);
	return right_weight > left_weight ? 1 : right_weight < left_weight ? -1 : 0;
}

void sort_intervals(CollectIntervals *cointerval)
{
	if (cointerval && cointerval->intervals)
		qsort(cointerval->intervals, cointerval->intervals_count,
		      sizeof(LivenessInterval), compare_intervals_by_weight);
}
