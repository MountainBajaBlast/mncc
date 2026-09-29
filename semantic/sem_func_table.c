#include "semantic.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <string.h>

static FuncInfo funcs[256];
static int func_count = 0;


void register_function(const char *name, int param_count, HashTable* scope) {
	FuncInfo* existing = find_function(name);

	if(existing && (existing->scope != NULL || scope == NULL)) {
		fprintf(stderr, "Функция '%s' уже объявлена\n", name);
		exit(1);
	}

	if (existing) {
		existing->scope = scope;
		return;
	}



	if (func_count >= 256) {
		fprintf(stderr, "Слишком много функций (максимум 256)\n");
		exit(1);
	}

	

	strncpy(funcs[func_count].name, name, 255);
	funcs[func_count].name[255] = '\0';
	funcs[func_count].param_count = param_count;
	funcs[func_count].scope = scope;
	func_count++;
}


FuncInfo* find_function(const char* name) {
	for (int i = 0; i < func_count; i++)
		if (strcmp(funcs[i].name, name) == 0)
			return  &funcs[i];
	return NULL;
}
