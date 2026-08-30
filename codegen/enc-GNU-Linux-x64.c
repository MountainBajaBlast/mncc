#include "enc-GNU-Linux-x64.h"
#include "../IR/ir.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



int encode_one(X64instruct *ins, X64functype *out)
{
	switch (ins->op) {
	case OP_MOV:
		if (ins->is_num) {
			out[0] = enc_mov_num(ins->reg, ins->num);
			return 1;
		}
		out[0] = enc_mov_reg(ins->reg, ins->src1);
		return 1;
	case OP_ADD:
		if (ins->is_num) {
			out[0] = enc_add_num(ins->reg, ins->src1, ins->src2);
			return 1;
		}
		out[0] = enc_add_reg(ins->reg, ins->src1, ins->src2);
		return 1;
	case OP_SUB:
		if (ins->is_num) {
			out[0] = enc_sub_num(ins->reg, ins->src1, ins->src2);
			return 1;
		}
		out[0] = enc_sub_reg(ins->reg, ins->src1, ins->src2);
		return 1;
	case OP_MUL:
		if (ins->is_num) {
			out[0] = enc_mul(ins->reg, ins->src1, ins->src2);
			return 1;
		}
		out[0] = enc_mul(ins->reg, ins->src1, ins->src2);
		return 1;

	case OP_DIV:
		if (ins->is_num) {
			out[0] = enc_cqo();
			out[1] = enc_idiv(ins->src1);
			return 2;
		}
		out[0] = enc_cqo();
		out[1] = enc_idiv(ins->reg);
		return 2;
	case OP_RET:
		if (ins->is_num) {
			out[0] = enc_ret_num(ins->src1);
		return 1;
                }
                else
		out[0] = enc_ret_reg(0, ins->src1);
		return 1;

	case OP_CMP:
		out[0] = ins->is_num ? enc_cmp_num(ins->src1, ins->src2) : enc_cmp_reg(ins->src1, ins->src2);
		return 1;
	case OP_JMP:
		out[0] = enc_jmp(ins->target);
		return 1;
	case OP_JE:
		out[0] = enc_je(ins->target);
		return 1;
	case OP_LABEL:
		return 0;
	default:
		return 0;
	}
}

void first_pass(X64instruct *insns, int count, int *label_addr, X64functype  *out, int *pos)
{
	*pos = 0;
	for (int i = 0; i < count; i++) {
		if (insns[i].op == OP_LABEL)
			label_addr[insns[i].target] = *pos;
		*pos += encode_one(&insns[i], *pos + out);
	}
}

void second_pass(X64instruct *insns, int count, int *label_addr, X64functype *out, int *pos)
{
	*pos = 0;
	for (int i = 0; i < count; i++) {
		switch (insns[i].op) {
		case OP_LABEL:
			break;
		case OP_JMP:
			out[*pos] = enc_jmp(label_addr[insns[i].target] - *pos);
			*pos += 1;
			break;
		case OP_JE:
			out[*pos] = enc_je(label_addr[insns[i].target2] - (*pos + 1));
			*pos += 2;
			break;
		default:
			*pos += encode_one(&insns[i], *pos + out);
			break;
		}
	}
}

int split_bytes(X64instruct *insns, int total_instructions, int *function_starts, char **function_names,
		int function_count, X64functype *encoded_bytes, int *piece_borders, int *main_index)
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

uint32_t *append_dead_functions(X64functype *text, X64functype *out, int *piece_borders, int function_count, int main_index,
				int *nwords)
{
	for (int function = 0; function < function_count; function++) {
		if (function == main_index)
			continue;

		int piece_start = piece_borders[function];
		int piece_length = piece_borders[function + 1] - piece_start;

		text = realloc(text, (*nwords + piece_length) * sizeof(X64functype));
		memcpy(text + *nwords, out + piece_start, piece_length * sizeof(X64functype));
		*nwords += piece_length;
	}

	return text;
}


void write_object_file(X64instruct *insns, int count, const char *obj_path, int *function_starts, char **function_names,
		       int function_count)
{
	int max_label = 0;
	for (int i = 0; i < count; i++) {
		if (insns[i].op == OP_LABEL && insns[i].target > max_label) {
			max_label = insns[i].target;
		}
	}

	int *label_addr = calloc(max_label + 1, sizeof(int));
	X64functype *out = calloc(count * 8 + 8, sizeof(X64functype));
	int pos;

	first_pass(insns, count, label_addr, out, &pos);
	second_pass(insns, count, label_addr, out, &pos);

	int piece_borders[function_count + 1];
	int main_index;

	split_bytes(insns, count, function_starts, function_names, function_count, out, piece_borders, &main_index);
	free(label_addr);

	int main_start = piece_borders[main_index];
	int nwords = piece_borders[main_index + 1] - main_start;
	X64functype text = append_dead_functions(out + main_start, out, piece_borders, function_count, main_index, &nwords);
	free(out);

	int text_bytes = nwords * 4;
	int text_off = 64;
	int sym_off = text_off + text_bytes;
	int str_off = sym_off + 24;
	int shstr_off = str_off + 6;
	int shdr_off = shstr_off + 33;

	Elf64_Ehdr eh = make_elf_header(shdr_off, 4);

	Elf64_Shdr shdr[4] = {
	    make_text_section(text_off, text_bytes),
	    make_symtab_section(sym_off, 24),
	    make_strtab_section(14, str_off, 6),
	    make_strtab_section(22, shstr_off, 33),
	};

	Elf64_Sym sym = make_symbol(0, 0, text_bytes);

	char strtab[] = {'m', 'a', 'i', 'n', '\0'};
	char shstrtab[] = ".text\0.symtab\0.strtab\0.shstrtab\0";


	FILE *f = fopen(obj_path, "wb");
	if (!f) {
		fprintf(stderr, "Fatal: Cannot open %s\n", obj_path);
		free(text);
		exit(1);
	}

	fwrite(&eh, sizeof(eh), 1, f);
	fwrite(text, 1, text_bytes, f);
	fwrite(&sym, sizeof(sym), 1, f);
	fwrite(strtab, 1, 6, f);
	fwrite(shstrtab, 1, 33, f);
	fwrite(shdr, sizeof(shdr[0]), 4, f);
}
