#ifndef SEMANTIC_H
#define SEMANTIC_H

struct ASTNode;

typedef enum { TYPE_INT, TYPE_VOID, TYPE_UKNOWN } VOFType;

typedef struct {
	VOFType type;
	char name[150];
	int symbol_id;
	int is_const;
} VarSym;

typedef struct HashNode {
	char *key;
	VarSym value;
	struct HashNode *next;
} HashNode;

typedef struct HashTable {
	HashNode **buckets;
	int size;
	int symbol_count;
	struct HashTable *parent;
} HashTable;


typedef struct {
        char name[256];
        int param_count;
        HashTable *scope;
} FuncInfo;


HashTable *create_table(int size);
void free_table(HashTable *table);

void insert(HashTable *table, const char *key, VarSym value);
VarSym search(HashTable *table, const char *key);
unsigned long hash_djb2(unsigned char *str);

VOFType Check_OP(VOFType left, VOFType right);
void Check_RET(VOFType func_type, VOFType ex_type);

VOFType eval_expr_type(struct ASTNode *node, HashTable *table);
void check_semantics(struct ASTNode *node, HashTable *table);

FuncInfo *find_function(const char* name);
void register_function(const char *name, int param_count, HashTable* scope);

HashTable *create_child_table(int size, HashTable *parent);
#endif
