#include "lira.h"

#include <stdbool.h>

void init_quad_liveness(QuadLiveness *ql, size_t max_regs)
{
        if (ql == NULL) {
                return;
        }

        ql->use = create_bit_vector(max_regs);
        ql->def = create_bit_vector(max_regs);
}

void free_quadliveness(QuadLiveness *ql)
{
        if (ql == NULL) {
                return;
        }

        free_bit_vector(&ql->use);
        free_bit_vector(&ql->def);
}

void init_block_liveness(BlockLiveness *bl, size_t max_regs)
{
        if (bl == NULL) {
                return;
        }

        bl->b_use = create_bit_vector(max_regs);
        bl->b_def = create_bit_vector(max_regs);
        bl->live_in = create_bit_vector(max_regs);
        bl->live_out = create_bit_vector(max_regs);
}

void free_blockliveness(BlockLiveness *bl)
{
        if (bl == NULL) {
                return;
        }

        free_bit_vector(&bl->b_use);
        free_bit_vector(&bl->b_def);
        free_bit_vector(&bl->live_in);
        free_bit_vector(&bl->live_out);
}
static inline void process_operand(BitVector *bv, const Operand *op)
{
        if (op && op->type == VIR_REG) {
                set_bit(bv, op->val.tmp_index);
        }
}

void compute_local_liveness(IRGraph *graph, QuadLiveness *quad,
                            BlockLiveness *block)
{
        if (graph == NULL || quad == NULL || block == NULL)
                return;

        size_t q_idx = 0;
        size_t b_idx = 0;
        BasicBlock *current_block = graph->head;

        while (current_block != NULL) {
                init_block_liveness(&block[b_idx], graph->reg_count);

                Quadriple *current_quad = current_block->head;
                while (current_quad != NULL) {
                        init_quad_liveness(&quad[q_idx], graph->reg_count);

                        process_operand(&quad[q_idx].use, current_quad->arg1);
                        process_operand(&quad[q_idx].use, current_quad->arg2);
                        process_operand(&quad[q_idx].def, current_quad->result);

                        for (size_t byte = 0;
                             byte < block[b_idx].b_use.bytes_count; byte++) {
                                unsigned char inst_use_minus_block_def =
                                    quad[q_idx].use.data[byte] &
                                    ~block[b_idx].b_def.data[byte];
                                block[b_idx].b_use.data[byte] |=
                                    inst_use_minus_block_def;
                                block[b_idx].b_def.data[byte] |=
                                    quad[q_idx].def.data[byte];
                        }

                        q_idx++;
                        if (current_quad == current_block->tail)
                                break;
                        current_quad = current_quad->next;
                }

                b_idx++;
                current_block = current_block->next_block;
        }
}

void compute_global_liveness(IRGraph *graph, QuadLiveness *quad,
                             BlockLiveness *block)
{
        if (graph == NULL || quad == NULL || block == NULL)
                return;

        bool changed;
        do {
                changed = false;
                size_t b_idx = 0;
                size_t total_quads = 0;
                BasicBlock *current_block = graph->head;
                while (current_block != NULL) {
                        size_t block_quad_count = 0;
                        Quadriple *q = current_block->head;
                        while (q != NULL) {
                                block_quad_count++;
                                if (q == current_block->tail)
                                        break;
                                q = q->next;
                        }

                        for (int i = 0; i < current_block->succ_count; i++) {
                                BasicBlock *succ = current_block->successors[i];
                                if (succ != NULL) {
                                        bit_vector_union(
                                            &block[b_idx].live_out,
                                            &block[succ->id_block].live_in);
                                }
                        }

                        if (block_quad_count > 0) {
                                BitVector current_live =
                                    create_bit_vector(graph->reg_count);
                                bit_vector_union(&current_live,
                                                 &block[b_idx].live_out);

                                size_t block_end_idx =
                                    total_quads + block_quad_count - 1;
                                size_t block_start_idx = total_quads;

                                for (size_t i = block_end_idx;; i--) {
                                        bit_vector_diff(&current_live,
                                                        &quad[i].def);
                                        bit_vector_union(&current_live,
                                                         &quad[i].use);
                                          if (i == block_start_idx)
                                                break;
                                }

                                if (!bit_vector_equal(&block[b_idx].live_in,
                                                      &current_live)) {
                                        bit_vector_diff(&block[b_idx].live_in,
                                                        &block[b_idx].live_in);
                                        bit_vector_union(&block[b_idx].live_in,
                                                         &current_live);
                                        changed = true;
                                }
                                free_bit_vector(&current_live);
                        } else {
                                if (!bit_vector_equal(&block[b_idx].live_in,
                                                      &block[b_idx].live_out)) {
                                        bit_vector_diff(&block[b_idx].live_in,
                                                        &block[b_idx].live_in);
                                        bit_vector_union(
                                            &block[b_idx].live_in,
                                            &block[b_idx].live_out);
                                        changed = true;
                                }
                        }
                        total_quads += block_quad_count;
                        b_idx++;
                        current_block = current_block->next_block;
                }

        } while (changed);
}


