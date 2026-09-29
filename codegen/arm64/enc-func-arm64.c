#include "../enc.h"
#include "enc-func-arm64.h"
 

uint32_t enc_mov_reg(int regdest, int reg)
{
return RR(A64_MOV_R, regdest, reg);
}

uint32_t enc_movz(int reg, uint16_t chunk, int chunk_num)
{
	return A64_MOVZ | Rd(reg) | Imm16(chunk) | Hw(chunk_num);
}

uint32_t enc_movk(int reg, uint16_t chunk, int chunk_num)
{
	return A64_MOVK | Rd(reg) | Imm16(chunk) |
	       Hw(chunk_num);
}


uint32_t enc_add_num(int regdest, int regn, int num)
{
	return RRI(A64_ADD_I, regdest, regn, num);
}

uint32_t enc_add_reg(int regdest, int regn, int regm)
{
	return RRR(A64_ADD_R, regdest, regn, regm);
}



uint32_t enc_sub_num(int regdest, int regn, int num)
{
	return RRI(A64_SUB_I, regdest, regn, num);
}

uint32_t enc_sub_reg(int regdest, int regn, int regm)
{
	return RRR(A64_SUB_R, regdest, regn, regm);
}


uint32_t enc_mul(int regdest, int regn, int regm)
{
	return RRR(A64_MUL, regdest, regn, regm);
}

uint32_t enc_sdiv(int regdest, int regn, int regm)
{
	return RRR(A64_SDIV, regdest, regn, regm);
}

uint32_t enc_msub(int regdest, int regn, int regm, int reg)
{
	return RRRA(A64_MSUB, regdest, regn, reg, regm);
}

uint32_t enc_neg_reg(int regdest, int regm)
{
	return RR(A64_NEG, regdest, regm);
}

uint32_t enc_cmp_num(int regn, int num)
{
	return A64_CMP_I | Imm12(num) | Rn(regn);
}

uint32_t enc_cmp_reg(int regn, int regm)
{
	return A64_CMP_R | Rm(regm) | Rn(regn);
}


uint32_t enc_cset(int reg)
{
	return A64_CSET | Rd(reg);
}

uint32_t enc_b(int offset)
{
	return A64_B | Off26(offset);
}

uint32_t enc_ret_reg(int rd, int rs)
{
	return enc_mov_reg(rd, rs);
}

uint32_t enc_cbnz(int reg, int offset)
{
	return A64_CBNZ | Rd(reg) | Off19(offset);
}

uint32_t enc_bl(int offset)
{
	return A64_CALL | Off26(offset);
}

uint32_t enc_ret_x30(void)
{
	return A64_RET | Rn(30);
}
