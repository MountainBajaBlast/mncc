#include "ir.h"

#include <stdlib.h>
#include <string.h>

#include "../semantic/semantic.h"

Operand *create_operand_num(long long value)
{
        Operand *op = calloc(1, sizeof(Operand));
        op->type = NUM;
        op->val.val_int = value;
        return op;
}

Operand *create_operand_var(char *name)
{
        Operand *vop = calloc(1, sizeof(Operand));
        vop->type = VAR;
        vop->val.var_name = strdup(name);
        return vop;
}

Operand *create_operand_reg(IRGraph *graph)
{
        Operand *rop = calloc(1, sizeof(Operand));
        rop->type = VIR_REG;
        rop->val.tmp_index = graph->reg_count++;
        return rop;
}

Operand *create_operand_reg_ref(int reg)
{
        Operand *op = calloc(1, sizeof(Operand));
        op->type = VIR_REG;
        op->val.tmp_index = reg;
        return op;
}
