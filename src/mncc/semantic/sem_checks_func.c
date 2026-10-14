#include "semantic.h"

#include <stdio.h>
#include <stdlib.h>

VOFType Check_OP(VOFType left, VOFType right)
{
	if (left == TYPE_INT && right == TYPE_INT) {
		return TYPE_INT;
	} else {
		fprintf(stderr, "Semantic error:\n");
		exit(1);
	}
}

void Check_RET(VOFType func_type, VOFType ex_type)
{
	if (func_type != ex_type) {
		fprintf(stderr, "Semantic error (семантическая ошибка от "
				"функции Check_RET)\n");
		exit(1);
	}
}
