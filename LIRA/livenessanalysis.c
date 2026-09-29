#include "lira.h"

#include <stdbool.h>

#define QUAD_INIT(max_regs) \
	(quad->use = create_bit_vector(max_regs), quad->def = create_bit_vector(max_regs))
#define QUAD_FREE() \
	(free_bit_vector(&quad->use), free_bit_vector(&quad->def))
#define BLOCK_INIT(max_regs) \
	(block->b_use = create_bit_vector(max_regs), block->b_def = create_bit_vector(max_regs), \
	 block->live_in = create_bit_vector(max_regs), block->live_out = create_bit_vector(max_regs))
#define BLOCK_FREE() \
	(free_bit_vector(&block->b_use), free_bit_vector(&block->b_def), \
	 free_bit_vector(&block->live_in), free_bit_vector(&block->live_out))

void init_quad_liveness(QuadLiveness *quad, size_t max_regs) { if (quad) QUAD_INIT(max_regs); }
void free_quadliveness(QuadLiveness *quad) { if (quad) QUAD_FREE(); }
void init_block_liveness(BlockLiveness *block, size_t max_regs) { if (block) BLOCK_INIT(max_regs); }
void free_blockliveness(BlockLiveness *block) { if (block) BLOCK_FREE(); }

static inline void process_operand(BitVector *bit_vector, const Operand *operand)
{
	if (operand && operand->type == VIR_REG)
		set_bit(bit_vector, operand->val.tmp_index);
}

void compute_local_liveness(IRGraph *graph, QuadLiveness *quad, BlockLiveness *block)
{
	if (!graph || !quad || !block)
		return;

	for (BasicBlock *basic_block = graph->head; basic_block; basic_block = basic_block->next_block)
		init_block_liveness(&block[basic_block->id_block], graph->reg_count);

	size_t quad_index = 0;
	EACH_QUAD(basic_block, current_quad) {
		size_t block_index = basic_block->id_block;
		init_quad_liveness(&quad[quad_index], graph->reg_count);
		process_operand(&quad[quad_index].use, current_quad->arg1);
		process_operand(&quad[quad_index].use, current_quad->arg2);
		process_operand(&quad[quad_index].def, current_quad->result);
		if (current_quad->call)
			for (int argument = 0; argument < current_quad->call->argument_count; argument++)
				process_operand(&quad[quad_index].use, current_quad->call->args[argument]);

		for (size_t byte = 0; byte < block[block_index].b_use.bytes_count; byte++) {
			block[block_index].b_use.data[byte] |= quad[quad_index].use.data[byte] & ~block[block_index].b_def.data[byte];
			block[block_index].b_def.data[byte] |= quad[quad_index].def.data[byte];
		}
		quad_index++;
	}
}

static bool block_live_in_changed(IRGraph *graph, QuadLiveness *quad, BlockLiveness *block,
				  BasicBlock *basic_block, size_t first_quad, size_t quad_count)
{
	size_t block_index = basic_block->id_block;
	BitVector live = create_bit_vector(graph->reg_count);
	bit_vector_union(&live, &block[block_index].live_out);
	for (size_t index = first_quad + quad_count; index-- > first_quad;) {
		bit_vector_diff(&live, &quad[index].def);
		bit_vector_union(&live, &quad[index].use);
	}
	bool changed = !bit_vector_equal(&block[block_index].live_in, &live);
	if (changed)
		bit_vector_copy(&block[block_index].live_in, &live);
	free_bit_vector(&live);
	return changed;
}

void compute_global_liveness(IRGraph *graph, QuadLiveness *quad, BlockLiveness *block)
{
	if (!graph || !quad || !block)
		return;

	bool changed;
	do {
		changed = false;
		size_t total_quads = 0;
		for (BasicBlock *basic_block = graph->head; basic_block; basic_block = basic_block->next_block) {
			size_t block_index = basic_block->id_block;
			size_t block_quads = 0;
			QUAD_COUNT(basic_block, block_quads);

			for (int successor = 0; successor < basic_block->succ_count; successor++)
				if (basic_block->successors[successor])
					bit_vector_union(&block[block_index].live_out,
							 &block[basic_block->successors[successor]->id_block].live_in);

			if (block_quads)
				changed |= block_live_in_changed(graph, quad, block, basic_block, total_quads, block_quads);
			else if (!bit_vector_equal(&block[block_index].live_in, &block[block_index].live_out)) {
				bit_vector_copy(&block[block_index].live_in, &block[block_index].live_out);
				changed = true;
			}
			total_quads += block_quads;
		}
	} while (changed);
}
