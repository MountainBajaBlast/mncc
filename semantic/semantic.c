#include "semantic.h"

#include <stdio.h>
#include <stdlib.h>

#include "../parser/parser.h"

#define HASH_TABLE_SIZE 1024


VOFType eval_expr_type(struct ASTNode *node, HashTable *table)
{
	if (node == NULL)
		return TYPE_VOID;

	VarSym symbol;

	switch (node->type) {
	case ND_NUMBER: {
		return TYPE_INT;
	}
	case ND_VARIABLES: {
		if (node->variables.name != NULL) {
			symbol = search(table, node->variables.name);
			return symbol.type;
		}

		return TYPE_UKNOWN;
	}
	case ND_BINARY_OP: {
		VOFType left_type = eval_expr_type(node->BinaryOp.left, table);
		VOFType right_type = eval_expr_type(node->BinaryOp.right, table);

		return Check_OP(left_type, right_type);
	}
	case ND_TYPE_INT: {
		return TYPE_INT;
	}

	case ND_UNARY: {
 	   return eval_expr_type(node->UnaryOp.operand, table);
}
	case ND_CALL: {
    return TYPE_INT;
}

	default:
		return TYPE_UKNOWN;
	}
}

void check_semantics(struct ASTNode *node, HashTable *table)
{
	if (node == NULL)
		return;

	switch (node->type) {
	case ND_FUNC: {
		if (node->func.name == NULL) {
			for (int i = 0; i < node->func.statement_count; i++) {
				if (node->func.statements[i]->type == ND_FUNC) {
					register_function(node->func.statements[i]->func.name,
							  node->func.statements[i]->func.parametr_count, NULL);
				}
			}
			for (int i = 0; i < node->func.statement_count; i++)
				check_semantics(node->func.statements[i], table);
			break;
		}

		HashTable *scope = create_child_table(table->size, table);

		for (int i = 0; i < node->func.parametr_count; i++) {
			VarSym param = {0};
			param.is_const = 0;
			param.type = TYPE_INT;
			insert(scope, node->func.parametrs[i]->variables.name, param);
		}

		register_function(node->func.name, node->func.parametr_count, scope);

		for (int i = 0; i < node->func.statement_count; i++)
			check_semantics(node->func.statements[i], scope);
		break;
	}

	case ND_VAR_DECL: {
		VarSym new_var = {0};
		new_var.is_const = node->var_decl.is_const;
		new_var.type = TYPE_INT;
		insert(table, node->var_decl.name, new_var);

		if (node->var_decl.initializer) {
			eval_expr_type(node->var_decl.initializer, table);
		}

		break;
	}

	case ND_RETURN: {
		VOFType expression_type = eval_expr_type(node->ret.expression, table);

		Check_RET(TYPE_INT, expression_type);
		break;
	}
	case ND_ASSIGMENT: {
		VarSym var = search(table, node->assigment.name);
		if (var.is_const) {
			printf("Are you serious?\n");
			exit(1);
		}

		(void)var;
		eval_expr_type(node->assigment.expression, table);
		break;
	}
	case ND_BRANCH: {
		eval_expr_type(node->branching.condition, table);
		if(node->branching.if_body) {
		HashTable *if_scope = create_child_table(table->size, table);
		check_semantics(node->branching.if_body, if_scope);
		free_table(if_scope);
		}
		if (node->branching.else_body) {
		HashTable *else_scope = create_child_table(table->size, table);
		check_semantics(node->branching.else_body, else_scope);
		free_table(else_scope);
		}
		break;
	}
	case ND_FILE:
		for (int i = 0; i < node->file.statement_count; i++)
			check_semantics(node->file.statements[i], table);
		break;
	case ND_CALL: {
		FuncInfo *expected = find_function(node->call.name);
		if (expected == NULL) { fprintf(stderr, "Функция '%s' не объявлена\n", node->call.name); exit(1); }
		if (node->call.argument_count != expected->param_count) {
			fprintf(stderr, "Функция '%s': ожидается %d аргументов, получено %d\n",
				node->call.name, expected->param_count, node->call.argument_count);
			exit(1);
		}

		for (int i = 0; i < node->call.argument_count; i++)
			eval_expr_type(node->call.arguments[i], table);
		break;
	}
	default:
		printf("[Debug] Семантика встретила необработанный тип узла: %d\n", node->type);
		break;
	}
}
