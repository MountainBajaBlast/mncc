#include "enc-GNU-Linux-x64.h"

static X64functype nop_pad(X64functype v, int len)
{
	for (int i = len; i < 8; i++)
		v |= ((X64functype)0x90 << (8 * i));
	return v;
}

X64functype enc_mov_reg(int regdest, int reg)
{
	uint64_t rex = 0x48 | ((reg >= 8) ? 0x04 : 0) | ((regdest >= 8) ? 0x01 : 0);
	uint64_t modrm = 0xC0 | ((reg & 7) << 3) | (regdest & 7);

	return nop_pad(rex | ((uint64_t)0x89 << 8) | (modrm << 16), 3);
}

X64functype enc_mov_num(int regdest, int num)
{
	uint64_t rex = 0x48 | ((regdest >= 8) ? 0x01 : 0);
	uint64_t modrm = 0xC0 | (regdest & 7);
	uint32_t imm = (uint32_t)num;

	return nop_pad(rex | ((uint64_t)0xC7 << 8) | (modrm << 16) | ((uint64_t)imm << 24), 7);
}

X64functype enc_add_num(int regdest, int num)
{
	uint64_t rex = 0x48 | ((regdest >= 8) ? 0x01 : 0);
	uint64_t modrm = 0xC0 | (regdest & 7);
	uint32_t imm = (uint32_t)num;

	return nop_pad(rex | ((uint64_t)0x81 << 8) | (modrm << 16) | ((uint64_t)imm << 24), 7);
}

X64functype enc_add_reg(int regdest, int regm)
{
	uint64_t rex = 0x48 | ((regm >= 8) ? 0x04 : 0) | ((regdest >= 8) ? 0x01 : 0);
	uint64_t modrm = 0xC0 | ((regm & 7) << 3) | (regdest & 7);

	return nop_pad(rex | ((uint64_t)0x01 << 8) | (modrm << 16), 3);
}

X64functype enc_sub_num(int regdest, int num)
{
	uint64_t rex = 0x48 | ((regdest >= 8) ? 0x01 : 0);
	uint64_t modrm = 0xC0 | (5 << 3) | (regdest & 7);
	uint32_t imm = (uint32_t)num;

	return nop_pad(rex | ((uint64_t)0x81 << 8) | (modrm << 16) | ((uint64_t)imm << 24), 7);
}

X64functype enc_sub_reg(int regdest, int regm)
{
	uint64_t rex = 0x48 | ((regm >= 8) ? 0x04 : 0) | ((regdest >= 8) ? 0x01 : 0);
	uint64_t modrm = 0xC0 | ((regm & 7) << 3) | (regdest & 7);

	return nop_pad(rex | ((uint64_t)0x29 << 8) | (modrm << 16), 3);
}

X64functype enc_mul(int regdest, int regm)
{
	uint64_t rex = 0x48 | ((regdest >= 8) ? 0x04 : 0) | ((regm >= 8) ? 0x01 : 0);
	uint64_t modrm = 0xC0 | ((regdest & 7) << 3) | (regm & 7);

	return nop_pad(rex | ((uint64_t)0x0F << 8) | ((uint64_t)0xAF << 16) | (modrm << 24), 4);
}

X64functype enc_imul_num(int regdest, int num)
{
	uint64_t rex = 0x48 | ((regdest >= 8) ? 0x04 : 0) | ((regdest >= 8) ? 0x01 : 0);
	uint64_t modrm = 0xC0 | ((regdest & 7) << 3) | (regdest & 7);
	uint32_t imm = (uint32_t)num;

	return nop_pad(rex | ((uint64_t)0x69 << 8) | (modrm << 16) | ((uint64_t)imm << 24), 7);
}

X64functype enc_cmp_num(int regn, int num)
{
	uint64_t rex = 0x48 | ((regn >= 8) ? 0x01 : 0);
	uint64_t modrm = 0xC0 | (7 << 3) | (regn & 7);
	uint32_t imm = (uint32_t)num;

	return nop_pad(rex | ((uint64_t)0x81 << 8) | (modrm << 16) | ((uint64_t)imm << 24), 7);
}

X64functype enc_cmp_reg(int regn, int regm)
{
	uint64_t rex = 0x48 | ((regm >= 8) ? 0x04 : 0) | ((regn >= 8) ? 0x01 : 0);
	uint64_t modrm = 0xC0 | ((regm & 7) << 3) | (regn & 7);

	return nop_pad(rex | ((uint64_t)0x39 << 8) | (modrm << 16), 3);
}

X64functype enc_idiv(int regm)
{
	uint64_t rex = 0x48 | ((regm >= 8) ? 0x01 : 0);
	uint64_t modrm = 0xC0 | (7 << 3) | (regm & 7);

	return nop_pad(rex | ((uint64_t)0xF7 << 8) | (modrm << 16), 3);
}

X64functype enc_cqo(void) { return nop_pad((0x48 << 8) | 0x99, 2); }

X64functype enc_ret(void) { return nop_pad(0xC3, 1); }

X64functype enc_jmp(int offset) { return nop_pad(0xE9 | ((uint64_t)(uint32_t)offset << 8), 5); }

X64functype enc_je(int offset)
{
	return nop_pad(0x0F | ((uint64_t)0x84 << 8) | ((uint64_t)(uint32_t)offset << 16), 6);
}