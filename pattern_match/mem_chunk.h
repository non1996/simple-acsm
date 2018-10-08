#pragma once

#include "afx.h"

class(mem_chunk) {
	mem_chunk *next;
	size_t cap;
	size_t used;
	char block[0];
};

static inline mem_chunk *mem_chunk_new(size_t cap) {
	mem_chunk *self = (mem_chunk*)mem_alloc_zero_fn(sizeof(mem_chunk) + cap);
	self->cap = cap;
	return self;
}

static inline void mem_chunk_delete(mem_chunk *self) {
	mem_free(self);
}

static inline bool mem_chunk_enough(mem_chunk *self, size_t expect) {
	return self->cap - self->used >= expect;
}

static inline char *mem_chunk_use(mem_chunk *self, size_t size) {
	assert(self->cap >= self->used + size);
	size_t used = self->used;
	self->used += size;
	return self->block + used;
}

