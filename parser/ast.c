#include <stdlib.h>
#include "parser.h"

void free_ast(ASTNode *node)
{
        if (node == NULL)
                return;

        switch (node->type) {
        case ND_BINARY_OP:
                free_ast(node->BinaryOp.left);
                free_ast(node->BinaryOp.right);
                break;

        case ND_UNARY:
                free_ast(node->UnaryOp.operand);
                break;

        case ND_BRANCH:
                free_ast(node->branching.condition);
                free_ast(node->branching.if_body);
                free_ast(node->branching.else_body);
                break;

        case ND_VARIABLES:
                free(node->variables.name);
                break;

        case ND_NUMBER:
                break;

        case ND_TYPE_INT:
                break;

        case ND_FILE:
                for (int i = 0;
                     i < node->file.statement_count;
                     i++) {
                        free_ast(node->file.statements[i]);
                }

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

        case ND_CALL:
                free_ast(node->call.name);

                for (int i = 0;
                     i < node->call.argument_count;
                     i++) {
                        free_ast(node->call.arguments[i]);
                }

                free(node->call.arguments);
                break;

        case ND_FUNC:
                free(node->func.name);

                free_ast(node->func.return_type);

                for (int i = 0;
                     i < node->func.parametr_count;
                     i++) {
                        free_ast(node->func.parametrs[i]);
                }

                free(node->func.parametrs);

                for (int i = 0;
                     i < node->func.statement_count;
                     i++) {
                        free_ast(node->func.statements[i]);
                }

                free(node->func.statements);
                break;
        }

        free(node);
}

