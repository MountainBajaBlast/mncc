#include "semantic.h"

#include <stdio.h>
#include <stdlib.h>

#include "../parser/parser.h"

#define HASH_TABLE_SIZE 1024


VOFType eval_expr_type(struct ASTNode *node, HashTable *table)
{
	if (node == NULL)
		return TYPE_VOID;

	int found = 0;
	VarSym symbol;

	switch (node->type) {
	case ND_NUMBER: {
		return TYPE_INT;
	}
	case ND_VARIABLES: {
		if (node->variables.name != NULL) {
			symbol = search(table, node->variables.name, &found);
			return symbol.type;
		}

		return TYPE_UKNOWN;
	}
	case ND_BINARY_OP: {
		VOFType left_type = eval_expr_type(node->BinaryOp.left, table);
		VOFType right_type =
		    eval_expr_type(node->BinaryOp.right, table);

		return Check_OP(left_type, right_type);
	}
	case ND_TYPE_INT: {
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
          for (int i = 0; i < node->func.statement_count; i++) {
               check_semantics(node->func.statements[i], table);  
          }
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
		VOFType expression_type =
		    eval_expr_type(node->ret.expression, table);

		Check_RET(TYPE_INT, expression_type);
		break;
	}
	case ND_ASSIGMENT: {
		int found = 0;
		VarSym var = search(table, node->assigment.name, &found);
		if (var.is_const) {
			printf("Пошёл ты нахуй пидор бля\n");
			exit(1);
		}

		(void)var;
		eval_expr_type(node->assigment.expression, table);
		break;
	}
	case ND_BRANCH: {
		eval_expr_type(node->branching.condition, table);
		check_semantics(node->branching.if_body, table);
		if (node->branching.else_body)
			check_semantics(node->branching.else_body, table);
		break;
	}
        case ND_FILE:
	    for (int i = 0; i < node->file.statement_count; i++)
	     check_semantics(node->file.statements[i], table);
		break;
	default:
		printf(
		    "[Debug] Семантика встретила необработанный тип узла: %d\n",
		    node->type);
		break;
	}
}
