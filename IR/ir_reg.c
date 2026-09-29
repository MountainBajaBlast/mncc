#include "ir.h"

#include "../semantic/semantic.h"


static RegArr *ensure_regs(VerMan *var, int symbol_id)
{
int new_cap = symbol_id + 1;

	if(new_cap <= (int)var->last_regs->capacity) {
		return var->last_regs;
	}


	var->last_regs->data = realloc(var->last_regs->data, new_cap * sizeof(int));
	for(int i = var->last_regs->capacity; i < new_cap ; i++) {
		 var->last_regs->data[i] = -1;
	}

	var->last_regs->capacity = new_cap;
	return var->last_regs;




}





int read_reg(VerMan *var, const char *name, HashTable *table)
{
	VarSym sum = search(table, name);

	return ensure_regs(var, sum.symbol_id)->data[sum.symbol_id];
}

void write_reg(VerMan *var, const char *name, HashTable *table, int tmp_index)
{
	VarSym sum = search(table, name);

	ensure_regs(var, sum.symbol_id)->data[sum.symbol_id] = tmp_index;
}
