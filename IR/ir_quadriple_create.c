#include "ir.h"

#include <stdlib.h>

#include "../semantic/semantic.h"

Quadriple *create_quadriple(Operand *argum1, Operand *argum2, Operand *res,
                            OpCode oper)
{
        Quadriple *quad = calloc(1, sizeof(Quadriple));
        quad->arg1 = argum1;
        quad->arg2 = argum2;
        quad->result = res;
        quad->operation = oper;
        return quad;
}

void append_quadriple(BasicBlock *block, Quadriple *quad)
{
        if (block->head == NULL) {
                block->head = quad;
                block->tail = quad;
        } else {
                block->tail->next = quad;
                quad->prev = block->tail;
                block->tail = quad;
        }
}


