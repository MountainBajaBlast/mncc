#include "parser.h"


struct ASTNode *parse_declaration();
struct ASTNode *parse_branching();
struct ASTNode *parse_statement();
struct ASTNode *parse_file();
struct ASTNode *parse_term();
void advance();
void match(TokenNames expected);
struct ASTNode *parse_expression();
struct ASTNode *CreateNode(NodeTypes type);
struct ASTNode* parse_primary();
struct ASTNode* parse_block();

struct ASTNode *parse_branching()
{
        struct ASTNode *Node = CreateNode(ND_BRANCH);
        match(TK_IF);
        match(TK_LARPEN);
        Node->branching.condition = parse_expression();
        if (current_token.type == TK_CMP) {
                match(TK_CMP);
                struct ASTNode *right = parse_expression();
                struct ASTNode *bin = CreateNode(ND_BINARY_OP);
                bin->BinaryOp.op = (int)TK_CMP;
                bin->BinaryOp.left = Node->branching.condition;
                bin->BinaryOp.right = right;
                Node->branching.condition = bin;
        }

        match(TK_RARPEN);
        match(TK_START);
        Node->branching.if_body = parse_block();

        if (current_token.type == TK_ELSE) {
                match(TK_ELSE);
                if (current_token.type == TK_IF)
                        Node->branching.else_body = parse_branching();
                else {
                        match(TK_START);
                        Node->branching.else_body = parse_block();
                }
        }
        return Node;
}

