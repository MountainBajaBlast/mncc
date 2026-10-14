#include "lira.h"

#include <stdlib.h>
#include <string.h>

#define APPLY_BITS(operator)                                          \
	if (left && right && left->bytes_count == right->bytes_count) \
		for (size_t byte = 0; byte < left->bytes_count; byte++) \
			left->data[byte] operator right->data[byte]

BitVector create_bit_vector(size_t max_regs)
{
	BitVector bit_vector = { calloc((max_regs + 7) / 8, 1), (max_regs + 7) / 8, max_regs };
	if (!bit_vector.data)
		bit_vector.bytes_count = bit_vector.max_regs = 0;
	return bit_vector;
}

void free_bit_vector(BitVector *bit_vector)
{
	if (bit_vector) {
		free(bit_vector->data);
		bit_vector->data = NULL;
		bit_vector->bytes_count = bit_vector->max_regs = 0;
	}
}

void set_bit(BitVector *bit_vector, size_t index)
{
	if (bit_vector && index < bit_vector->max_regs)
		bit_vector->data[index / 8] |= 1 << (index % 8);
}

int check_bit(BitVector *bit_vector, size_t index)
{
	return bit_vector && index < bit_vector->max_regs &&
	       (bit_vector->data[index / 8] & (1 << (index % 8)));
}

void bit_vector_union(BitVector *left, const BitVector *right) { APPLY_BITS(|=); }
void bit_vector_diff(BitVector *left, const BitVector *right) { APPLY_BITS(&= ~); }

void bit_vector_copy(BitVector *destination, const BitVector *source)
{
	if (destination && source && destination->bytes_count == source->bytes_count)
		memcpy(destination->data, source->data, destination->bytes_count);
}

int bit_vector_equal(const BitVector *left, const BitVector *right)
{
	return left && right && left->bytes_count == right->bytes_count &&
	       !memcmp(left->data, right->data, left->bytes_count);
}
