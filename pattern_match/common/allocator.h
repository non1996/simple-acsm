#pragma once

#include "afx.h"

class_decl(mem_chunk);

class(raw_allocator) {
	mem_chunk *mem;
	size_t total;
};

constructor(raw_allocator);
distructor(raw_allocator);
void *raw_allocator_allocate(raw_allocator *self, size_t n, size_t meta_size);

#define ALLOCATOR_DECL(type) class_decl(allocator_ ## type)

#define ALLOCATOR_IMPL(type) \
class(allocator_ ## type) { \
	raw_allocator a; \
}; \
static inline allocator_ ## type *allocator_ ## type ## _new() { \
	return (allocator_ ## type*)new(raw_allocator); \
} \
static inline void allocator_ ## type ## _delete(allocator_ ## type *self) { \
	raw_allocator_distructor((raw_allocator*)self); \
	mem_free(self); \
} \
static inline type *allocator_ ## type ## _allocate(allocator_ ## type *self, size_t n) { \
	return (type*)raw_allocator_allocate((raw_allocator*)self, n, sizeof(type)); \
}

#define allocator(type) allocator_ ## type
#define allocator_new(type) allocator_ ## type ## _new
#define allocator_delete(type) allocator_ ## type ## _delete
#define allocator_allocate(type) allocator_ ## type ## _allocate
#define allocator_construct(type) type ## _constructor
#define allocator_distruct(type) type ## _distructor
