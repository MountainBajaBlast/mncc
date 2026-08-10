#include <stdlib.h>

#include "parser.h"

void free_ast(ASTNode *node)
{
	if (!node)
		return;

	switch (node->type) {
	case ND_BINARY_OP:
		free_ast(node->BinaryOp.left);
		free_ast(node->BinaryOp.right);
		break;

	case ND_VARIABLES:
		free(node->variables.name);
		break;

	case ND_FILE:
		for (int i = 0; i < node->file.statement_count; i++)
			free_ast(node->file.statements[i]);

		free(node->file.statements);
		break;

	case ND_RETURN:
		free_ast(node->ret.expression);
		break;

	case ND_VAR_DECL:
		free(node->var_decl.name);
		free_ast(node->var_decl.initializer);
		break;

	case ND_ASSIGMENT:
		free(node->assigment.name);
		free_ast(node->assigment.expression);
		break;

	default:
		break;
	}

	free(node);
}
