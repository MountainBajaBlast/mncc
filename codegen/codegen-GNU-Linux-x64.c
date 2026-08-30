#include "codegen-GNU-Linux-x64.h"
#include "enc-GNU-Linux-x64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../IR/ir.h"


void compile_to_binary(IRGraph *graph, X64instruct **out_insns, int *out_count)
{
        if (!graph || !graph->head) {
                *out_insns = NULL;
                *out_count = 0;
                return;
        }

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

        RISCinstruct *insns =
            malloc((total_instr + graph->block_count) * sizeof(RISCinstruct));
        if (!insns) {
                fprintf(stderr, "Fatal: Out of memory in codegen\n");
                exit(1);
        }

        int idx = 0;
        current_block = graph->head;
        while (current_block != NULL) {
                insns[idx].op = OP_LABEL;
                insns[idx].target = current_block->id_block;
                insns[idx].is_num = 0;
                insns[idx].src1 = 0;
                insns[idx].src2 = 0;
                insns[idx].reg = 0;
                insns[idx].target2 = 0;
                idx++;

                Quadriple *quad = current_block->head;
                int emitted_jump = 0;
                while (quad != NULL) {
                        insns[idx].is_num = 0;
                        insns[idx].src1 = 0;
                        insns[idx].src2 = 0;
                        insns[idx].reg = 0;
                        insns[idx].target = 0;
                        insns[idx].target2 = 0;
                        

			switch (quad->operation) {
                        case ADD:
                                insns[idx].op = OP_ADD;
                                break;
                        case SUB:
                                insns[idx].op = OP_SUB;
                                break;
                        case MULT:
                                insns[idx].op = OP_MUL;
                                break;
                        case DIV:
                                insns[idx].op = OP_DIV;
                                break;
                        case ASSIGN:
                                insns[idx].op = OP_MOV;
                                break;
                        case RET:
                                insns[idx].op = OP_RET;
                                break;
                        case CMP:
                                insns[idx].op = OP_CMP;
                                break;
                        case JMP:
                                insns[idx].op = OP_JMP;
                                if (current_block->jmp_target)
                                        insns[idx].target =
                                            current_block->jmp_target->id_block;
                                emitted_jump = 1;
                                break;
                        case JE:
                                insns[idx].op = OP_JE;
                                if (current_block->true_target_je)
                                        insns[idx].target =
                                            current_block->true_target_je
                                                ->id_block;
                                if (current_block->false_target_je)
                                        insns[idx].target2 =
                                            current_block->false_target_je
                                                ->id_block;
                                emitted_jump = 1;
                                break;
                        default:
                                break;
                        }

                        if (quad->arg1) {
                                if (quad->arg1->type == NUM) {
                                        insns[idx].is_num = 1;
                                        insns[idx].src1 =
                                            (int)quad->arg1->val.val_int;
                                } else {
                                        insns[idx].src1 =
                                            quad->arg1->val.tmp_index;
                                }
                        }

                        if (quad->arg2) {
                                if (quad->arg2->type == NUM) {
				     insns[idx].is_num = 1;
                                        insns[idx].src2 =
                                            (int)quad->arg2->val.val_int;
                                } else {
                                        insns[idx].src2 =
                                            quad->arg2->val.tmp_index;
                                }
                        }

                        if (quad->result) {
                                insns[idx].reg = quad->result->val.tmp_index;
                        }

                        if (insns[idx].is_num && quad->arg1 &&
                            quad->arg1->type == NUM && quad->arg2 &&
                            quad->arg2->type != NUM &&
                            (quad->operation == ADD ||
                             quad->operation == MULT ||
                             quad->operation == CMP)) {
                                int tmp = insns[idx].src1;
                                insns[idx].src1 = insns[idx].src2;
                                insns[idx].src2 = tmp;
                        }

                        idx++;
                        if (quad == current_block->tail)
                                break;
                        quad = quad->next;
                }

                if (!emitted_jump && current_block->terminator == JMP &&
                    current_block->jmp_target) {
                        insns[idx].op = OP_JMP;
                        insns[idx].target = current_block->jmp_target->id_block;
                        insns[idx].is_num = 0;
                        insns[idx].src1 = 0;
                        insns[idx].src2 = 0;
                        insns[idx].reg = 0;
                        insns[idx].target2 = 0;
                        idx++;
                }
                current_block = current_block->next_block;
        }

        *out_insns = insns;
        *out_count = idx;
}

void compile_program_to_binary(IRProgram *program,
                               X64instruct **out_insns, int *out_count,
                               int **out_starts, char ***out_names,
                               int *out_function_count)
{
        RISCinstruct *merged = NULL;
        int total_instructions = 0;
        int label_base = 0;

        int *starts = calloc(program->count, sizeof(int));
        char **names = calloc(program->count, sizeof(char *));

	for (int i = 0; i < program->count; i++) {
                IRGraph *current = program->graphs[i];

                for (int block = 0; block < current->block_count; block++)
                        current->blocks[block]->id_block += label_base;
                label_base += current->block_count;

                RISCinstruct *piece = NULL;
                int piece_count = 0;
                compile_to_binary(current, &piece, &piece_count);

                starts[i] = total_instructions;
                names[i] = current->func_name;

                if (piece_count > 0) {
                        merged = realloc(merged,
                                         (total_instructions + piece_count) *
                                             sizeof(RISCinstruct));
                        memcpy(merged + total_instructions, piece,
                               piece_count * sizeof(RISCinstruct));
                        total_instructions += piece_count;
                        free(piece);
                }
        }

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
                 "cc %s -o %s",
                 obj_path, output_path);
        system(cmd);
}
                                                                                                                                                                                                                                                            

