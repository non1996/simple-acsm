#include "allocator.h"
#include "mem_chunk.h"

#define MEMCHUNK_DEFAULT_CAP 1024

constructor(raw_allocator) {
	constructor_end;
}

distructor(raw_allocator) {
	mem_chunk *temp;
	while (self->mem) {
		temp = self->mem;
		self->mem = self->mem->next;
		mem_chunk_delete(temp);
	}
}

void *raw_allocator_allocate(raw_allocator * self, size_t n, size_t meta_size) {
	size_t real = (n <= MEMCHUNK_DEFAULT_CAP) ? MEMCHUNK_DEFAULT_CAP : n;
	mem_chunk *new_chunk;
	if (!self->mem) {
		self->mem = mem_chunk_new(real * meta_size);
	}
	else if (!mem_chunk_enough(self->mem, n * meta_size)) {
		new_chunk = mem_chunk_new(real * meta_size);
		new_chunk->next = self->mem;
		self->mem = new_chunk;	
	}
	self->total += n * meta_size;
	return mem_chunk_use(self->mem, n * meta_size);
}
