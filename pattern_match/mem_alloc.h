#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H

#include <stdlib.h>
#include <assert.h>

static inline void *mem_alloc_fn(size_t size) {
	void *buf;
	if (size == 0)
		return NULL;
	buf = malloc(size);
	return buf;
}

static inline void *mem_alloc_zero_fn(size_t size) {
	void *buf;
	if (size == 0)
		return NULL;
	buf = mem_alloc_fn(size);
	memset(buf, 0, size);
	return buf;
}

static inline void *mem_realloc_fn(void *ptr, size_t new_size) {
	void *buf = realloc(ptr, new_size);
	assert(buf != NULL);
	return buf;
}

#define mem_alloc(type, num) (type*)mem_alloc_fn(num * sizeof(type))
#define mem_alloc_zero(type, num) (type*)mem_alloc_zero_fn(num * sizeof(type))
#define mem_realloc(type, ptr, new_num) (type*)mem_realloc_fn(ptr, new_num * sizeof(type));
#define mem_free(ptr) do { free((ptr)); (ptr) = NULL; }while(0)

#endif // !MEM_ALLOC_H

