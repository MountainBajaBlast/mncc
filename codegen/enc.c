#include "enc.h"
#include "../IR/ir.h"
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int enc_load_num(int reg, uint64_t num, uint32_t *out)
{
	int n = 0;
	out[n++] = enc_movz(reg, (uint16_t)num, 0);
	for (int hw = 1; hw < 4; hw++) {
		uint16_t chunk = (uint16_t)(num >> (hw * 16));
		if (chunk)
			out[n++] = enc_movk(reg, chunk, hw);
	}
	return n;
}



int encode_one(RISCinstruct *ins, uint32_t *out)
{
	switch (ins->op) {
	case OP_MOV:
		if (ins->is_num)
			return enc_load_num(ins->reg, ins->src1, out);
		out[0] = enc_mov_reg(ins->reg, ins->src1);
		return 1;

	case OP_ADD:
		if (ins->is_num && ins->src2 > 0xFFF) {
			int n = enc_load_num(16, ins->src2, out);
			out[n] = enc_add_reg(ins->reg, ins->src1, 16);
			return n + 1;
		}
		if (ins->is_num) {
			out[0] = enc_add_num(ins->reg, ins->src1, ins->src2);
			return 1;
		}
		out[0] = enc_add_reg(ins->reg, ins->src1, ins->src2);
		return 1;

	case OP_SUB:
		if (ins->is_num && ins->src2 > 0xFFF) {
			int n = enc_load_num(16, ins->src2, out);
			out[n] = enc_sub_reg(ins->reg, ins->src1, 16);
			return n + 1;
		}
		if (ins->is_num) {
			out[0] = enc_sub_num(ins->reg, ins->src1, ins->src2);
			return 1;
		}
		out[0] = enc_sub_reg(ins->reg, ins->src1, ins->src2);
		return 1;

	case OP_MUL:
		if (ins->is_num) {
			int n = enc_load_num(16, ins->src2, out);
			out[n] = enc_mul(ins->reg, ins->src1, 16);
			return n + 1;
		}
		out[0] = enc_mul(ins->reg, ins->src1, ins->src2);
		return 1;

	case OP_DIV:
		if (ins->is_num) {
			int n = enc_load_num(16, ins->src2, out);
			out[n] = enc_sdiv(ins->reg, ins->src1, 16);
			out[n + 1] = enc_msub(16, ins->reg, 16, ins->src1);
			return n + 2;
		}
		out[0] = enc_sdiv(ins->reg, ins->src1, ins->src2);
		out[1] = enc_msub(16, ins->reg, ins->src1, ins->src2);
		return 2;

	case OP_RET:
		if (ins->is_num)
			return enc_load_num(0, ins->src1, out);
		out[0] = enc_ret_reg(0, ins->src1);
		return 1;

	case OP_CMP:
		if (ins->is_num && ins->src2 > 0xFFF) {
			int n = enc_load_num(16, ins->src2, out);
			out[n] = enc_cmp_reg(ins->src1, 16);
			out[n + 1] = enc_cset(ins->reg);
			return n + 2;
		}
		out[0] = ins->is_num ? enc_cmp_num(ins->src1, ins->src2) : enc_cmp_reg(ins->src1, ins->src2);
		out[1] = enc_cset(ins->reg);
		return 2;

	case OP_JMP:
		out[0] = enc_b(ins->target);
		return 1;

	case OP_JE:
		out[0] = enc_cbnz(ins->src1, ins->target);
		out[1] = enc_b(ins->target2);
		return 2;

	case OP_LABEL:
		return 0;

	default:
		return 0;
	}
}




void first_pass(RISCinstruct *insns, int count, int *label_addr, uint32_t *out, int *pos)
{
	*pos = 0;
	for (int i = 0; i < count; i++) {
		if (insns[i].op == OP_LABEL)
			label_addr[insns[i].target] = *pos;
		*pos += encode_one(&insns[i], *pos + out);
	}
}

void second_pass(RISCinstruct *insns, int count, int *label_addr, uint32_t *out, int *pos)
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
		default:
			*pos += encode_one(&insns[i], *pos + out);
			break;
		}
	}
}

int split_bytes(RISCinstruct *insns, int total_instructions, int *function_starts, char **function_names,
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


void write_object_file(RISCinstruct *insns, int count, const char *obj_path, int *function_starts,
		       char **function_names, int function_count)
{
	int max_label = 0;
	for (int i = 0; i < count; i++) {
		if (insns[i].op == OP_LABEL && insns[i].target > max_label) {
			max_label = insns[i].target;
		}
	}

	int *label_addr = calloc(max_label + 1, sizeof(int));
	uint32_t *out = calloc(count * 8 + 8, sizeof(uint32_t));
	int pos;

	first_pass(insns, count, label_addr, out, &pos);
	second_pass(insns, count, label_addr, out, &pos);

	int piece_borders[function_count + 1];
	int main_index;

	split_bytes(insns, count, function_starts, function_names, function_count, out, piece_borders, &main_index);

	free(label_addr);

	int main_start = piece_borders[main_index];
	int nwords;
	uint32_t *text = wrap_code(out + main_start, piece_borders[main_index + 1] - main_start, &nwords);
	text = append_dead_functions(text, out, piece_borders, function_count, main_index, &nwords);
	free(out);

	uint32_t code_size = nwords * 4;
	uint32_t hdrs_size = 32 + 152 + 24;
	uint32_t symoff = hdrs_size + code_size;
	uint32_t stroff = symoff + sizeof(struct nlist_64);

	struct mach_header_64 hdr = make_mach_header(2, hdrs_size - 32);
	struct segment_command_64 seg = make_segment(1, code_size);
	struct section_64 sec = make_section(hdrs_size, code_size);
	struct symtab_command st = make_symtab(symoff, 1, stroff, 8);
	struct nlist_64 nl = make_nlist(1, 0);

	FILE *f = fopen(obj_path, "wb");
	if (!f) {
		fprintf(stderr, "Fatal: Cannot open %s\n", obj_path);
		free(text);
		exit(1);
	}

	fwrite(&hdr, sizeof(hdr), 1, f);
	fwrite(&seg, sizeof(seg), 1, f);
	fwrite(&sec, sizeof(sec), 1, f);
	fwrite(&st, sizeof(st), 1, f);
	fwrite(text, 4, nwords, f);
	fwrite(&nl, sizeof(nl), 1, f);
	char strtab[8] = "\0_main\0";
	fwrite(strtab, 1, 8, f);
	fclose(f);
	free(text);
}
