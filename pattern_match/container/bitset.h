#pragma once

#include "afx.h"

#define BITARRAY_SHIFT 5
#define WORD_BIT 32
#define BITARRAY_MASK ((1u << BITARRAY_SHIFT) - 1)
#define FIND_SET 0
#define FIND_CLEAR 1
#define store_size(nbit) ((nbit + BITARRAY_MASK) >> BITARRAY_SHIFT)

class(bitset) {
	uint32_t * arr;
	size_t nbit;
};

static inline constructor(bitset, size_t count) {
	self->arr = mem_alloc_zero(uint32_t, store_size(count));
	self->nbit = count;
	constructor_end;
}

static inline distructor(bitset) {
	mem_free(self->arr);
}

static inline size_t bitset_size(bitset *self) {
	return self->nbit;
}

static inline void bitset_check(bitset *self, size_t expect) {
	if (self->nbit >= expect)
		return;

	while (self->nbit < expect)
		self->nbit <<= 1;

	mem_realloc(uint32_t, self->arr, store_size(self->nbit));
}

static inline void bitset_set(bitset *self, size_t bit) {
	//bitset_check(self, bit);
	if (bit >= self->nbit)
		return;

	self->arr[bit >> BITARRAY_SHIFT] |= (1u << (bit & BITARRAY_MASK));
}

static inline void bitset_clear(bitset *self, size_t bit) {
	if (bit >= self->nbit)
		return;

	self->arr[bit >> BITARRAY_SHIFT] &= ~(1u << (bit & BITARRAY_MASK));
}

static inline bool bitset_is_set(bitset *self, size_t bit) {
	if (bit >= self->nbit)
		return false;

	return self->arr[bit >> BITARRAY_SHIFT] & (1u << (bit & BITARRAY_MASK));
}

static inline void bitset_clear_all(bitset *self) {
	memset(self->arr, 0, 4 * store_size(self->nbit));
}
