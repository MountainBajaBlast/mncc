#define Rd(r)    ((uint32_t)((r) & 0x1F) << 0)
#define Rn(r)    ((uint32_t)((r) & 0x1F) << 5)
#define Ra(r)    ((uint32_t)((r) & 0x1F) << 10)
#define Rm(r)    ((uint32_t)((r) & 0x1F) << 16)
#define Imm12(i) ((uint32_t)((i) & 0xFFF) << 10)
#define Imm16(i) ((uint32_t)((i) & 0xFFFF) << 5)
#define Hw(h)    ((uint32_t)((h) & 0x3) << 21)
#define Off19(o) ((uint32_t)((o) & 0x7FFFF) << 5)
#define Off26(o) ((uint32_t)((o) & 0x3FFFFFF) << 0)
#define A64_ADD_R  0x8B000000
#define A64_ADD_I  0x91000000
#define A64_SUB_R  0xCB000000
#define A64_SUB_I  0xD1000000
#define A64_MUL    0x9B007C00
#define A64_SDIV   0x9AC00C00
#define A64_MSUB   0x9B008000
#define A64_NEG    0xCB0003E0
#define A64_MOV_R  0xAA0003E0
#define A64_MOVZ   0xD2800000
#define A64_MOVK   0xF2800000
#define A64_CMP_R  0xEB00001F
#define A64_CMP_I  0xF100001F
#define A64_CSET   0x9A9F17E0
#define A64_B      0x14000000
#define A64_CBNZ   0xB5000000
#define A64_CALL   0x94000000
#define A64_RET    0xD65F03C0
#define RRR(base, d, n, m)     ((base) | Rm(m) | Rn(n) | Rd(d))
#define RRI(base, d, n, i)     ((base) | Imm12(i) | Rn(n) | Rd(d))
#define RR(base, d, m)         ((base) | Rm(m) | Rd(d))
#define RRRA(base, d, n, a, m) ((base) | Rm(m) | Ra(a) | Rn(n) | Rd(d))
