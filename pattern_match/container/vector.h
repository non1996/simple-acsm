#ifndef VECTOR_H
#define VECTOR_H

#include "afx.h"

class(vector) {
	void **buffer;
	size_t size;
	size_t capacity;
};

constructor(vector);
distructor(vector);

size_t vector_size(vector *self);
void vector_clear(vector *self);
void * vector_at(vector *self, size_t index);
void vector_reserve(vector *self, size_t capacity);
void vector_push_back(vector *self, void *item);
void * vector_pop_back(vector *self);
void * vector_front(vector *self);
void * vector_back(vector *self);
void vector_erase(vector *self, void *data);
void vector_erase_keep_order(vector *self, void *data);
void vector_erase_idx(vector *self, size_t index);
void vector_erase_keep_order_idx(vector *self, size_t index);
void vector_insert(vector *self, void *data, size_t index);
void vector_sort(vector *self, int(*cmp)(const void *, const void *));


#define vector_foreach(type, elem, l) \
{ \
	uint32_t index; \
	for (index = 0; index < l->size; ++index) { \
		type *elem = (type*)vector_at(l, index);

#define vector_foreach_end }}

#define vector_deep_clear(type, list) \
do { \
	vector_foreach(type, elem, (list)) \
		delete(type, elem); \
	vector_foreach_end \
} while(0)

#endif // !VECTOR_H

