#include "../IR/ir.h"
#include "../optimizator/opt.h"
#ifndef LIRA_H
#define LIRA_H

typedef struct {
	unsigned char *data;
	size_t bytes_count;
	size_t max_regs;
} BitVector;

typedef struct {
	BitVector use;
	BitVector def;
} QuadLiveness;

typedef struct {
	BitVector b_use;
	BitVector b_def;
	BitVector live_in;
	BitVector live_out;
} BlockLiveness;

typedef struct {
	int start_time;
	int end_time;
	int phys_reg;
	int vreg_id;
} LivenessInterval;

typedef struct {
	LivenessInterval *intervals;
	size_t intervals_count;
} CollectIntervals;

typedef struct {
	int phys_reg[31];
} PhysRegTrack;

typedef enum { REG_PHYSICAL, REG_VIRTUAL } RegType;

BitVector create_bit_vector(size_t max_regs);
void free_bit_vector(BitVector *bv);
void init_quad_liveness(QuadLiveness *ql, size_t max_regs);
void free_quadliveness(QuadLiveness *ql);
void init_block_liveness(BlockLiveness *bl, size_t max_regs);
void free_blockliveness(BlockLiveness *bl);
void set_bit(BitVector *bv, size_t idx);
int check_bit(BitVector *bv, size_t idx);
void bit_vector_union(BitVector *left, const BitVector *right);
void bit_vector_diff(BitVector *left, const BitVector *right);
int bit_vector_equal(const BitVector *bv1, const BitVector *bv2);
void compute_local_liveness(IRGraph *graph, QuadLiveness *quad,
			    BlockLiveness *block);
void compute_global_liveness(IRGraph *graph, QuadLiveness *quad,
			     BlockLiveness *block);
CollectIntervals *create_intervals(size_t vreg_count);
void free_intervals(CollectIntervals *cointerval);
void build_intervals(IRGraph *graph, QuadLiveness *quad, BlockLiveness *block,
		     CollectIntervals *cointerval);
int compare_intervals_by_weight(const void *a, const void *b);
void sort_intervals(CollectIntervals *cointerval);
void greedy_allocate(CollectIntervals *cointerval, PhysRegTrack *reg_track);
void rewrite_registers(IRGraph *graph, CollectIntervals *cointerval);
#endif
