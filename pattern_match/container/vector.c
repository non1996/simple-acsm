#include "vector.h"

#define VECTOR_DEFAULT_CAP 16

static void vector_check(vector *self, size_t expected);

constructor(vector) {
	self->buffer = mem_alloc_zero(void *, VECTOR_DEFAULT_CAP);
	self->size = 0;
	self->capacity = VECTOR_DEFAULT_CAP;
	constructor_end;
}

distructor(vector) {
	mem_free(self->buffer);
}

size_t vector_size(vector * self) {
	return self->size;
}

void vector_clear(vector *self) {
	self->size = 0;
}

void * vector_at(vector *self, size_t index) {
	assert(index < self->size);
	return self->buffer[index];
}

void vector_reserve(vector *self, size_t capacity) {
	self->buffer = mem_realloc(void*, self->buffer, capacity);
	if (self->size > capacity)
		self->size = capacity;
	self->capacity = capacity;
}

void vector_push_back(vector *self, void *item) {
	vector_check(self, self->size + 1);

	self->buffer[self->size++] = item;
}

void * vector_pop_back(vector *self) {
	assert(self->size > 0);
	return self->buffer[--self->size];
}

void * vector_front(vector *self) {
	assert(self->size != 0);
	return self->buffer[0];
}

void * vector_back(vector *self) {
	assert(self->size != 0);
	return self->buffer[self->size - 1];
}

void vector_erase(vector *self, void *data) {
	size_t index;
	for (index = 0; index < self->size; ++index)
		if (self->buffer[index] == data) 
			vector_erase_idx(self, index);	
}

void vector_erase_keep_order(vector *self, void *data) {
	size_t index;
	for (index = 0; index < self->size; ++index)
		if (self->buffer[index] == data) 
			vector_erase_keep_order_idx(self, index);	
}

void vector_erase_idx(vector *self, size_t index) {
	assert(index < self->size);
	self->buffer[index] = self->buffer[--self->size];
}

void vector_erase_keep_order_idx(vector *self, size_t index) {
	assert(index < self->size);

	if (index == self->size - 1) {
		--self->size;
		return;
	}

	memcpy(self->buffer + index, 
		   self->buffer + index + 1, 
		   (--self->size - index) * sizeof(void*));
}

void vector_insert(vector *self, void *data, size_t index) {
	vector_check(self, ++self->size);

	memmove(self->buffer + index + 1,
		    self->buffer + index,
		    (self->size - index - 1) * sizeof(void*));
	self->buffer[index] = data;
}

void vector_sort(vector * self, int(*cmp)(const void *, const void *)) {
	qsort(self->buffer, self->size, sizeof(void*), cmp);
}

void vector_check(vector * self, size_t expected) {
	while (self->capacity < expected)
		self->capacity = self->capacity * 2;
	
	self->buffer = mem_realloc
		(void *, self->buffer, self->capacity);
}
