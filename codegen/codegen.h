#include "../IR/ir.h"
#ifndef CODEGEN_H
#define CODEGEN_H

typedef enum {
     ASM_MOV,
     ASM_ADD, 
     ASM_SUB, 
     ASM_MUL, 
     ASM_DIV,
     ASM_RET,
     ASM_CMP,
     ASM_JMP,
     ASM_JE,
     ASM_LABEL
} asm_oper;

typedef struct RISCinstruct {
   asm_oper op;
   int src1;
   int src2;
   int reg;
   int is_imm;
   int target;
   int target2;
} RISCinstruct;


void in_asm(FILE *out, RISCinstruct *inst, int count);

void compile_to_binary(IRGraph *graph, const char *output_name);


#endif
