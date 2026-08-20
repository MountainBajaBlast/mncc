#include "enc.h"
#ifndef CODEGEN_H
#define CODEGEN_H

void link_object(const char *obj_path, const char *output_path);

void compile_to_binary(IRGraph *graph, RISCinstruct **out_insns,
		       int *out_count);

#endif
