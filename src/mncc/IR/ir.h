#include "../semantic/semantic.h"
#ifndef IR_H
#define IR_H
#include "../parser/parser.h"

typedef enum { IR_TYPE_INT64, IR_TYPE_VOID } Types;

typedef enum { VIR_REG, NUM, VAR, NONE, REG_PHYS } OpKind;

typedef enum { RET, DIV, MULT, ADD, SUB, ASSIGN, JMP, JE, PHIN, CMP, TERMINATOR_NONE, UNARY, CALL } OpCode;

typedef struct {
	int result_reg;
	int orig_var;
	int reg_index;
	int from_label;
} PhiEntry;

typedef struct Operand {
	OpKind type;

	union {
		long long val_int;
		char *var_name;
		int tmp_index;
		int label_id;

	} val;

} Operand;

typedef struct Quadriple {
	struct Operand *arg1;
	struct Operand *arg2;
	OpCode operation;
	struct Operand *result;
	struct Quadriple *next;
	struct Quadriple *prev;
	struct FunCall *call;
} Quadriple;

typedef struct FunCall {
	char FuncName[256];
	struct Operand **args;
	int argument_count;
} FunCall;

typedef struct BasicBlock {
	char *label;
	Quadriple *head;
	Quadriple *tail;
	struct BasicBlock *next_block;
	struct BasicBlock *idom;
	PhiEntry *phis;
	int phi_count;
	int phi_cap;
	int id_block;
	OpCode terminator;
	struct BasicBlock *true_target_je;
	struct BasicBlock *false_target_je;
	struct BasicBlock *jmp_target;
	struct BasicBlock **predecessors;
	int pred_count;
	int pred_cap;
	struct BasicBlock **successors;
	int succ_count;
	int succ_cap;
	struct BasicBlock **df;
	int df_count;
	int df_cap;
} BasicBlock;

typedef struct IRGraph {
	int reg_count;
	struct BasicBlock *head;
	char *func_name;
	BasicBlock *current_block;
	int block_count;
	BasicBlock **blocks;
	int param_count;
	int *param_phys;
} IRGraph;

typedef struct DominatorTree {
	BasicBlock *entry;
	int *dfn;
	int *semi;
	int *parent;
	BasicBlock **bucket;
	int *ancestor;
	int *label;
	BasicBlock **block_by_dfn;
	int *idom;
	int block_count;
} DominatorTree;

typedef struct RegArr {
	int *data;
	size_t capacity;
	size_t size;
} RegArr;

typedef struct VerMan {
	int symbol_count;
	RegArr *last_regs;
} VerMan;

typedef struct IRResult {
	struct Operand *operand;
	BasicBlock *block;
} IRResult;


typedef struct IRProgram {
	IRGraph **graphs;
	int count;
} IRProgram;

#define FOR_EACH_BLOCK(graph, blk) \
	for (BasicBlock *blk = (graph)->head; (blk) != NULL; (blk) = (blk)->next_block)

#define FOR_EACH_QUAD(blk, quad) \
	for (Quadriple *quad = (blk)->head; (quad) != NULL; (quad) = (quad)->next)

#define FOR_EACH_PRED(blk, i) for (int i = 0; i < (blk)->pred_count; i++)

#define FOR_EACH_SUCC(blk, i) for (int i = 0; i < (blk)->succ_count; i++)

#define FOR_EACH_PHI(blk, i) for (int i = 0; i < (blk)->phi_count; i++)

#define FOR_EACH_DF(blk, i) for (int i = 0; i < (blk)->df_count; i++)

Operand *create_operand_num(long long value);
Operand *create_operand_var(char *name);
Operand *create_operand_reg(IRGraph *graph);
Quadriple *create_quadriple(Operand *argum1, Operand *argum2, Operand *res, FunCall *call, OpCode oper);
FunCall *create_func_call(char *name, Operand **args);
void append_quadriple(BasicBlock *block, Quadriple *quad);
BasicBlock *create_basic_block(char *labe);
IRGraph *create_ir_graph(char *function_name);
void free_basic_block(BasicBlock *block);
void free_ir_graph(IRGraph *graph);
BasicBlock *append_new_block(IRGraph *graph, char *label);
void set_current_block(IRGraph *graph, BasicBlock *block);
void free_verman(VerMan *vregs);
void init_arr(RegArr *reg, size_t init_cap);
VerMan *init_verman(int symbols, RegArr *regs);
int read_reg(VerMan *var, const char *name, HashTable *table);
void write_reg(VerMan *var, const char *name, HashTable *table, int tmp_index);
Operand *create_operand_reg_ref(int reg);
IRResult gen_ir_expr(ASTNode *tree, VerMan *manager, HashTable *table, IRGraph *graph);
IRResult gen_ir_stmt(ASTNode *tree, VerMan *manager, HashTable *table, IRGraph *graph);
IRResult gen_ir_call(ASTNode *tree, VerMan *manager, HashTable *table, IRGraph *graph);
IRResult gen_ir_branching(ASTNode *tree, VerMan *manager, HashTable *table, IRGraph *graph);
IRProgram *compile_to_ir(HashTable *table, ASTNode *node);
int dsu_eval(DominatorTree *domtree, int v);
void bb_add_phi(BasicBlock *block, int result_reg, int reg_index, int from_block);
void free_phis(BasicBlock *block);
void bb_add_successor(BasicBlock *block, BasicBlock *succ);
void bb_add_predecessor(BasicBlock *block, BasicBlock *pred);
void build_cfg(IRGraph *graph);
int dsu_find(DominatorTree *domtree, int v);
DominatorTree *create_dominator_tree(IRGraph *graph);
void free_domtree(DominatorTree *domtree);
void df_add(BasicBlock *block, BasicBlock *x);
void dsu_link(DominatorTree *domtree, int v, int w);
void build_dominators(DominatorTree *domtree);
int dominates(DominatorTree *domtree, BasicBlock *a, BasicBlock *b);
BasicBlock *get_idom(DominatorTree *domtree, BasicBlock *block);
void compute_dominance_frontier(IRGraph *graph, DominatorTree *dt);
void place_phi(IRGraph *graph, DominatorTree *dt);
void into_ssa(IRGraph *graph);
void out_of_ssa(IRGraph *graph);
void dump_graph(IRGraph *graph);

#endif
