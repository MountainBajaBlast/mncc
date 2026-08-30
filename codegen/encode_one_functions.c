#include "enc.h"

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

uint32_t enc_msub(int regdest, int regn, int regm, int reg)
{
        return 0x9B008000 | ((uint32_t)regm << 16) | ((uint32_t)reg << 10) |
               ((uint32_t)regn << 5) | ((uint32_t)regdest << 0);
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


