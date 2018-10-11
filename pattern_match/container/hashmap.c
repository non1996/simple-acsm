#include "hashmap.h"
#include "bitset.h"

#define HASHMAP_BLANKET_MAX 1024

constructor(hashmap, hash_fn hash, cmp_fn cmp, cpy_fn cpy, clr_fn clr) {
	int index;
	
	self->blanket = (pair**)mem_alloc_zero(pair*, HASHMAP_BLANKET_MAX);
	self->size = 0;
	
	for (index = 0; index < HASHMAP_BLANKET_MAX; ++index)
		self->blanket[index] = new(pair, nullptr, nullptr, nullptr);

	hash ? self->hash = hashmap_default_hash : self->hash;
	cmp ? self->cmp = hashmap_default_cmp : self->cmp;
	cpy ? self->cpy = hashmap_default_cpy : self->cpy;
	clr ? self->clr = hashmap_default_clr : self->clr;
	constructor_end;
}

distructor(hashmap) {
	hashmap_clear(self);
	mem_free(self->blanket);
}

void hashmap_clear(hashmap *self) {
	int index = 0;
	hashmap_pair *temp;
	for (index = 0; index < HASHMAP_BLANKET_MAX; ++index) {
		while (self->blanket[index]->next) {
			temp = self->blanket[index]->next;
			self->blanket[index]->next = self->blanket[index]->next->next;

			self->clr(temp->key);
			delete(hashmap_pair, temp);
		}		
	}
	self->size = 0;
}

//bool hashmap_remove(hashmap *self, void *key) {
//	size_t index;
//	hashmap_pair *curr, *prev;
//
//	index = self->hash(key);
//	prev = self->blanket[index];
//	curr = self->blanket[index]->next;
//
//	while (curr) {
//		if (self->cmp(curr->key, key)) {
//			prev->next = curr->next;
//			
//			self->clr(curr->key);
//			delete(hashmap_pair, curr);
//			
//			if (self->blanket[index]->next == nullptr)
//				bitset_clear(self->valid, index);
//			self->size--;
//			return true;
//		}
//		curr = curr->next;
//	}
//	return false;
//}

void * hashmap_insert_pair(hashmap *self, pair *p) {
	size_t index;
	hashmap_pair *curr, *prev;
	void *old_value;

	index = self->hash(p->key);
	prev = self->blanket[index];
	curr = self->blanket[index]->next;

	self->size++;

	while (curr) {
		if (self->cmp(curr->key, p->key)) {
			prev->next = p;
			p->next = curr->next;
			old_value = curr->value;
			
			self->clr(curr->key);
			delete(hashmap_pair, curr);

			return old_value;
		}
		curr = curr->next;
	}

	prev->next = p;
	return nullptr;
}

hashmap_pair * hashmap_find(hashmap *self, void *key) {
	hashmap_pair *curr;

	curr = self->blanket[self->hash(key)]->next;

	while (curr) {
		if (self->cmp(curr->key, key)) 
			return curr;
		curr = curr->next;
	}

	return nullptr;
}

constructor(hashmap_pair,
	hashmap *map, void *key, void *value) {
	self->key = map->cpy(key);
	self->value = value;
	constructor_end;
}

distructor(hashmap_pair) {

}

size_t hashmap_default_hash(void * key) {
	size_t res = (size_t)key;
	res = ~res + (res << 15);	// res = (res << 15) - res - 1;
	res = res ^ (res >> 12);
	res = res + (res << 2);
	res = res ^ (res >> 4);
	res = res * 2057;			// res = (res + (res << 3)) + (res << 11);
	res = res ^ (res >> 16);
	return res % HASHMAP_BLANKET_MAX;
}

bool hashmap_default_cmp(void * key1, void * key2) {
	return key1 == key2;
}

void * hashmap_default_cpy(void * key) {
	return key;
}

void hashmap_default_clr(void * key) {
	
}
