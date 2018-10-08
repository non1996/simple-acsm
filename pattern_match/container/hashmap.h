#ifndef HASHMAP_H
#define HASHMAP_H

#include "afx.h"

class_decl(hashmap_pair);
class_decl(hashmap_iterator);
class_decl(bitset);

typedef size_t(*hash_fn)(void *key);
typedef bool(*cmp_fn)(void *key1, void *key2);
typedef void*(*cpy_fn)(void *key);
typedef void(*clr_fn)(void *key);

class(hashmap) {
	hashmap_pair **blanket;
	bitset *valid;
	size_t size;

	hash_fn hash;
	cmp_fn cmp;
	cpy_fn cpy;
	clr_fn clr;
};

constructor(hashmap, hash_fn hash, cmp_fn cmp, cpy_fn cpy, clr_fn clr);
distructor(hashmap);

bool hashmap_empty(hashmap *self);
void hashmap_clear(hashmap *self);
bool hashmap_remove(hashmap *self, void *key);
void * hashmap_insert(hashmap *self, void *key, void *value);
hashmap_pair * hashmap_find(hashmap *self, void *key);

class(hashmap_pair) {
	void *key;
	void *value;
	hashmap_pair *next;
};

distructor(hashmap_pair);

#endif // !HASHMAP_H
