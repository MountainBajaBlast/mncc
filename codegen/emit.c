#include "enc.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

void first_pass(Instruction *insns, int count, int *label_addr, uint32_t *out, int *pos)
{
        *pos = 0;
        for (int i = 0; i < count; i++) {
                if (insns[i].op == OP_LABEL)
                        label_addr[insns[i].target] = *pos;
                *pos += encode_one(&insns[i], *pos + out);
        }
}

void second_pass(Instruction *insns, int count, int *label_addr, uint32_t *out, int *pos)
{
        *pos = 0;
        for (int i = 0; i < count; i++) {
                switch (insns[i].op) {
                case OP_LABEL:
                        break;
                case OP_JMP:
                        out[*pos] = enc_b(label_addr[insns[i].target] - *pos);
                        *pos += 1;
                        break;
                case OP_JE:
                        out[*pos] = enc_cbnz(insns[i].src1, label_addr[insns[i].target] - *pos);
                        out[*pos + 1] = enc_b(label_addr[insns[i].target2] - (*pos + 1));
                        *pos += 2;
                        break;
                case OP_CALL:
                        out[*pos] = enc_bl(label_addr[insns[i].target] - *pos);
                        out[*pos + 1] = enc_mov_reg(insns[i].reg, 0);
                        *pos += 2;
                        break;
                default:
                        *pos += encode_one(&insns[i], *pos + out);
                        break;
                }
        }
}

int split_bytes(Instruction *insns, int total_instructions, int *function_starts, char **function_names,
		int function_count, uint32_t *encoded_bytes, int *piece_borders, int *main_index)
{
	int current_word = 0;
	int next_function = 0;

	for (int instruction = 0; instruction < total_instructions; instruction++) {
		if (next_function < function_count && instruction == function_starts[next_function]) {
			piece_borders[next_function] = current_word;
			next_function++;
		}
		if (insns[instruction].op == OP_JMP) {
			current_word += 1;
		} else if (insns[instruction].op == OP_JE) {
			current_word += 2;
		} else if (insns[instruction].op == OP_CALL) {
			current_word += 2;
		} else {
			current_word += encode_one(&insns[instruction], encoded_bytes + current_word);
		}
	}
	piece_borders[function_count] = current_word;

	int found_main = -1;
	for (int function = 0; function < function_count; function++)
		if (strcmp(function_names[function], "main") == 0)
			found_main = function;

	if (found_main < 0) {
		fprintf(stderr, "Error: main not found");
		exit(1);
	}

	*main_index = found_main;
	return 0;
}

uint32_t *append_dead_functions(uint32_t *text, uint32_t *out, int *piece_borders, int function_count, int main_index,
				int *nwords)
{
	for (int function = 0; function < function_count; function++) {
		if (function == main_index)
			continue;

		int piece_start = piece_borders[function];
		int piece_length = piece_borders[function + 1] - piece_start;

		text = realloc(text, (*nwords + piece_length) * sizeof(uint32_t));
		memcpy(text + *nwords, out + piece_start, piece_length * sizeof(uint32_t));
		*nwords += piece_length;
	}

	return text;
}

