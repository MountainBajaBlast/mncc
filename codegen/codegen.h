#include "enc.h"
#ifndef CODEGEN_H
#define CODEGEN_H

void link_object(const char *obj_path, const char *output_path);

void compile_to_binary(IRGraph *graph, RISCinstruct **out_insns,
		       int *out_count);

void compile_program_to_binary(IRProgram *program,
                               RISCinstruct **out_insns, int *out_count,
                               int **out_starts, char ***out_names,
                               int *out_function_count);

#endif
