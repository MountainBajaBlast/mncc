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
			return n + 1;
		}
		out[0] = enc_sdiv(ins->reg, ins->src1, ins->src2);
		return 1;

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
		out[0] = ins->is_num ? enc_cmp_num(ins->src1, ins->src2)
				     : enc_cmp_reg(ins->src1, ins->src2);
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


uint32_t enc_mov_reg(int regdest, int reg)
{
	return 0xAA0003E0 | ((uint32_t)reg << 16) | ((uint32_t)regdest << 0);
}

uint32_t enc_movz(int reg, uint16_t chunk, int chunk_num)
{
	return 0xD2800000 | ((uint32_t)(reg & 0x1F) << 0) |
	       ((uint32_t)(chunk & 0xFFFF) << 5) |
	       ((uint32_t)(chunk_num & 3) << 21);
}

uint32_t enc_movk(int reg, uint16_t chunk, int chunk_num)
{
	return 0xF2800000 | ((uint32_t)(reg & 0x1F) << 0) |
	       ((uint32_t)(chunk & 0xFFFF) << 5) |
	       ((uint32_t)(chunk_num & 3) << 21);
}

uint32_t enc_add_num(int regdest, int regn, int num)
{
	return 0x91000000 | ((uint32_t)(num & 0xFFF) << 10) |
	       ((uint32_t)regn << 5) | ((uint32_t)regdest << 0);
}

uint32_t enc_add_reg(int regdest, int regn, int regm)
{
	return 0x8B000000 | ((uint32_t)regm << 16) | ((uint32_t)regn << 5) |
	       ((uint32_t)regdest << 0);
}

uint32_t enc_sub_num(int regdest, int regn, int num)
{
	return 0xD1000000 | ((uint32_t)regn << 5) |
	       ((uint32_t)(num & 0xFFF) << 10) | ((uint32_t)regdest << 0);
}

uint32_t enc_sub_reg(int regdest, int regm, int regn)
{
	return 0xCB000000 | ((uint32_t)regm << 16) | ((uint32_t)regn << 5) |
	       ((uint32_t)regdest << 0);
}

uint32_t enc_mul(int regdest, int regn, int regm)
{
	return 0x9B007C00 | ((uint32_t)regm << 16) | ((uint32_t)regn << 5) |
	       ((uint32_t)regdest << 0);
}

uint32_t enc_sdiv(int regdest, int regn, int regm)
{
	return 0x9AC00C00 | ((uint32_t)regm << 16) | ((uint32_t)regn << 5) |
	       ((uint32_t)regdest << 0);
}

uint32_t enc_cmp_num(int regn, int num)
{
	return 0xF100001F | ((uint32_t)regn << 5) |
	       ((uint32_t)(num & 0xFFF) << 10);
}

uint32_t enc_cmp_reg(int regn, int regm)
{
	return 0xEB00001F | ((uint32_t)regn << 5) | ((uint32_t)regm << 16);
}

uint32_t enc_cset(int reg) { return 0x9A9F17E0 | ((uint32_t)reg & 0x1F); }

uint32_t enc_b(int offset)
{
	return 0x14000000 | ((uint32_t)offset & 0x3FFFFFF);
}

uint32_t enc_ret_reg(int rd, int rs) { return enc_mov_reg(rd, rs); }

uint32_t enc_cbnz(int reg, int offset)
{
	return 0xB5000000 | ((uint32_t)(offset & 0x7FFFF) << 5) |
	       ((uint32_t)reg & 0x1F);
}

struct mach_header_64 make_mach_header(int ncmds, int sizeofcmds)
{
	return (struct mach_header_64){
	    .magic = MH_MAGIC_64,
	    .cputype = CPU_TYPE_ARM64,
	    .filetype = MH_OBJECT,
	    .ncmds = ncmds,
	    .sizeofcmds = sizeofcmds,
	};
}

struct segment_command_64 make_segment(int nsects, uint64_t vmsize)
{
	return (struct segment_command_64){
	    .cmd = LC_SEGMENT_64,
	    .cmdsize = 72 + nsects * 80,
	    .segname = "__TEXT",
	    .vmsize = vmsize,
	    .filesize = vmsize,
	    .maxprot = 7,
	    .initprot = 7,
	    .nsects = nsects,
	};
}

struct section_64 make_section(uint32_t offset, uint64_t size)
{
	return (struct section_64){
	    .sectname = "__text",
	    .segname = "__TEXT",
	    .size = size,
	    .offset = offset,
	    .align = 2,
	};
}

struct symtab_command make_symtab(uint32_t symoff, int nsyms, uint32_t stroff,
				  uint32_t strsize)
{
	return (struct symtab_command){
	    .cmd = LC_SYMTAB,
	    .cmdsize = 24,
	    .symoff = symoff,
	    .nsyms = nsyms,
	    .stroff = stroff,
	    .strsize = strsize,
	};
}

struct nlist_64 make_nlist(uint32_t strx, uint64_t value)
{
	return (struct nlist_64){
	    .n_un.n_strx = strx,
	    .n_type = N_EXT | N_SECT,
	    .n_sect = 1,
	    .n_value = value,
	};
}

uint32_t *wrap_code(uint32_t *code, int code_size, int *out_nwords)
{
	int total = 2 + code_size + 3;
	uint32_t *text = calloc(total, sizeof(uint32_t));

	text[0] = 0xA9BF7BFD;
	text[1] = 0xAA0003FD;
	memcpy(text + 2, code, code_size * 4);
	text[code_size + 2] = 0xA8C17BFD;
	text[code_size + 3] = 0xD2800030;
	text[code_size + 4] = 0xD4001001;

	*out_nwords = total;

	return text;
}


void first_pass(RISCinstruct *insns, int count, int *label_addr, uint32_t *out,
		int *pos)
{
	*pos = 0;
	for (int i = 0; i < count; i++) {
		if (insns[i].op == OP_LABEL)
			label_addr[insns[i].target] = *pos;
		*pos += encode_one(&insns[i], *pos + out);
	}
}

void second_pass(RISCinstruct *insns, int count, int *label_addr, uint32_t *out,
		 int *pos)
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
			out[*pos] = enc_cbnz(
			    insns[i].src1, label_addr[insns[i].target] - *pos);
			out[*pos + 1] =
			    enc_b(label_addr[insns[i].target2] - (*pos + 1));
			*pos += 2;
			break;
		default:
			*pos += encode_one(&insns[i], *pos + out);
			break;
		}
	}
}

void write_object_file(RISCinstruct *insns, int count, const char *obj_path)
{
	int max_label = 0;
	for (int i = 0; i < count; i++)
		if (insns[i].op == OP_LABEL && insns[i].target > max_label)
			max_label = insns[i].target;

	int *label_addr = calloc(max_label + 1, sizeof(int));
	uint32_t *out = calloc(count * 8 + 8, sizeof(uint32_t));
	int pos;

	first_pass(insns, count, label_addr, out, &pos);
	second_pass(insns, count, label_addr, out, &pos);

	free(label_addr);

	int nwords;
	uint32_t *text = wrap_code(out, pos, &nwords);
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
