#ifndef PARSER_H
#define PARSER_H
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "../lexer/lexer.h"
#include <stdbool.h>


void advance();
struct ASTNode* parse_function();
void free_ast(struct ASTNode *node);

typedef enum {
     ND_RETURN,
     ND_NUMBER,
     ND_FILE,
     ND_VARIABLES,
     ND_TYPE_INT,
     ND_BINARY_OP,
     ND_VAR_DECL,
     ND_ASSIGMENT,
     ND_BRANCH
} NodeTypes;

typedef struct ASTNode ASTNode;
          
      
struct ASTNode {
     NodeTypes type;

   union {

      struct {
        ASTNode *condition;
        ASTNode *if_body;
        ASTNode *else_body;;
      } branching;




      struct {
         int op;
         ASTNode *left;
         ASTNode *right;
     } BinaryOp;

       struct {
          char *name;
     } variables;
            

       struct {
          char *name;
          ASTNode *initializer;
          bool is_const;
       } var_decl;

       struct { 
         char *name;
         ASTNode *expression;
       } assigment;


       struct {
          int value;
      } Numbers;

        struct {
            ASTNode **statements;
            int statement_count;
        } file;

         struct {
                ASTNode *expression;
        } ret;

         struct {
              long value;
        } IntegerType;
  };

};
#endif
