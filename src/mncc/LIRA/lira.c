#include "lira.h"

#define TRY_ALLOCATE(interval, reg_track, first_reg, last_reg)                     \
	do {                                                                       \
		for (int phys_reg = (first_reg); phys_reg <= (last_reg) &&         \
		     (interval)->phys_reg == -1; phys_reg++)                       \
			if ((reg_track)->phys_reg[phys_reg] <=                     \
			    (interval)->start_time) {                              \
				(interval)->phys_reg = phys_reg;                   \
				(reg_track)->phys_reg[phys_reg] =                  \
				    (interval)->end_time;                          \
			}                                                          \
	} while (0)

void greedy_allocate(CollectIntervals *cointerval, PhysRegTrack *reg_track)
{
	if (!cointerval || !cointerval->intervals)
		return;

	for (size_t interval_index = 0; interval_index < cointerval->intervals_count; interval_index++) {
		LivenessInterval *interval = &cointerval->intervals[interval_index];

		if (interval->start_time < 0)
			continue;

		if (interval->is_call)
			TRY_ALLOCATE(interval, reg_track, 19, 28);
		else {
			TRY_ALLOCATE(interval, reg_track, 0, 15);
			TRY_ALLOCATE(interval, reg_track, 19, 28);
		}
	}
}

static void assign_physical_reg(Operand *operand, CollectIntervals *cointerval)
{
	if (!operand || operand->type == NUM)
		return;

	for (size_t interval_index = 0; interval_index < cointerval->intervals_count; interval_index++) {
		LivenessInterval *interval = &cointerval->intervals[interval_index];

		if (interval->vreg_id != operand->val.tmp_index)
			continue;
		if (interval->phys_reg == -1)
			return;
		operand->type = REG_PHYS;
		operand->val.tmp_index = interval->phys_reg;
		return;
	}
}

void rewrite_registers(IRGraph *graph, CollectIntervals *cointerval)
{
	EACH_QUAD(basic_block, current_quad) {
		assign_physical_reg(current_quad->arg1, cointerval);
		assign_physical_reg(current_quad->arg2, cointerval);
		assign_physical_reg(current_quad->result, cointerval);
		if (current_quad->call)
			for (int argument = 0; argument < current_quad->call->argument_count; argument++)
				assign_physical_reg(current_quad->call->args[argument], cointerval);
	}

	for (int param = 0; param < graph->param_count; param++) {
		int vreg = graph->param_phys[param];
		graph->param_phys[param] = vreg;
		for (size_t interval_index = 0; interval_index < cointerval->intervals_count; interval_index++) {
			LivenessInterval *interval = &cointerval->intervals[interval_index];
			if (interval->vreg_id == vreg && interval->phys_reg != -1) {
				graph->param_phys[param] = interval->phys_reg;
				break;
			}
		}
	}
}
