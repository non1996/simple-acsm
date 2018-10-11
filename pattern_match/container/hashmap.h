#ifndef HASHMAP_H
#define HASHMAP_H

#include "afx.h"

class_decl(pair);

typedef size_t(*hash_fn)(void *key);
typedef bool(*cmp_fn)(void *key1, void *key2);

class(hashmap) {
	pair **blanket;
	size_t size;
	size_t capacity;

	hash_fn hash;
	cmp_fn cmp;
};

constructor(hashmap, size_t cap, hash_fn hash, cmp_fn cmp);
distructor(hashmap);

static inline bool hashmap_empty(hashmap *self) {
		return self->size == 0;
}

void hashmap_clear(hashmap *self);
//bool hashmap_remove(hashmap *self, void *key);
void * hashmap_insert(hashmap *self, pair *p);
pair * hashmap_find(hashmap *self, void *key);

class(pair) {
	pair *next;
};

#define PAIR_DECL(key_type, value_type) \
class(pair_ ## key_type ## _ ## value_type) { \
	pair p; \
	key_type key; \
	value_type value; \
}; \
static inline key_type pair_key(pair_ ## key_type ## _ ## value_type *self) { \
	return self->key; \
} \
static inline value_type pair_value(pair_ ## key_type ## _ ## value_type *self) { \
	return self->value; \
} 



#endif // !HASHMAP_H
