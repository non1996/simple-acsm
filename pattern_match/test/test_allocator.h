#pragma once

#include "mem_chunk.h"
#include "allocator.h"

ALLOCATOR_IMPL(int)

void test_allocator() {
	mem_chunk *chunk;

	chunk = mem_chunk_new(1024 * 16);

	assert(chunk->cap == 1024 * 16);

	chunk->block[0] = (char)0x01;
	chunk->block[1] = (char)0x01;
	chunk->block[2] = (char)0x01;
	chunk->block[3] = (char)0x01;

	assert(0x01010101 == (*(int*)(chunk->block)));

	int *a, *b;
	allocator(int) *al = allocator_new(int)();
	raw_allocator *rl;

	a =	allocator_allocate(int)(al, 32);
	b = allocator_allocate(int)(al, 1024);

	rl = (raw_allocator*)al;

	assert(rl->total == (32 + 1024) * sizeof(int));
	assert(rl->mem);
	assert(rl->mem->next);

	allocator_delete(int)(al);
}