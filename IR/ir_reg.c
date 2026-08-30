#include "ir.h"

#include <stdio.h>
#include <stdlib.h>

#include "../semantic/semantic.h"


int read_reg(VerMan *var, const char *name, HashTable *table)
{
	int found = 0;
	VarSym sum = search(table, name, &found);
	if (!found) {
		fprintf(stderr, "Unknown variable %s\n", name);
		exit(EXIT_FAILURE);
	}

	int variant = var->last_regs->data[sum.symbol_id];

	return variant;
}

void write_reg(VerMan *var, const char *name, HashTable *table, int tmp_index)
{
	int found = 0;

	VarSym sum = search(table, name, &found);

	if (!found) {
		fprintf(stderr, "Unknown variable %s\n", name);
		exit(EXIT_FAILURE);
	}

	var->last_regs->data[sum.symbol_id] = tmp_index;
}
