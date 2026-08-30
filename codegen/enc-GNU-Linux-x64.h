#include "../IR/ir.h"
#ifndef ENC_H
#define ENC_H
#include <elf.h>
#include <stdint.h>

typedef enum { OP_MOV, OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_RET, OP_CMP, OP_JE, OP_JMP, OP_LABEL } oper;

typedef struct X64instruct {
	oper op;
	int src1;
	int src2;
	int reg;
        int is_num;
	int target;
	int target2;
} X64instruct;

typedef uint64_t X64functype;

Elf64_Ehdr make_elf_header(uint32_t shoff, int shnum);
Elf64_Shdr make_section(uint32_t name, uint32_t type, uint64_t flags, uint64_t offset, uint64_t size);
Elf64_Shdr make_text_section(uint64_t offset, uint64_t size);
Elf64_Shdr make_symtab_section(uint64_t offset, uint64_t size);
Elf64_Shdr make_strtab_section(uint32_t name, uint64_t offset, uint64_t size);
Elf64_Sym make_symbol(uint32_t strx, uint64_t value, uint64_t size);
int encode_one(X64instruct *ins, X64functype *out);
void first_pass(X64instruct *insns, int count, int *label_addr, X64functype *out, int *pos);
void second_pass(X64instruct *insns, int count, int *label_addr, X64functype *out, int *pos);
int split_bytes(X64instruct *insns, int total_instructions, int *function_starts, char **function_names,
		int function_count, X64functype *encoded_bytes, int *piece_borders, int *main_index);
X64functype *append_dead_functions(X64functype *out, int *piece_borders, int function_count, int main_index,
				int *nwords);
void write_object_file(X64instruct *insns, int count, const char *obj_path, int *function_starts, char **function_names,
		       int function_count);
X64functype enc_mov_reg(int regdest, int reg);
X64functype enc_mov_num(int regdest, int num);
X64functype enc_add_num(int regdest, int regn, int num);
X64functype enc_add_reg(int regdest, int regn, int regm);
X64functype enc_sub_num(int regdest, int regn, int num);
X64functype enc_sub_reg(int regdest, int regm, int regn);
X64functype enc_mul(int regdest, int regn, int regm);
X64functype enc_cmp_num(int regn, int num);
X64functype enc_cmp_reg(int regn, int regm);
X64functype enc_ret_reg(int rd, int rs);
X64functype enc_ret_num(int num);
X64functype enc_idiv(int regm);
X64functype enc_cqo(void);
X64functype enc_je(int offset);
X64functype enc_jmp(int offset);

#endif
