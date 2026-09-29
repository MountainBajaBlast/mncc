#ifndef ENC_H
#define ENC_H
#include "../IR/ir.h"
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <stdint.h>

typedef enum { OP_ADD, OP_MULT, OP_SUB, OP_DIV, OP_MOV, OP_JE, OP_JMP, OP_LABEL, OP_CMP, OP_NEG, OP_CALL, OP_RET } oper;

typedef struct Instruction {
	oper op;
	int src1;	
	int src2;
	int reg;
	int is_num;
	int is_num2;
	int target;
	int target2;
} Instruction;

int enc_load_num(int reg, uint64_t num, uint32_t *out);
int encode_one(Instruction *ins, uint32_t *out);

uint32_t enc_mov_reg(int regdest, int reg);
uint32_t enc_movz(int reg, uint16_t chunk, int chunk_num);
uint32_t enc_movk(int reg, uint16_t chunk, int chunk_num);
uint32_t enc_add_num(int regdest, int regn, int num);
uint32_t enc_add_reg(int regdest, int regn, int regm);
uint32_t enc_sub_num(int regdest, int regn, int num);
uint32_t enc_sub_reg(int regdest, int regn, int regm);
uint32_t enc_mul(int regdest, int regn, int regm);
uint32_t enc_sdiv(int regdest, int regn, int regm);
uint32_t enc_msub(int regdest, int regn, int regm, int reg);
uint32_t enc_neg_reg(int regdest, int regm);
uint32_t enc_cmp_num(int regn, int num);
uint32_t enc_cmp_reg(int regn, int regm);
uint32_t enc_cset(int reg);
uint32_t enc_b(int offset);
uint32_t enc_ret_reg(int rd, int rs);
uint32_t enc_cbnz(int reg, int offset);
uint32_t enc_bl(int offset);
uint32_t enc_ret_x30(void);

void compile_program_to_binary(struct IRProgram *program, Instruction **out_insns, int *out_count,
			       int **out_starts, char ***out_names, int *out_function_count);
void write_object_file(Instruction *insns, int count, const char *obj_path, int *function_starts,
		       char **function_names, int function_count);
void link_object(const char *obj_path, const char *output_path);

void first_pass(Instruction *insns, int count, int *label_addr, uint32_t *out, int *pos);
void second_pass(Instruction *insns, int count, int *label_addr, uint32_t *out, int *pos);
int split_bytes(Instruction *insns, int total_instructions, int *function_starts, char **function_names,
		int function_count, uint32_t *encoded_bytes, int *piece_borders, int *main_index);
uint32_t *append_dead_functions(uint32_t *text, uint32_t *out, int *piece_borders, int function_count, int main_index,
				int *nwords);

struct mach_header_64 make_mach_header(int ncmds, int sizeofcmds);
struct segment_command_64 make_segment(int nsects, uint64_t vmsize);
struct section_64 make_section(uint32_t offset, uint64_t size);
struct symtab_command make_symtab(uint32_t symoff, int nsyms, uint32_t stroff, uint32_t strsize);
struct nlist_64 make_nlist(uint32_t strx, uint64_t value);
uint32_t *wrap_code(uint32_t *code, int code_size, int *out_nwords);

#endif
