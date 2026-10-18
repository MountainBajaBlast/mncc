#include "enc.h"
#include "../LIRA/lira.h"
#include <stdio.h>
#include <string.h>
#define EACH_BLOCK(current_block)                                               \
        for (BasicBlock *current_block = graph->head; current_block;              \
             current_block = current_block->next_block)




static const oper quad_to_oper[] = {
	[ADD] = OP_ADD,		[RET] = OP_RET,		[CMP] = OP_CMP,
	[JMP] = OP_JMP,		[JE] = OP_JE,		[DIV] = OP_DIV,
	[MULT] = OP_MULT,	[SUB] = OP_SUB,		[UNARY] = OP_NEG,
	[CALL] = OP_CALL,	[ASSIGN] = OP_MOV,
};

static char **func_names;
static int *func_entries;
static int func_count;

static int resolve_func(const char *name)
{
	for (int i = 0; i < func_count; i++) {
		if (strcmp(func_names[i], name) == 0)
			return func_entries[i];
	}
	fprintf(stderr, "Fatal: Call to unknown function %s\n", name);
	exit(1);
}


static int count_instructions(IRGraph *graph)
{
	int count = 0;
	int ret_count = 0;
	int has_call = 0;
	count += graph->param_count;
	EACH_BLOCK(block) {
		count++;
		int emitted_jump = 0;
		for (Quadriple *quad = block->head; quad; quad = quad->next) {
			count++;
			if (quad->operation == CALL) {
				has_call = 1;
				count += quad->call->argument_count;
			}
			if (quad->operation == RET)
				ret_count++;
			if (quad->operation == JMP || quad->operation == JE)
				emitted_jump = 1;
		}
		if (!emitted_jump && block->terminator == JMP && block->jmp_target)
			count++;
	}
	if (has_call)
		count += 1 + ret_count;
	return count;
}

static void write_label(Instruction *dst, int id)
{
	dst->op = OP_LABEL;
	dst->target = id;
	dst->src1 = 0;
	dst->src2 = 0;
	dst->reg = 0;
	dst->is_num = 0;
	dst->is_num2 = 0;
	dst->target2 = 0;
}

static void set_terminator(Instruction *dst, BasicBlock *block, Quadriple *quad, int *emitted_jump)
{
	if (quad->operation == JMP) {
		dst->target = block->jmp_target->id_block;
		*emitted_jump = 1;
	} else if (quad->operation == JE) {
		dst->target = block->true_target_je->id_block;
		dst->target2 = block->false_target_je->id_block;
		*emitted_jump = 1;
	}
}

static void write_terminator(Instruction *dst, BasicBlock *block)
{
	dst->op = OP_JMP;
	dst->target = block->jmp_target->id_block;
	dst->src1 = 0;
	dst->src2 = 0;
	dst->reg = 0;
	dst->is_num = 0;
	dst->is_num2 = 0;
	dst->target2 = 0;
}

static void fill_operand(Operand *op, int *src, int *is_num)
{
	if (op->type == NUM) {
		*is_num = 1;
		*src = (int)op->val.val_int;
	} else {
		*is_num = 0;
		*src = op->val.tmp_index;
	}
}

static void set_result(Operand *result, Instruction *dst)
{
	dst->reg = result->val.tmp_index;
}

static void emit_call(Instruction *insns, Quadriple *quad, int (*resolve)(const char *), int *idx)
{
	for (int arg = 0; arg < quad->call->argument_count; arg++) {
		insns[*idx].op = OP_MOV;
		insns[*idx].reg = arg;
		fill_operand(quad->call->args[arg], &insns[*idx].src1, &insns[*idx].is_num);
		(*idx)++;
	}

	insns[*idx] = (Instruction){0};
	insns[*idx].op = OP_CALL;
	insns[*idx].target = resolve(quad->call->FuncName);
	if (quad->result)
		set_result(quad->result, &insns[*idx]);
	(*idx)++;
}

void compile_to_binary(IRGraph *graph, int (*resolve)(const char *), Instruction **out_insns, int *out_count,
		       int slot_reg)
{
	if (!graph || !graph->head) {
		*out_insns = NULL;
		*out_count = 0;
		return;
	}

	int total = count_instructions(graph);

	Instruction *insns = malloc((size_t)total * sizeof(Instruction));
	if (!insns) {
		fprintf(stderr, "Fatal: Out of memory in codegen\n");
		exit(1);
	}

	int idx = 0;
	int first_block = 1;
	EACH_BLOCK(block) {
		write_label(&insns[idx], block->id_block);
		idx++;

		if (first_block && slot_reg >= 0) {
			insns[idx] = (Instruction){0};
			insns[idx].op = OP_MOV;
			insns[idx].reg = slot_reg;
			insns[idx].src1 = 30;
			insns[idx].is_num = 0;
			idx++;
		}

		if (first_block) {
			for (int p = 0; p < graph->param_count; p++) {
				insns[idx] = (Instruction){0};
				insns[idx].op = OP_MOV;
				insns[idx].reg = graph->param_phys[p];
				insns[idx].src1 = p;
				insns[idx].is_num = 0;
				idx++;
			}
			first_block = 0;
		}

		int emitted_jump = 0;
		for (Quadriple *quad = block->head; quad; quad = quad->next) {
			if (quad->operation == CALL) {
				emit_call(insns, quad, resolve, &idx);
				continue;
			}

			if (quad->operation == RET && slot_reg >= 0) {
				insns[idx] = (Instruction){0};
				insns[idx].op = OP_MOV;
				insns[idx].reg = 30;
				insns[idx].src1 = slot_reg;
				insns[idx].is_num = 0;
				idx++;
			}

			insns[idx] = (Instruction){0};
			insns[idx].op = quad_to_oper[quad->operation];
			set_terminator(&insns[idx], block, quad, &emitted_jump);
			if (quad->arg1)
				fill_operand(quad->arg1, &insns[idx].src1, &insns[idx].is_num);
			if (quad->arg2)
				fill_operand(quad->arg2, &insns[idx].src2, &insns[idx].is_num2);
			if (quad->result)
				set_result(quad->result, &insns[idx]);
			idx++;
		}

		if (!emitted_jump && block->terminator == JMP && block->jmp_target) {
			write_terminator(&insns[idx], block);
			idx++;
		}
	}

	*out_insns = insns;
	*out_count = idx;
}



static int graph_has_call(IRGraph *graph)
{
	EACH_BLOCK(block) {
		for (Quadriple *quad = block->head; quad; quad = quad->next)
			if (quad->operation == CALL)
				return 1;
	}
	return 0;
}

void compile_program_to_binary(IRProgram *program, Instruction  **out_insns, int *out_count, int **out_starts,
			       char ***out_names, int *out_function_count)
{
	int *order = calloc(program->count, sizeof(int));
	int main_pos = -1;
	for (int i = 0; i < program->count; i++) {
		if (strcmp(program->graphs[i]->func_name, "main") == 0) {
			main_pos = i;
			break;
		}
	}

	int order_index = 0;
	if (main_pos >= 0)
		order[order_index++] = main_pos;
	for (int i = 0; i < program->count; i++)
		if (i != main_pos)
			order[order_index++] = i;

	Instruction *merged = NULL;
	int total_instructions = 0;
	int label_base = 0;

	int *starts = calloc(program->count, sizeof(int));
	char **names = calloc(program->count, sizeof(char *));
	int *slots = calloc(program->count, sizeof(int));

	func_names = names;
	func_entries = calloc(program->count, sizeof(int));
	func_count = program->count;

	int slot_pool[3] = {16, 17, 18};
	int slot_used = 0;
	for (int o = 0; o < program->count; o++) {
		IRGraph *current = program->graphs[order[o]];
		if (graph_has_call(current)) {
			if (slot_used >= 3) {
				fprintf(stderr, "Fatal: too many nested calls, no free slot register\n");
				exit(1);
			}
			slots[o] = slot_pool[slot_used++];
		} else {
			slots[o] = -1;
		}
	}

	for (int o = 0; o < program->count; o++) {
		IRGraph *current = program->graphs[order[o]];

		for (int block = 0; block < current->block_count; block++)
			current->blocks[block]->id_block += label_base;
		label_base += current->block_count;

		names[o] = current->func_name;
		func_entries[o] = current->blocks[0]->id_block;
	}

	for (int o = 0; o < program->count; o++) {
		IRGraph *current = program->graphs[order[o]];

		Instruction *piece = NULL;
		int piece_count = 0;
		compile_to_binary(current, resolve_func, &piece, &piece_count, slots[o]);

		starts[o] = total_instructions;
		names[o] = current->func_name;

		if (piece_count > 0) {
			merged = realloc(merged, (total_instructions + piece_count) * sizeof(Instruction));
			memcpy(merged + total_instructions, piece, piece_count * sizeof(Instruction));
			total_instructions += piece_count;
			free(piece);
		}
	}

	free(order);
	free(func_entries);

	*out_insns = merged;
	*out_count = total_instructions;
	*out_starts = starts;
	*out_names = names;
	*out_function_count = program->count;
}




void link_object(const char *obj_path, const char *output_path)
{
	char cmd[512];
	snprintf(cmd, sizeof cmd,
		 "ld -arch arm64 %s -o %s -lSystem -e _main "
		 "-platform_version macos 14.0.0 14.0.0 "
		 "-L$(xcrun --show-sdk-path)/usr/lib",
		 obj_path, output_path);
	system(cmd);
}
