#include "../IR/ir.h"


#ifndef CODEGEN_GNU_LINUX_X64
#define CODEGEN_GNU_LINUX_X64

void compile_to_binary(IRGraph *graph, X64instruct **out_insns, int *out_count);

void compile_program_to_binary(IRProgram *program,
                               X64instruct **out_insns, int *out_count,
                               int **out_starts, char ***out_names,
                               int *out_function_count);

void link_object(const char *obj_path, const char *output_path);

#endif
