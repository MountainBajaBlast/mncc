#include "../IR/ir.h"
#ifndef ENC_H
#define ENC_H
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <stdint.h>



typedef enum {
	OP_MOV,
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_RET,
	OP_CMP,
	OP_JMP,
	OP_JE,
	OP_LABEL,
	OP_CSET,
	OP_CBNZ,
	OP_MOVZ,
	OP_MOVK
} oper;

typedef struct RISCinstruct {
	oper op;
	int src1;
	int src2;
	int reg;
	int is_num;
	int target;
	int target2;
} RISCinstruct;


int enc_load_num(int reg, uint64_t num, uint32_t *out);
int encode_one(RISCinstruct *ins, uint32_t *out);

uint32_t enc_mov_reg(int regdest, int reg);
uint32_t enc_movz(int reg, uint16_t chunk, int chunk_num);
uint32_t enc_movk(int reg, uint16_t chunk, int chunk_num);
uint32_t enc_add_num(int regdest, int regn, int num);
uint32_t enc_add_reg(int regdest, int regn, int regm);
uint32_t enc_sub_num(int regdest, int regn, int num);
uint32_t enc_sub_reg(int regdest, int regm, int regn);
uint32_t enc_mul(int regdest, int regn, int regm);
uint32_t enc_sdiv(int regdest, int regn, int regm);
uint32_t enc_cmp_num(int regn, int num);
uint32_t enc_cmp_reg(int regn, int regm);
uint32_t enc_cset(int reg);
uint32_t enc_b(int offset);
uint32_t enc_ret_reg(int rd, int rs);
uint32_t enc_cbnz(int reg, int offset);
struct mach_header_64 make_mach_header(int ncmds, int sizeofcmds);
struct segment_command_64 make_segment(int nsects, uint64_t vmsize);
struct section_64 make_section(uint32_t offset, uint64_t size);
struct symtab_command make_symtab(uint32_t symoff, int nsyms, uint32_t stroff,
				  uint32_t strsize);
int split_bytes(RISCinstruct *insns, int total_instructions,
                int *function_starts, char **function_names,
                int function_count, uint32_t *encoded_bytes,
                int *piece_borders, int *main_index);
struct nlist_64 make_nlist(uint32_t strx, uint64_t value);
void first_pass(RISCinstruct *insns, int count, int *label_addr, uint32_t *out,
		int *pos);
void second_pass(RISCinstruct *insns, int count, int *label_addr, uint32_t *out,
		 int *pos);
uint32_t *wrap_code(uint32_t *code, int code_size, int *out_nwords);
uint32_t *append_dead_functions(uint32_t *text, uint32_t *out,
				int *piece_borders, int function_count,
				int main_index, int *nwords);
void write_object_file(RISCinstruct *insns, int count, const char *obj_path,  int *function_starts, char **function_names, int function_count);


#endif
