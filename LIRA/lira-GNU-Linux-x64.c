#include "lira.h"

void greedy_allocate(CollectIntervals *cointerval, PhysRegTrack *reg_track)
{
	if (cointerval == NULL || cointerval->intervals == NULL) {
		return;
	}

	for (size_t reg_idx = 0; reg_idx < cointerval->intervals_count; reg_idx++) {
		LivenessInterval *current = &cointerval->intervals[reg_idx];

		for (size_t i = 0; i < 16; i++) {
			if (i == 0 || i == 2 || i == 4 || i == 5 || i == 11 || i == 15)
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
