#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../parser/parser.h"
#include "semantic.h"

#define HASH_TABLE_SIZE  1024



HashTable* create_table(int size) {
           HashTable *table = malloc(sizeof(HashTable));
           table->size = size;
           table->buckets = calloc(size, sizeof(HashNode*));
           table->symbol_count = 0;
           return table;
}

VarSym search(HashTable *table, const char *key, int *found) {


     unsigned long index =  hash_djb2((unsigned char*)key) % table->size;
     

     HashNode *current = table->buckets[index];

     while(current != NULL) {
         if(strcmp(current->key, key) == 0) {

              *found = 1;
              return current->value;
         } 

          
         current = current->next;
     }


      fprintf(stderr, "Семантическая ошибка переменная  '%s'  ещё не объявлена!\n", key);
      exit(1);

     
}

unsigned long hash_djb2(unsigned char *str) {
         unsigned long hash = 5381;
         int c;

         while((c = *str++)) {
              hash = ((hash << 5) + hash) + c;
         }
         return hash;
}

void insert(HashTable *table, const char *key, VarSym value) {
     
      
     unsigned long index = hash_djb2((unsigned char*)key) % table->size;

    

     HashNode *current = table->buckets[index];


     while(current != NULL) {
         if(strcmp(current->key, key) == 0) {
               fprintf(stderr, "Семантическая ошибка: Переменная '%s' уже объявлена!\n", key);
               exit(1);       
         } 
         current = current->next;
           
         
     }

     HashNode *new_node = calloc(1, sizeof(HashNode));
     new_node->key = strdup(key);
     
     value.symbol_id = table->symbol_count;

     new_node->value = value;

     new_node->next = table->buckets[index];
     table->buckets[index] = new_node;

     table->symbol_count++; 
}

void free_table(HashTable *table) {
     for (int i = 0; i < table->size; i++) {
         HashNode *current = table->buckets[i];
         while(current != NULL) {
              HashNode *temp = current;
              current = current->next;
              free(temp->key);
              free(temp);
          }
     }
     free(table->buckets);
     free(table);
}

VOFType Check_OP(VOFType left, VOFType right) {
       if (left == TYPE_INT && right == TYPE_INT) {
           return TYPE_INT;
       } else {
          fprintf(stderr, "Semantic error:\n");
          exit(1);
      }
      
}

void  Check_RET(VOFType func_type, VOFType ex_type) {
        if (func_type != ex_type) {
          fprintf(stderr, "Semantic error (семантическая ошибка от функции Check_RET)\n");
          exit(1);
 
       } 
       
}


VOFType eval_expr_type(struct ASTNode* node, HashTable* table) {
    if (node == NULL) return TYPE_VOID;

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
            VOFType right_type = eval_expr_type(node->BinaryOp.right, table);
           
            return Check_OP(left_type, right_type);
        }
        case ND_TYPE_INT: {
            return TYPE_INT;
        }

        default:
            return TYPE_UKNOWN;
    }
}




void check_semantics(struct ASTNode* node, HashTable* table) {
     if (node == NULL) return;

    

         switch (node->type) {
              case ND_FILE:
                   for(int i = 0; i < node->file.statement_count; i++) {
                      check_semantics(node->file.statements[i], table);
                   }
                   break;
               case ND_VAR_DECL: {
                    VarSym new_var = {0};
                    new_var.is_const = node->var_decl.is_const;    
                    new_var.type = TYPE_INT;
                    insert(table, node->var_decl.name, new_var);
                
                    if(node->var_decl.initializer) {
                           eval_expr_type(node->var_decl.initializer, table);                   
                    }                     

                    break;
               }
               
           
               case ND_RETURN: {
                    VOFType expression_type = eval_expr_type(node->ret.expression, table);
                    
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
             
                   default:
                        printf("[Debug] Семантика встретила необработанный тип узла: %d\n", node->type);
                        break;
            }            
}


























































