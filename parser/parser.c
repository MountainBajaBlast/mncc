#include "../lexer/lexer.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "parser.h"
#include <string.h>
#include <stdbool.h>

Lexer *lexer;

Token current_token;

struct ASTNode* parse_declaration();
struct ASTNode* parse_branching();
struct ASTNode* parse_statement();
struct ASTNode* parse_file();


void advance() {
   current_token = next_token(lexer);
}

void match(TokenNames expected) {
           if(current_token.type != expected) {
                printf("Match error\n ");
                exit(1);
            }
            advance();
   }


struct ASTNode* CreateNode(NodeTypes type) {
    struct ASTNode* NewNode = calloc(1, sizeof(struct ASTNode));
    if (!NewNode) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    NewNode->type = type;
    return NewNode;
}

struct ASTNode* parse_primary() {

struct ASTNode* NewNode = NULL;
        
       switch(current_token.type) { 
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
                default:
                 printf("Syntax error, token type = %d\n", current_token.type);
                 exit(1);
       }
       return NewNode;
}          

struct ASTNode* parse_term() {
    struct ASTNode* left_node = parse_primary();
    while (current_token.type == TK_MULTYPLY || current_token.type == TK_DIVISION) {
        TokenNames op_type = current_token.type;
        match(op_type);
        struct ASTNode* binary_node = CreateNode(ND_BINARY_OP);
        binary_node->BinaryOp.op = (int)op_type;
        binary_node->BinaryOp.left = left_node;
        binary_node->BinaryOp.right = parse_primary();
        left_node = binary_node;
    }
    return left_node;
}

struct ASTNode* parse_expression() {
    struct ASTNode* left_node = parse_term();
    while (current_token.type == TK_ADD || current_token.type == TK_SUBTRACTION) {
        TokenNames op_type = current_token.type;
        match(op_type);
        struct ASTNode* binary_node = CreateNode(ND_BINARY_OP);
        binary_node->BinaryOp.op = (int)op_type;
        binary_node->BinaryOp.left = left_node;
        binary_node->BinaryOp.right = parse_term();
        left_node = binary_node;
    }
    return left_node;
}


struct ASTNode* parse_block() {
    struct ASTNode* nodef = CreateNode(ND_FILE);
    nodef->file.statements = (struct ASTNode**)malloc(sizeof(struct ASTNode*) * 100);
    nodef->file.statement_count = 0;

    while (current_token.type != TK_END) {
        struct ASTNode* code_str = NULL;

        if (current_token.type == TK_INTEGER || current_token.type == TK_CONST) {
            code_str = parse_declaration();
        } else if (current_token.type == TK_IF) {
            code_str = parse_branching();
        } else {
            code_str = parse_statement();
        }

        nodef->file.statements[nodef->file.statement_count] = code_str;
        nodef->file.statement_count++;
    }
    match(TK_END);
    return nodef;
}


struct ASTNode* parse_branching() {
       struct ASTNode* Node = CreateNode(ND_BRANCH);
       match(TK_IF);
       match(TK_LARPEN);
       Node->branching.condition = parse_expression();
       if (current_token.type == TK_CMP) {
           match(TK_CMP);
           struct ASTNode* right = parse_expression();
           struct ASTNode* bin = CreateNode(ND_BINARY_OP);
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





struct ASTNode* parse_statement() {
    if(current_token.type == TK_RETURN) {
        struct ASTNode* NewNode = CreateNode(ND_RETURN);
        match(TK_RETURN);
        NewNode->ret.expression = parse_expression();
        match(TK_SEMICOLON);
        return NewNode;
    }

    struct ASTNode* expr = parse_expression();

    if (expr->type == ND_VARIABLES && current_token.type == TK_ASSIGN) {
        struct ASTNode* assign = CreateNode(ND_ASSIGMENT);
        assign->assigment.name = expr->variables.name;
        free(expr);
        match(TK_ASSIGN);
        assign->assigment.expression = parse_expression();
        match(TK_SEMICOLON);
        return assign;
    }

    return expr;
}














struct ASTNode* parse_file() {
         struct ASTNode* nodef = CreateNode(ND_FILE);

         nodef->file.statements = (struct ASTNode**)malloc(sizeof(struct ASTNode*) * 100);
         nodef->file.statement_count = 0;

         while (current_token.type != TK_EOF) {
                        struct ASTNode* stmt = parse_statement();

                        nodef->file.statements[nodef->file.statement_count] = stmt;

                        nodef->file.statement_count++;
            }
            match(TK_EOF);
            return nodef;
   }






struct ASTNode* parse_declaration() {
       

       struct ASTNode* node_var =  CreateNode(ND_VAR_DECL);

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
struct ASTNode* parse_function() {
         struct ASTNode* func_node = CreateNode(ND_FILE);
         func_node->file.statements = (struct ASTNode**)malloc(sizeof(struct ASTNode*) * 100);
         func_node->file.statement_count = 0;


         match(TK_INTEGER);
         match(TK_FUNCNAME);
         match(TK_LARPEN); 
         match(TK_RARPEN);
         match(TK_START);

         while (current_token.type != TK_END) {
               struct ASTNode* code_str = NULL;


               if (current_token.type == TK_INTEGER || current_token.type == TK_CONST) {
                   code_str = parse_declaration();
               } else if (current_token.type == TK_IF) {
                        code_str = parse_branching();
               } else {
                   code_str = parse_statement();
                }
               

               int index = func_node->file.statement_count;
               index = func_node->file.statement_count;
               func_node->file.statements[index] = code_str;
               func_node->file.statement_count++;
         }
         match(TK_END);

         return func_node;
 }






