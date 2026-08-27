#include "ir.h"

#include <stdlib.h>

#include "../semantic/semantic.h"


void init_arr(RegArr *reg, size_t init_cap)
{
        reg->data = calloc(init_cap, sizeof(int));

        for (size_t i = 0; i < init_cap; i++)
                reg->data[i] = -1;

        reg->size = 0;
        reg->capacity = init_cap;
}

VerMan *init_verman(int symbols, RegArr *regs)
{
        VerMan *vregs = calloc(1, sizeof(VerMan));
        vregs->last_regs = regs;
        vregs->symbol_count = symbols;

        return vregs;
}

void free_verman(VerMan *vregs)
{
        if (vregs == NULL) {
                return;
        }

        if (vregs->last_regs != NULL) {
                if (vregs->last_regs->data != NULL) {
                        free(vregs->last_regs->data);
                }

                free(vregs->last_regs);
        }

        free(vregs);
}


















