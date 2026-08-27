#include <stdio.h>
#include "parser.h"
#include <stdlib.h>
#include <string.h>

struct ASTNode *parse_declaration();
struct ASTNode *parse_branching();
struct ASTNode *parse_statement();
struct ASTNode *parse_file();
struct ASTNode *parse_term();
void advance();
void match(TokenNames expected);
struct ASTNode *parse_expression();
struct ASTNode *CreateNode(NodeTypes type);

struct ASTNode *parse_primary()
{
        struct ASTNode *NewNode = NULL;

        switch (current_token.type) {
        case TK_NUMBER: {
                NewNode = CreateNode(ND_NUMBER);
                NewNode->Numbers.value = atoi(current_token.text);
                match(TK_NUMBER);
                break;
        }
        case TK_VARIABLE: {
                NewNode = CreateNode(ND_VARIABLES);
                NewNode->variables.name = strdup(current_token.text);
                match(TK_VARIABLE);
                break;
        }
        case TK_LARPEN: {
                match(TK_LARPEN);
                NewNode = parse_expression();
                match(TK_RARPEN);
                break;
        }

        default:
                printf("Syntax error, token type = %d\n", current_token.type);
                exit(1);
        }
        return NewNode;
}



struct ASTNode *parse_statement()
{
        if (current_token.type == TK_RETURN) {
                struct ASTNode *NewNode = CreateNode(ND_RETURN);
                match(TK_RETURN);
                NewNode->ret.expression = parse_expression();
                match(TK_SEMICOLON);
                return NewNode;
        }

        struct ASTNode *expr = parse_expression();

        if (expr->type == ND_VARIABLES && current_token.type == TK_ASSIGN) {
                struct ASTNode *assign = CreateNode(ND_ASSIGMENT);
                assign->assigment.name = expr->variables.name;
                free(expr);
                match(TK_ASSIGN);
                assign->assigment.expression = parse_expression();
                match(TK_SEMICOLON);
                return assign;
        }

        return expr;
}


struct ASTNode *parse_declaration()
{
        struct ASTNode *node_var = CreateNode(ND_VAR_DECL);

        if (current_token.type == TK_CONST) {
                node_var->var_decl.is_const = true;
                match(TK_CONST);
        }

        match(TK_INTEGER);
        node_var->var_decl.name = strdup(current_token.text);

        match(TK_VARIABLE);

        node_var->var_decl.initializer = NULL;

        if (current_token.type == TK_ASSIGN) {
                match(TK_ASSIGN);
                node_var->var_decl.initializer = parse_expression();
        }

        match(TK_SEMICOLON);

        return node_var;
}

















