#include "enc-GNU-Linux-x64.h"

X64functype enc_mov_reg(int regdest, int reg)
{
	uint64_t rex = 0x48 | (reg >= 8 ? 0x04 : 0) | (regdest >= 8 ? 0x01 : 0);
	uint64_t modrm = 0xC0 | ((reg & 7) << 3) | (regdest & 7);

	return (rex << 16) | ((uint64_t)0x89 << 8) | modrm;
}


X64functype enc_mov_num(int regdest, int num)
{
	uint64_t rex = 0x40 | (regdest >= 8 ? 0x01 : 0);   
	uint64_t opcode = 0xB8 | (regdest & 7);
	uint32_t imm = (uint32_t)num;
	return (rex << 40) | ((uint64_t)opcode << 32) | imm;  
}




X64functype enc_add_num(int regdest, int regn, int num)
{
	uint64_t rex = 0x48 | (regdest >= 8 ? 0x01 : 0);
	uint64_t modrm = 0xC0 | (0 << 3) | (regdest & 7);
	uint64_t imm = (uint32_t)(int32_t)num;

	return (rex << 56) | ((uint64_t)0x81 << 48) | (modrm << 40) | imm;
}

X64functype enc_add_reg(int regdest, int regn, int regm)
{
	uint64_t rex = 0x48 | (regm >= 8 ? 0x04 : 0) | (regdest >= 8 ? 0x01 : 0);
	uint64_t modrm = 0xC0 | ((regm & 7) << 3) | (regdest & 7);

	return (rex << 16) | ((uint64_t)0x01 << 8) | modrm;
}

X64functype enc_sub_num(int regdest, int regn, int num)
{
	uint64_t rex = 0x48 | (regdest >= 8 ? 0x01 : 0);
	uint64_t modrm = 0xC0 | (5 << 3) | (regdest & 7);
	uint64_t imm = (uint32_t)(int32_t)num;

	return (rex << 56) | ((uint64_t)0x81 << 48) | (modrm << 40) | imm;
}

X64functype enc_sub_reg(int regdest, int regm, int regn)
{
	uint64_t rex = 0x48 | (regm >= 8 ? 0x04 : 0) | (regdest >= 8 ? 0x01 : 0);
	uint64_t modrm = 0xC0 | ((regm & 7) << 3) | (regdest & 7);

	return (rex << 16) | ((uint64_t)0x29 << 8) | modrm;
}

X64functype enc_mul(int regdest, int regn, int regm)
{
	uint64_t rex = 0x48 | (regdest >= 8 ? 0x04 : 0) | (regm >= 8 ? 0x01 : 0);
	uint64_t modrm = 0xC0 | ((regdest & 7) << 3) | (regm & 7);

	return (rex << 24) | ((uint64_t)0x0F << 16) | ((uint64_t)0xAF << 8) | modrm;
}

X64functype enc_cmp_num(int regn, int num)
{
	uint64_t rex = 0x48 | (regn >= 8 ? 0x01 : 0);
	uint64_t modrm = 0xC0 | (7 << 3) | (regn & 7);
	uint64_t imm = (uint32_t)(int32_t)num;

	return (rex << 56) | ((uint64_t)0x81 << 48) | (modrm << 40) | imm;
}

X64functype enc_cmp_reg(int regn, int regm)
{
	uint64_t rex = 0x48 | (regm >= 8 ? 0x04 : 0) | (regn >= 8 ? 0x01 : 0);
	uint64_t modrm = 0xC0 | ((regm & 7) << 3) | (regn & 7);

	return (rex << 16) | ((uint64_t)0x39 << 8) | modrm;
}

X64functype enc_ret_reg(int rd, int rs) { return enc_mov_reg(rd, rs); }

X64functype enc_ret_num(int num) { return enc_mov_num(0, num); }



X64functype enc_idiv(int regm)
{

	uint64_t rex = 0x48 | (regm >= 8 ? 0x01 : 0);
	uint64_t modrm = 0xC0 | (7 << 3) | (regm & 7);

	return (rex << 16) | ((uint64_t)0xF7 << 8) | modrm;
}


X64functype enc_cqo(void) { return (0x48 << 8) | 0x99; }

X64functype enc_jmp(int offset) { return ((uint64_t)0xE9 << 32) | (uint32_t)offset; }

X64functype enc_je(int offset) { return ((uint64_t)0x0F << 40) | ((uint64_t)0x84 << 32) | (uint32_t)offset; }
