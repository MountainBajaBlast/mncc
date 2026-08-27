#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Lexer *lexer;

Token current_token;



struct ASTNode *parse_declaration();
struct ASTNode *parse_branching();
struct ASTNode *parse_statement();
struct ASTNode *parse_file();
struct ASTNode *parse_term();



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
	nodef->file.statements =
	    (struct ASTNode **)malloc(sizeof(struct ASTNode *) * 100);
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

	nodef->func.statements =
	    (struct ASTNode **)malloc(sizeof(struct ASTNode *) * 100);
	nodef->func.statement_count = 0;

	while (current_token.type != TK_EOF) {
		struct ASTNode *function = parse_function();

		nodef->func.statements[nodef->func.statement_count] = function;

		nodef->func.statement_count++;
	}

	match(TK_EOF);
	return nodef;
}



struct ASTNode *parse_function()
{
	struct ASTNode *func = CreateNode(ND_FUNC);
	func->func.statements =
	    (struct ASTNode **)malloc(sizeof(struct ASTNode *) * 100);
	func->func.statement_count = 0;

        
	match(TK_INTEGER);
        func->func.name = strdup(current_token.text);
	match(TK_FUNCNAME);
	match(TK_LARPEN);
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
