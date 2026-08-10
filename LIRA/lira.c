#include "lira.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

BitVector create_bit_vector(size_t max_regs)
{
	BitVector bv;

	bv.bytes_count = (max_regs + 7) / 8;
	bv.max_regs = max_regs;

	bv.data =
	    (unsigned char *)calloc(bv.bytes_count, sizeof(unsigned char));

	if (bv.data == NULL) {
		bv.bytes_count = 0;
		bv.max_regs = 0;
	}

	return bv;
}

void free_bit_vector(BitVector *bv)
{
	if (bv == NULL) {
		return;
	}

	if (bv->data != NULL) {
		free(bv->data);
		bv->data = NULL;
	}

	bv->bytes_count = 0;
	bv->max_regs = 0;
}

void init_quad_liveness(QuadLiveness *ql, size_t max_regs)
{
	if (ql == NULL) {
		return;
	}

	ql->use = create_bit_vector(max_regs);
	ql->def = create_bit_vector(max_regs);
}

void free_quadliveness(QuadLiveness *ql)
{
	if (ql == NULL) {
		return;
	}

	free_bit_vector(&ql->use);
	free_bit_vector(&ql->def);
}

void init_block_liveness(BlockLiveness *bl, size_t max_regs)
{
	if (bl == NULL) {
		return;
	}

	bl->b_use = create_bit_vector(max_regs);
	bl->b_def = create_bit_vector(max_regs);
	bl->live_in = create_bit_vector(max_regs);
	bl->live_out = create_bit_vector(max_regs);
}

void free_blockliveness(BlockLiveness *bl)
{
	if (bl == NULL) {
		return;
	}

	free_bit_vector(&bl->b_use);
	free_bit_vector(&bl->b_def);
	free_bit_vector(&bl->live_in);
	free_bit_vector(&bl->live_out);
}

void set_bit(BitVector *bv, size_t idx)
{
	if (bv == NULL || idx >= bv->max_regs) {
		return;
	}

	size_t byte_idx = idx / 8;
	size_t bit_idx = idx % 8;

	bv->data[byte_idx] |= (1 << bit_idx);
}

int check_bit(BitVector *bv, size_t idx)
{
	if (bv == NULL || idx >= bv->max_regs) {
		return 0;
	}

	size_t byte_idx = idx / 8;
	size_t bit_idx = idx % 8;

	return (bv->data[byte_idx] & (1 << bit_idx)) != 0;
}

void bit_vector_union(BitVector *left, const BitVector *right)
{
	if (left == NULL || right == NULL ||
	    left->bytes_count != right->bytes_count) {
		return;
	}

	for (int i = 0; i < left->bytes_count; i++) {
		left->data[i] |= right->data[i];
	}
}

void bit_vector_diff(BitVector *left, const BitVector *right)
{
	if (left == NULL || right == NULL ||
	    left->bytes_count != right->bytes_count) {
		return;
	}

	for (int i = 0; i < left->bytes_count; i++) {
		left->data[i] &= ~right->data[i];
	}
}

int bit_vector_equal(const BitVector *bv1, const BitVector *bv2)
{
	if (bv1 == NULL || bv2 == NULL ||
	    bv1->bytes_count != bv2->bytes_count) {
		return 0;
	}

	return memcmp(bv1->data, bv2->data, bv1->bytes_count) == 0;
}

static inline void process_operand(BitVector *bv, const Operand *op)
{
	if (op && op->type == VIR_REG) {
		set_bit(bv, op->val.tmp_index);
	}
}

void compute_local_liveness(IRGraph *graph, QuadLiveness *quad,
			    BlockLiveness *block)
{
	if (graph == NULL || quad == NULL || block == NULL)
		return;

	size_t q_idx = 0;
	size_t b_idx = 0;
	BasicBlock *current_block = graph->head;

	while (current_block != NULL) {
		init_block_liveness(&block[b_idx], graph->reg_count);

		Quadriple *current_quad = current_block->head;
		while (current_quad != NULL) {
			init_quad_liveness(&quad[q_idx], graph->reg_count);

			process_operand(&quad[q_idx].use, current_quad->arg1);
			process_operand(&quad[q_idx].use, current_quad->arg2);
			process_operand(&quad[q_idx].def, current_quad->result);

			for (size_t byte = 0;
			     byte < block[b_idx].b_use.bytes_count; byte++) {
				unsigned char inst_use_minus_block_def =
				    quad[q_idx].use.data[byte] &
				    ~block[b_idx].b_def.data[byte];
				block[b_idx].b_use.data[byte] |=
				    inst_use_minus_block_def;
				block[b_idx].b_def.data[byte] |=
				    quad[q_idx].def.data[byte];
			}

			q_idx++;
			if (current_quad == current_block->tail)
				break;
			current_quad = current_quad->next;
		}

		b_idx++;
		current_block = current_block->next_block;
	}
}

void compute_global_liveness(IRGraph *graph, QuadLiveness *quad,
			     BlockLiveness *block)
{
	if (graph == NULL || quad == NULL || block == NULL)
		return;

	bool changed;
	do {
		changed = false;
		size_t b_idx = 0;
		size_t total_quads = 0;
		BasicBlock *current_block = graph->head;
		while (current_block != NULL) {
			size_t block_quad_count = 0;
			Quadriple *q = current_block->head;
			while (q != NULL) {
				block_quad_count++;
				if (q == current_block->tail)
					break;
				q = q->next;
			}

			for (int i = 0; i < current_block->succ_count; i++) {
				BasicBlock *succ = current_block->successors[i];
				if (succ != NULL) {
					bit_vector_union(
					    &block[b_idx].live_out,
					    &block[succ->id_block].live_in);
				}
			}

			if (block_quad_count > 0) {
				BitVector current_live =
				    create_bit_vector(graph->reg_count);
				bit_vector_union(&current_live,
						 &block[b_idx].live_out);

				size_t block_end_idx =
				    total_quads + block_quad_count - 1;
				size_t block_start_idx = total_quads;

				for (size_t i = block_end_idx;; i--) {
					bit_vector_diff(&current_live,
							&quad[i].def);
					bit_vector_union(&current_live,
							 &quad[i].use);

					if (i == block_start_idx)
						break;
				}

				if (!bit_vector_equal(&block[b_idx].live_in,
						      &current_live)) {
					bit_vector_diff(&block[b_idx].live_in,
							&block[b_idx].live_in);
					bit_vector_union(&block[b_idx].live_in,
							 &current_live);
					changed = true;
				}
				free_bit_vector(&current_live);
			} else {
				if (!bit_vector_equal(&block[b_idx].live_in,
						      &block[b_idx].live_out)) {
					bit_vector_diff(&block[b_idx].live_in,
							&block[b_idx].live_in);
					bit_vector_union(
					    &block[b_idx].live_in,
					    &block[b_idx].live_out);
					changed = true;
				}
			}
			total_quads += block_quad_count;
			b_idx++;
			current_block = current_block->next_block;
		}

	} while (changed);
}

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

void build_intervals(IRGraph *graph, QuadLiveness *quad, BlockLiveness *block,
		     CollectIntervals *cointerval)
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

				if (check_bit(&quad[idx].use, reg) ||
				    check_bit(&quad[idx].def, reg)) {
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
			if (check_bit(&block[i].live_in, reg) &&
			    (start_time == -1 || block_start[i] < start_time)) {
				start_time = block_start[i];
			}

			if (check_bit(&block[i].live_out, reg) &&
			    block_end[i] > end_time) {
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
	qsort(cointerval->intervals, cointerval->intervals_count,
	      sizeof(LivenessInterval), compare_intervals_by_weight);
}

void greedy_allocate(CollectIntervals *cointerval, PhysRegTrack *reg_track)
{
	if (cointerval == NULL || cointerval->intervals == NULL) {
		return;
	}

	for (size_t reg_idx = 0; reg_idx < cointerval->intervals_count;
	     reg_idx++) {
		LivenessInterval *current = &cointerval->intervals[reg_idx];

		for (size_t i = 0; i < 31; i++) {
			if (i == 16 || i == 17 || i == 18 || i == 29 || i == 30)
				continue;
			if (reg_track->phys_reg[i] < current->start_time) {
				current->phys_reg = i;
				reg_track->phys_reg[i] = current->end_time;
				break;
			}
		}
	}
}

static void assign_physical_reg(Operand *op, CollectIntervals *cointerval)
{
	if (op == NULL || op->type == NUM) {
		return;
	}

	int vreg_id = op->val.tmp_index;

	for (size_t i = 0; i < cointerval->intervals_count; i++) {
		if (cointerval->intervals[i].vreg_id == vreg_id) {
			int phys_reg = cointerval->intervals[i].phys_reg;
			if (phys_reg == -1)
				return;
			op->type = REG_PHYS;
			op->val.tmp_index = phys_reg;
			return;
		}
	}
}

void rewrite_registers(IRGraph *graph, CollectIntervals *cointerval)
{
	BasicBlock *current_block = graph->head;
	while (current_block != NULL) {
		Quadriple *current_quad = current_block->head;
		while (current_quad != NULL) {
			assign_physical_reg(current_quad->arg1, cointerval);
			assign_physical_reg(current_quad->arg2, cointerval);
			assign_physical_reg(current_quad->result, cointerval);

			if (current_quad == current_block->tail) {
				break;
			}
			current_quad = current_quad->next;
		}

		current_block = current_block->next_block;
	}
}
