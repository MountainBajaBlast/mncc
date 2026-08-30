#include "ir.h"
#include "../semantic/semantic.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

BasicBlock *append_new_block(IRGraph *graph, char *label);
void set_current_block(IRGraph *graph, BasicBlock *block);
void free_verman(VerMan *vregs);



IRResult gen_ir_branching(ASTNode *tree, VerMan *manager, HashTable *table, IRGraph *graph)
{
	BasicBlock *entry_bb = graph->current_block;

	BasicBlock *then_bb = append_new_block(graph, "then_bb");
	BasicBlock *else_bb = append_new_block(graph, "else_bb");
	BasicBlock *merge_bb = append_new_block(graph, "merge_bb");

	set_current_block(graph, entry_bb);
	IRResult cond = gen_ir_expr(tree->branching.condition, manager, table, graph);

	Quadriple *je_quad = create_quadriple(cond.operand, NULL, NULL, JE);
	append_quadriple(entry_bb, je_quad);

	entry_bb->terminator = JE;
	entry_bb->true_target_je = then_bb;
	entry_bb->false_target_je = else_bb;

	set_current_block(graph, then_bb);
	if (tree->branching.if_body != NULL) {
		gen_ir_stmt(tree->branching.if_body, manager, table, graph);
	}
	then_bb->terminator = JMP;
	then_bb->jmp_target = merge_bb;

	set_current_block(graph, else_bb);
	if (tree->branching.else_body != NULL) {
		gen_ir_stmt(tree->branching.else_body, manager, table, graph);
	}
	else_bb->terminator = JMP;
	else_bb->jmp_target = merge_bb;

	set_current_block(graph, merge_bb);

	IRResult res;
	res.operand = NULL;
	res.block = merge_bb;
	return res;
}

IRResult gen_ir_expr(ASTNode *tree, VerMan *manager, HashTable *table, IRGraph *graph)
{
	if (tree == NULL) {
		printf("Error: it s null\n");
		exit(1);
	}

	IRResult res;
	res.operand = NULL;

	switch (tree->type) {
	case ND_NUMBER: {
		res.operand = create_operand_num(tree->Numbers.value);
		break;
	}
	case ND_VARIABLES: {
		char *var_name = tree->variables.name;
		int reg = read_reg(manager, var_name, table);
		res.operand = create_operand_reg_ref(reg);
		break;
	}
	case ND_BINARY_OP: {
		ASTNode *left = tree->BinaryOp.left;
		ASTNode *right = tree->BinaryOp.right;

		IRResult left_res = gen_ir_expr(left, manager, table, graph);
		IRResult right_res = gen_ir_expr(right, manager, table, graph);

		Operand *res_op = create_operand_reg(graph);
		OpCode op;

		switch (tree->BinaryOp.op) {
		case TK_ADD:
			op = ADD;
			break;
		case TK_SUBTRACTION:
			op = SUB;
			break;
		case TK_MULTYPLY:
			op = MULT;
			break;
		case TK_DIVISION:
			op = DIV;
			break;
		case TK_CMP:
			op = CMP;
			break;
		default:
			fprintf(stderr, "Error: Unknown binary operator\n");
			exit(1);
		}

		Quadriple *new_quad = create_quadriple(left_res.operand, right_res.operand, res_op, op);
		append_quadriple(graph->current_block, new_quad);
		res.operand = res_op;
		break;
	}
	default:
		break;
	}

	return res;
}

IRResult gen_ir_stmt(ASTNode *tree, VerMan *manager, HashTable *table, IRGraph *graph)
{
	if (tree == NULL) {
		printf("Error: it s null\n");
		exit(1);
	}

	IRResult res;
	res.operand = NULL;

	switch (tree->type) {
	case ND_VAR_DECL: {
		Operand *target_reg = create_operand_reg(graph);

		if (tree->var_decl.initializer) {
			IRResult var_decl_init = gen_ir_stmt(tree->var_decl.initializer, manager, table, graph);
			Quadriple *new_quad_assign = create_quadriple(var_decl_init.operand, NULL, target_reg, ASSIGN);
			append_quadriple(graph->current_block, new_quad_assign);
		}

		write_reg(manager, tree->var_decl.name, table, target_reg->val.tmp_index);
		res.operand = target_reg;
		break;
	}
	case ND_FILE: {
		int count = tree->file.statement_count;

		for (int i = 0; i < count; i++) {
			ASTNode *stmt = tree->file.statements[i];
			gen_ir_stmt(stmt, manager, table, graph);
		}
		break;
	}
	case ND_RETURN: {
		ASTNode *ret_expr = tree->ret.expression;
		IRResult ret_val = gen_ir_expr(ret_expr, manager, table, graph);
		Quadriple *ret_quad = create_quadriple(ret_val.operand, NULL, NULL, RET);
		append_quadriple(graph->current_block, ret_quad);
		graph->current_block->terminator = RET;
		res.operand = ret_val.operand;
		break;
	}
	case ND_ASSIGMENT: {
		Operand *target_reg = create_operand_reg_ref(read_reg(manager, tree->assigment.name, table));

		if (tree->assigment.expression) {
			IRResult var_assigment = gen_ir_stmt(tree->assigment.expression, manager, table, graph);
			Quadriple *new_quad_assign = create_quadriple(var_assigment.operand, NULL, target_reg, ASSIGN);
			append_quadriple(graph->current_block, new_quad_assign);
		}

		write_reg(manager, tree->assigment.name, table, target_reg->val.tmp_index);
		res.operand = target_reg;
		break;
	}
	case ND_BRANCH:
		res = gen_ir_branching(tree, manager, table, graph);
		break;

	case ND_NUMBER:
	case ND_VARIABLES:
	case ND_BINARY_OP:

		res = gen_ir_expr(tree, manager, table, graph);
		break;
	default:
		break;
	}
	return res;
}

IRProgram *compile_to_ir(HashTable *table, ASTNode *node)
{
	IRProgram *prog = malloc(sizeof(IRProgram));
	prog->count = node->func.statement_count;
	prog->graphs = calloc(prog->count, sizeof(IRGraph *));

	for (int i = 0; i < node->func.statement_count; i++) {
		ASTNode *fn = node->func.statements[i];


		IRGraph *graph = create_ir_graph(fn->func.name);
		BasicBlock *block = create_basic_block("entry");
		block->id_block = 0;
		graph->head = block;
		graph->blocks = realloc(graph->blocks, sizeof(BasicBlock *));
		graph->blocks[graph->block_count] = block;
		graph->block_count++;
		graph->current_block = block;
		int sym_count = table->symbol_count;
		RegArr *array = calloc(1, sizeof(RegArr));
		init_arr(array, table->symbol_count);
		VerMan *manager = init_verman(sym_count, array);

		for (int j = 0; j < fn->func.statement_count; j++) {
			gen_ir_stmt(fn->func.statements[j], manager, table, graph);
		}

		free_verman(manager);
		prog->graphs[i] = graph;
	}
	return prog;
}
