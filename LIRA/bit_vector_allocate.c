#include "lira.h"

#include <stdlib.h>
#include <string.h>

BitVector create_bit_vector(size_t max_regs)
{
        BitVector bv;

        bv.bytes_count = (max_regs + 7) / 8;
        bv.max_regs = max_regs;

        bv.data =
            (unsigned char *)calloc(bv.bytes_count, sizeof(unsigned char));

        if (bv.data == NULL) {
                bv.bytes_count = 0;
                bv.max_regs = 0;
        }

        return bv;
}

void free_bit_vector(BitVector *bv)
{
        if (bv == NULL) {
                return;
        }

        if (bv->data != NULL) {
                free(bv->data);
                bv->data = NULL;
        }

        bv->bytes_count = 0;
        bv->max_regs = 0;
}

void set_bit(BitVector *bv, size_t idx)
{
        if (bv == NULL || idx >= bv->max_regs) {
                return;
        }

        size_t byte_idx = idx / 8;
        size_t bit_idx = idx % 8;

        bv->data[byte_idx] |= (1 << bit_idx);
}

int check_bit(BitVector *bv, size_t idx)
{
        if (bv == NULL || idx >= bv->max_regs) {
                return 0;
        }

        size_t byte_idx = idx / 8;
        size_t bit_idx = idx % 8;

        return (bv->data[byte_idx] & (1 << bit_idx)) != 0;
}

void bit_vector_union(BitVector *left, const BitVector *right)
{
        if (left == NULL || right == NULL ||
            left->bytes_count != right->bytes_count) {
                return;
        }

        for (int i = 0; i < left->bytes_count; i++) {
                left->data[i] |= right->data[i];
        }
}

void bit_vector_diff(BitVector *left, const BitVector *right)
{
        if (left == NULL || right == NULL ||
            left->bytes_count != right->bytes_count) {
                return;
        }

        for (int i = 0; i < left->bytes_count; i++) {
                left->data[i] &= ~right->data[i];
        }
}

int bit_vector_equal(const BitVector *bv1, const BitVector *bv2)
{
        if (bv1 == NULL || bv2 == NULL ||
            bv1->bytes_count != bv2->bytes_count) {
                return 0;
        }

        return memcmp(bv1->data, bv2->data, bv1->bytes_count) == 0;
}

