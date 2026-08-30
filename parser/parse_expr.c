#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

struct ASTNode *parse_declaration();
struct ASTNode *parse_branching();
struct ASTNode *parse_statement();
struct ASTNode *parse_file();
struct ASTNode *parse_term();
void advance();
void match(TokenNames expected);
struct ASTNode *parse_expression();
struct ASTNode *CreateNode(NodeTypes type);
struct ASTNode *parse_primary();

struct ASTNode *parse_expression()
{
	struct ASTNode *left_node = parse_term();
	while (current_token.type == TK_ADD || current_token.type == TK_SUBTRACTION) {
		TokenNames op_type = current_token.type;
		match(op_type);
		struct ASTNode *binary_node = CreateNode(ND_BINARY_OP);
		binary_node->BinaryOp.op = (int)op_type;
		binary_node->BinaryOp.left = left_node;
		binary_node->BinaryOp.right = parse_term();
		left_node = binary_node;
	}
	return left_node;
}


struct ASTNode *parse_term()
{
	struct ASTNode *left_node = parse_primary();
	while (current_token.type == TK_MULTYPLY || current_token.type == TK_DIVISION) {
		TokenNames op_type = current_token.type;
		match(op_type);
		struct ASTNode *binary_node = CreateNode(ND_BINARY_OP);
		binary_node->BinaryOp.op = (int)op_type;
		binary_node->BinaryOp.left = left_node;
		binary_node->BinaryOp.right = parse_primary();
		left_node = binary_node;
	}
	return left_node;
}
