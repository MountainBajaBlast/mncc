#include "enc.h"

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

int enc_add(Instruction *ins, uint32_t *out)
{
	if (ins->is_num2 && ins->src2 > 0xFFF) {
			int n = enc_load_num(16, ins->src2, out);
			out[n] = enc_add_reg(ins->reg, ins->src1, 16);
			return n + 1;
		}
		if (ins->is_num2) {
			out[0] = enc_add_num(ins->reg, ins->src1, ins->src2);
			return 1;
		}
		if (ins->is_num) {
			int n = enc_load_num(16, ins->src1, out);
			out[n] = enc_add_reg(ins->reg, 16, ins->src2);
			return n + 1;
		}
		out[0] = enc_add_reg(ins->reg, ins->src1, ins->src2);
		return 1;
}

int enc_mov(Instruction *ins, uint32_t *out)
{
	if (ins->is_num) {
		return enc_load_num(ins->reg, ins->src1, out);
	}
	out[0] = enc_mov_reg(ins->reg, ins->src1);
	return 1;
}


int enc_sub(Instruction *ins, uint32_t *out) 
{

if (ins->is_num2 && ins->src2 > 0xFFF) {
			int n = enc_load_num(16, ins->src2, out);
			out[n] = enc_sub_reg(ins->reg, ins->src1, 16);
			return n + 1;
		}
		if (ins->is_num2) {
			out[0] = enc_sub_num(ins->reg, ins->src1, ins->src2);
			return 1;
		}
		if (ins->is_num) {
			int n = enc_load_num(16, ins->src1, out);
			out[n] = enc_sub_reg(ins->reg, 16, ins->src2);
			return n + 1;
		}
		out[0] = enc_sub_reg(ins->reg, ins->src1, ins->src2);
		return 1;
}


int enc_cmp(Instruction *ins, uint32_t *out) 
{
if (ins->is_num2 && ins->src2 > 0xFFF) {
			int n = enc_load_num(16, ins->src2, out);
			out[n] = enc_cmp_reg(ins->src1, 16);
			out[n + 1] = enc_cset(ins->reg);
			return n + 2;
		}

if (ins->is_num2) {
		out[0] = enc_cmp_num(ins->src1, ins->src2);
		out[1] = enc_cset(ins->reg);
		return 2;
}

if (ins->is_num) {
		int n = enc_load_num(16, ins->src1, out);
		out[n] = enc_cmp_reg(16, ins->src2);
		out[n + 1] = enc_cset(ins->reg);
		return n + 2;
}


out[0] = enc_cmp_reg(ins->src1, ins->src2);
out[1] = enc_cset(ins->reg);
return 2;
}

int enc_neg(Instruction *ins, uint32_t *out)
{
if (ins->is_num) {
     		    int n = enc_load_num(ins->reg, -(int64_t)ins->src1, out);
 		    return n;
		}
		out[0] = enc_neg_reg(ins->reg, ins->src1);
		return 1;
}

int enc_mult(Instruction *ins, uint32_t *out)
{
if (ins->is_num2) {
			int n = enc_load_num(16, ins->src2, out);
			out[n] = enc_mul(ins->reg, ins->src1, 16);
			return n + 1;
		}
		if (ins->is_num) {
			int n = enc_load_num(16, ins->src1, out);
			out[n] = enc_mul(ins->reg, 16, ins->src2);
			return n + 1;
		}
		out[0] = enc_mul(ins->reg, ins->src1, ins->src2);
		return 1;
}

int enc_div(Instruction *ins, uint32_t *out)
{
if (ins->is_num2) {
			int n = enc_load_num(16, ins->src2, out);
			out[n] = enc_sdiv(ins->reg, ins->src1, 16);
			out[n + 1] = enc_msub(16, ins->reg, 16, ins->src1);
			return n + 2;
		}
		if (ins->is_num) {
			int n = enc_load_num(16, ins->src1, out);
			out[n] = enc_sdiv(ins->reg, 16, ins->src2);
			out[n + 1] = enc_msub(16, ins->reg, ins->src2, 16);
			return n + 2;
		}
	out[0] = enc_sdiv(ins->reg, ins->src1, ins->src2);
	out[1] = enc_msub(16, ins->reg, ins->src2, ins->src1);
	return 2;
}

int enc_je(Instruction *ins, uint32_t *out)
{
	out[0] = enc_cbnz(ins->src1, ins->target);
		out[1] = enc_b(ins->target2);
		return 2;
}

int enc_jmp(Instruction *ins, uint32_t *out)
{
out[0] = enc_b(ins->target);
return 1;
}

int enc_label(Instruction *ins, uint32_t *out)
{
	(void)ins;
	(void)out;
	return 0;
}

int enc_call(Instruction *ins, uint32_t *out)
{
	out[0] = enc_bl(ins->target);
	out[1] = enc_mov_reg(ins->reg, 0);
	return 2;
}

int enc_ret(Instruction *ins, uint32_t *out)
{
	int n;
	if (ins->is_num)
		n = enc_load_num(0, ins->src1, out);
	else {
		out[0] = enc_ret_reg(0, ins->src1);   
		n = 1;
	}
	out[n] = enc_ret_x30();                        
	return n + 1;
}

static int (*encoders[])(Instruction *, uint32_t *) = {
[OP_ADD] = enc_add,
[OP_MULT] = enc_mult, 
[OP_SUB] = enc_sub, 
[OP_DIV] = enc_div, 
[OP_MOV] = enc_mov, 
[OP_JE] = enc_je, 
[OP_JMP] = enc_jmp, 
[OP_LABEL] = enc_label, 
[OP_CMP] = enc_cmp, 
[OP_NEG] = enc_neg, 
[OP_CALL] = enc_call, 
[OP_RET] = enc_ret 
};

int encode_one(Instruction *ins, uint32_t *out)
{
int (*f)(Instruction *, uint32_t *) = encoders[ins->op];
return f ? f(ins, out) : 0;
}
