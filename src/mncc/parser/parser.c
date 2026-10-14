#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Lexer *lexer;

Token current_token;


void advance() { current_token = next_token(lexer); }

void match(TokenNames expected)
{
	if (current_token.type != expected) {
		printf("Match error\n ");
		exit(1);
	}
	advance();
}

struct ASTNode *CreateNode(NodeTypes type)
{
	struct ASTNode *NewNode = calloc(1, sizeof(struct ASTNode));
	if (!NewNode) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}
	NewNode->type = type;
	return NewNode;
}




struct ASTNode *parse_block()
{
	struct ASTNode *nodef = CreateNode(ND_FILE);
	nodef->file.statements = (struct ASTNode **)malloc(sizeof(struct ASTNode *) * 100);
	nodef->file.statement_count = 0;

	while (current_token.type != TK_END) {
		struct ASTNode *code_str = NULL;

		switch (current_token.type) {
		case (TK_INTEGER):
			code_str = parse_declaration();
			break;
		case (TK_CONST):
			code_str = parse_declaration();
			break;
		case (TK_IF):
			code_str = parse_branching();
			break;
		default:
			code_str = parse_statement();
			break;
		}

		nodef->file.statements[nodef->file.statement_count] = code_str;
		nodef->file.statement_count++;
	}
	match(TK_END);
	return nodef;
}

struct ASTNode *parse_file()
{
	struct ASTNode *nodef = CreateNode(ND_FUNC);

	nodef->func.statements = (struct ASTNode **)malloc(sizeof(struct ASTNode *) * 100);
	nodef->func.statement_count = 0;

	while (current_token.type != TK_EOF) {
		struct ASTNode *function = parse_function();

		nodef->func.statements[nodef->func.statement_count] = function;

		nodef->func.statement_count++;
	}

	match(TK_EOF);
	return nodef;
}

struct ASTNode* parse_funcall()
{


         struct ASTNode *funcall_node = CreateNode(ND_CALL);
         funcall_node->call.name = strdup(current_token.text);
	match(TK_IDENTIFIER);



          match(TK_LARPEN);


	int capacity = 4;

          funcall_node->call.arguments = (struct ASTNode **)malloc(sizeof(struct ASTNode *) * capacity);

	funcall_node->call.argument_count = 0;

        while (current_token.type != TK_RARPEN) {

		 if (funcall_node->call.argument_count >= capacity) {
                        capacity *= 2;

                        funcall_node->call.arguments =
                                realloc(
                                        funcall_node->call.arguments,
                                        sizeof(struct ASTNode *) * capacity
                                );
                  }

                 funcall_node->call.arguments[funcall_node->call.argument_count] = parse_expression();
                funcall_node->call.argument_count++;
                if (current_token.type != TK_COMMA) {
                      break;
                 }

        match(TK_COMMA);
    }

        match(TK_RARPEN);
	
        return funcall_node;
}




struct ASTNode *parse_function()
{
	struct ASTNode *func = CreateNode(ND_FUNC);
	 func->func.return_type = CreateNode(ND_TYPE_INT);
	func->func.statements = (struct ASTNode **)malloc(sizeof(struct ASTNode *) * 100);
	func->func.statement_count = 0;


	match(TK_INTEGER);
	func->func.name = strdup(current_token.text);
	match(TK_IDENTIFIER);
	match(TK_LARPEN);
	int capacity = 10;

	func->func.parametrs = (struct ASTNode **)malloc(sizeof(struct ASTNode *) * capacity);
	 while (current_token.type != TK_RARPEN) {

		 match(TK_INTEGER);


	         ASTNode* param = CreateNode(ND_VARIABLES);	
		 param->variables.name = strdup(current_token.text);	
		 match(TK_IDENTIFIER);
                 if (func->func.parametr_count >= capacity) {
                        capacity *= 2;

                        func->func.parametrs =
                                realloc(
                                        func->func.parametrs,
                                        sizeof(ASTNode *) * capacity
                                );
                  }
		    func->func.parametrs[
                func->func.parametr_count
        ] = param;

        func->func.parametr_count++;

        if (current_token.type != TK_COMMA) {
                break;
        }

        match(TK_COMMA);
}













	match(TK_RARPEN);
	match(TK_START);



	while (current_token.type != TK_END) {
		struct ASTNode *code_str = NULL;

		switch (current_token.type) {
		case (TK_INTEGER):
			code_str = parse_declaration();
			break;
		case (TK_CONST):
			code_str = parse_declaration();
			break;
		case (TK_IF):
			code_str = parse_branching();
			break;
		default:
			code_str = parse_statement();
			break;
		}

		int index = func->func.statement_count;
		index = func->func.statement_count;
		func->func.statements[index] = code_str;
		func->func.statement_count++;
	}

	match(TK_END);


	return func;
}
