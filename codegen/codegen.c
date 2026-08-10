#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../IR/ir.h"

void in_asm(FILE *out, RISCinstruct *inst, int count)
{
	fprintf(out, "    .global _main\n");
	fprintf(out, "     .align 4\n");
	fprintf(out, "  _main:\n");

	fprintf(out, "    stp x29, x30, [sp, #-16]!\n");
	fprintf(out, "    mov x29, sp\n");

	for (int i = 0; i < count; i++) {
		switch (inst[i].op) {
		case ASM_MOV:
			if (inst[i].is_imm) {
				fprintf(out, "  mov w%d, #%d\n", inst[i].reg,
					inst[i].src1);
			} else {
				fprintf(out, "  mov w%d, w%d\n", inst[i].reg,
					inst[i].src1);
			}
			break;
		case ASM_ADD:
			fprintf(out, "  add w%d, w%d, w%d\n", inst[i].reg,
				inst[i].src1, inst[i].src2);
			break;
		case ASM_SUB:
			fprintf(out, "  sub w%d, w%d, w%d\n", inst[i].reg,
				inst[i].src1, inst[i].src2);
			break;
		case ASM_MUL:
			fprintf(out, "  mul w%d, w%d, w%d\n", inst[i].reg,
				inst[i].src1, inst[i].src2);
			break;
		case ASM_DIV:
			fprintf(out, "  sdiv w%d, w%d, w%d\n", inst[i].reg,
				inst[i].src1, inst[i].src2);
			fprintf(out, "  msub w1, w%d, w%d, w%d\n", inst[i].reg,
				inst[i].src2, inst[i].src1);
			break;
		case ASM_RET:
			if (inst[i].is_imm) {
				fprintf(out, "  mov w0, #%d\n", inst[i].src1);
			} else if (inst[i].src1 != 0) {
				fprintf(out, "  mov w0, w%d\n", inst[i].src1);
			}
			break;

		case ASM_LABEL:
			fprintf(out, "L_%d:\n", inst[i].target);
			break;
		case ASM_CMP:
			if (inst[i].is_imm) {
				fprintf(out, "  cmp w%d, #%d\n", inst[i].src1,
					inst[i].src2);
			} else {
				fprintf(out, "  cmp w%d, w%d\n", inst[i].src1,
					inst[i].src2);
			}
			fprintf(out, "  cset w%d, eq\n ", inst[i].reg);
			break;
		case ASM_JMP:
			fprintf(out, " b L_%d\n", inst[i].target);
			break;
		case ASM_JE:
			fprintf(out, "  cbnz w%d, L_%d\n", inst[i].src1,
				inst[i].target);
			fprintf(out, "  b L_%d\n", inst[i].target2);
			break;
		}
	}

	fprintf(out, "    ldp x29, x30, [sp], #16\n");
	fprintf(out, "    mov x16, #1\n");
	fprintf(out, "    svc #0x80\n");
}

void compile_to_binary(IRGraph *graph, const char *output_name)
{
	if (!graph || !graph->head)
		return;

	int total_instr = 0;
	BasicBlock *current_block = graph->head;
	while (current_block != NULL) {
		total_instr++;
		Quadriple *quad = current_block->head;
		while (quad != NULL) {
			total_instr++;
			if (quad == current_block->tail)
				break;
			quad = quad->next;
		}
		current_block = current_block->next_block;
	}

	RISCinstruct *asm_code =
	    malloc((total_instr + graph->block_count) * sizeof(RISCinstruct));
	if (!asm_code) {
		fprintf(stderr, "Fatal: Out of memory in codegen\n");
		exit(1);
	}

	int idx = 0;
	current_block = graph->head;
	while (current_block != NULL) {
		asm_code[idx].op = ASM_LABEL;
		asm_code[idx].target = current_block->id_block;
		asm_code[idx].is_imm = 0;
		asm_code[idx].src1 = 0;
		asm_code[idx].src2 = 0;
		asm_code[idx].reg = 0;
		asm_code[idx].target2 = 0;
		idx++;

		Quadriple *quad = current_block->head;
		int emitted_jump = 0;
		while (quad != NULL) {
			asm_code[idx].is_imm = 0;
			asm_code[idx].src1 = 0;
			asm_code[idx].src2 = 0;
			asm_code[idx].reg = 0;
			asm_code[idx].target = 0;
			asm_code[idx].target2 = 0;

			switch (quad->operation) {
			case ADD:
				asm_code[idx].op = ASM_ADD;
				break;
			case SUB:
				asm_code[idx].op = ASM_SUB;
				break;
			case MULT:
				asm_code[idx].op = ASM_MUL;
				break;
			case DIV:
				asm_code[idx].op = ASM_DIV;
				break;
			case ASSIGN:
				asm_code[idx].op = ASM_MOV;
				break;
			case RET:
				asm_code[idx].op = ASM_RET;
				break;
			case CMP:
				asm_code[idx].op = ASM_CMP;
				break;
			case JMP:
				asm_code[idx].op = ASM_JMP;
				if (current_block->jmp_target)
					asm_code[idx].target =
					    current_block->jmp_target->id_block;
				emitted_jump = 1;
				break;
			case JE:
				asm_code[idx].op = ASM_JE;
				if (current_block->true_target_je)
					asm_code[idx].target =
					    current_block->true_target_je
						->id_block;
				if (current_block->false_target_je)
					asm_code[idx].target2 =
					    current_block->false_target_je
						->id_block;
				emitted_jump = 1;
				break;
			default:
				break;
			}

			if (quad->arg1) {
				if (quad->arg1->type == NUM) {
					asm_code[idx].is_imm = 1;
					asm_code[idx].src1 =
					    (int)quad->arg1->val.val_int;
				} else {
					asm_code[idx].src1 =
					    quad->arg1->val.tmp_index;
				}
			}

			if (quad->arg2) {
				if (quad->arg2->type == NUM) {
					asm_code[idx].is_imm = 1;
					asm_code[idx].src2 =
					    (int)quad->arg2->val.val_int;
				} else {
					asm_code[idx].src2 =
					    quad->arg2->val.tmp_index;
				}
			}

			if (quad->result) {
				asm_code[idx].reg = quad->result->val.tmp_index;
			}

			idx++;
			if (quad == current_block->tail)
				break;
			quad = quad->next;
		}

		if (!emitted_jump && current_block->terminator == JMP &&
		    current_block->jmp_target) {
			asm_code[idx].op = ASM_JMP;
			asm_code[idx].target =
			    current_block->jmp_target->id_block;
			asm_code[idx].is_imm = 0;
			asm_code[idx].src1 = 0;
			asm_code[idx].src2 = 0;
			asm_code[idx].reg = 0;
			asm_code[idx].target2 = 0;
			idx++;
		}
		current_block = current_block->next_block;
	}

	FILE *asm_file = fopen("temp_output.s", "w");
	if (!asm_file) {
		fprintf(stderr, "Fatal: Cannot create assembly file\n");
		free(asm_code);
		exit(1);
	}

	in_asm(asm_file, asm_code, idx);
	fclose(asm_file);
	free(asm_code);

	char cmd[512];
	snprintf(cmd, sizeof(cmd), "clang -arch arm64 temp_output.s -o %s",
		 output_name);

	int res = system(cmd);
	remove("temp_output.s");

	if (res != 0) {
		fprintf(stderr, "Fatal: Assembler failed\n");
		exit(1);
	}
}
