#include "ac.h"
#include "cstring.h"
#include "queue.h"
#include "log.h"
#include "file_stream.h"
#include "allocator.h"

class(ac_node) {
	p_ac_node h_next;										//	next node in hashmap blanket 
	p_ac_node child_next, fail, parent, children;
	uint32_t match_id;										//	pattern who end on this node 
	union {
		wchar key;								
		uint32_t padding;									
	};
};

//	hash_key = 0000 0000 0000 0000 | parent(32-bit) | key(8-bit) 	
static inline uint64_t make_key(p_ac_node parent, wchar key) {
	uint64_t p = (uint64_t)parent << 16;
	return p | (uint64_t)key;
}

class(hashmap) {
	p_ac_node *blanket;
	size_t capacity;
	size_t size;
};

constructor(hashmap, size_t cap) {
	self->blanket = mem_alloc_zero(p_ac_node, cap);
	self->capacity = cap;
	constructor_end;
}

distructor(hashmap) {
	mem_free(self->blanket);
}

static inline size_t hashmap_hash(hashmap *self, uint64_t key) {
	uint64_t res = key;
	res = ~res + (res << 15);	// res = (res << 15) - res - 1;
	res = res ^ (res >> 12);
	res = res + (res << 2);
	res = res ^ (res >> 4);
	res = res * 2057;			// res = (res + (res << 3)) + (res << 11);
	res = res ^ (res >> 16);
	return (size_t)(res % self->capacity);
}

//	Count how many blankets have elements.
static inline size_t hashmap_full(hashmap *self) {
	size_t index, count = 0;
	for (index = 0; index < self->capacity; ++index)
		if (self->blanket[index])
			count++;
	return count;
}

static inline size_t hashmap_deepest(hashmap *self) {
	size_t deepest = 0, depth, index;
	p_ac_node node;
	for (index = 0; index < self->capacity; ++index) {
		depth = 0;
		node = self->blanket[index];
		while (node) {
			depth++;
			node = node->h_next;
		}
		if (depth > deepest)
			deepest = depth;
	}
	return deepest;
}

static inline void hashmap_insert(hashmap *self, p_ac_node node) {
	size_t index = hashmap_hash(self, make_key(node->parent, node->key));
	node->h_next = self->blanket[index];
	self->blanket[index] = node;
	self->size++;
}

static inline p_ac_node hashmap_find(hashmap *self, uint64_t key) {
	size_t index = hashmap_hash(self, key);
	p_ac_node iter = self->blanket[index];
	size_t count = 0;
	while (iter) {
		if (make_key(iter->parent, iter->key) == key)
			return iter;
		iter = iter->h_next;
		count++;
	}
	return nullptr;
}

static inline void hashmap_resize(hashmap *self, size_t new_cap) {
	assert(new_cap != 0);

	size_t index, count = 0;
	size_t old_cap = self->capacity, old_size = self->size;
	p_ac_node iter, next;
	p_ac_node *old_blanket = self->blanket;
	p_ac_node *new_blanket = mem_alloc_zero(p_ac_node, new_cap);

	self->blanket = new_blanket;
	self->capacity = new_cap;
	self->size = 0;

	for (index = 0; index < old_cap; ++index) {
		iter = old_blanket[index];
		while (iter) {
			next = iter->h_next;
			hashmap_insert(self, iter);
			iter = next;
			count++;
		}
	}
	mem_free(old_blanket);
}

#define USE_FILE_STREAM		0
#define USE_STRING			1

#define NO_MATCH			0

#define callback(func, id, arg) do { if (func) func(id, arg); }while(0)

static inline p_ac_node acsm_find_child(acsm *self, p_ac_node p, wchar key);
static inline ac_node *acsm_alloc_node(acsm * self, wchar key);
static inline bool acsm_should_insert(acsm *self, fixed_wstring *pattern, uint32_t ptn_id,
	p_ac_node *parent, bool *repeat);
static inline void acsm_add_child(acsm *self, p_ac_node parent, p_ac_node child);
static inline ac_node *acsm_construct_branch(acsm * self, fixed_wstring *pattern, uint32_t pattern_id);
static inline void acsm_find_fail(acsm *self, p_ac_node node);
static inline void acsm_children_inqueue(acsm *self, queue *q, p_ac_node p);

static inline bool acsm_search_getend(acsm *self);
static inline wchar acsm_curr_key(acsm *self);
static inline void acsm_next(acsm *self);

ALLOCATOR_IMPL(ac_node);

constructor(ac_node, wchar key) {
	self->h_next = nullptr;
	self->children = nullptr;
	self->child_next = nullptr;
	self->parent = nullptr;
	self->fail = nullptr;
	self->key = key;
	constructor_end;
}

distructor(ac_node) { }

constructor(acsm) {
	self->ac_alloc = allocator_new(ac_node)();
	self->root = acsm_alloc_node(self, 0x0000u);
	self->root->fail = self->root;
	self->nodes = new(hashmap, 1024 * 1024);
	
	constructor_end;
}

distructor(acsm) {
	allocator_delete(ac_node)(self->ac_alloc);
	delete(hashmap, self->nodes);
}

static inline p_ac_node acsm_find_child(acsm *self, p_ac_node parent, wchar key) {
	return hashmap_find(self->nodes, make_key(parent, key));
}

static inline bool acsm_should_insert(acsm *self, fixed_wstring *pattern, 
	uint32_t ptn_id, p_ac_node *parent, bool *repeat) {
	p_ac_node node_iter = self->root, child_iter;

	assert(fixed_wstring_size(pattern));

	for (fixed_wstring_begin(pattern); !fixed_wstring_getend(pattern); fixed_wstring_next(pattern)) {
		child_iter = acsm_find_child(self, node_iter, fixed_wstring_get(pattern));
		if (!child_iter) {
			*parent = node_iter;
			return true;
		}
		node_iter = child_iter;
	}

	if (child_iter->match_id == NO_MATCH)
		child_iter->match_id = ptn_id;
	else
		*repeat = true;
	return false;
}

static inline ac_node *acsm_alloc_node(acsm * self, wchar key) {
	ac_node *new_node;
	new_node = allocator_allocate(ac_node)(self->ac_alloc, 1);
	allocator_construct(ac_node)(new_node, key);
	return new_node;
}

static inline void acsm_add_child(acsm *self, p_ac_node parent, p_ac_node child) {
	child->parent = parent;
	child->child_next = parent->children;
	parent->children = child;
	hashmap_insert(self->nodes, child);
}

static inline ac_node *acsm_construct_branch(acsm * self, fixed_wstring *pattern, uint32_t pattern_id) {
	p_ac_node new_node, list, tail;

	list = tail = acsm_alloc_node(self, fixed_wstring_get(pattern));
	fixed_wstring_next(pattern);
	
	for (; !fixed_wstring_getend(pattern); fixed_wstring_next(pattern)) {
		new_node = acsm_alloc_node(self, fixed_wstring_get(pattern));
		acsm_add_child(self, tail, new_node);
		tail = new_node;
	}

	tail->match_id = pattern_id;
	return list;
}

bool acsm_add_pattern(acsm * self, fixed_wstring *pattern, uint32_t ptn_id) {
	bool repeat = false;
	p_ac_node insert;

	if (acsm_should_insert(self, pattern, ptn_id, &insert, &repeat))
		acsm_add_child(self, insert, 
			acsm_construct_branch(self, pattern, ptn_id));

	if (!repeat) {
		self->pattern_num++;
		return true;
	}
	return false;
}

static inline void acsm_children_inqueue(acsm *self, queue *q, p_ac_node parent) {
	p_ac_node child = parent->children;
	while (child) {
		queue_push(q, child);
		child = child->child_next;
	}
}

static inline void acsm_find_fail(acsm *self, p_ac_node node) {
	p_ac_node fail_try = node->parent->fail;

	if (node->parent == self->root) {
		node->fail = self->root;
		return;
	}

	while (true) {
		node->fail = acsm_find_child(self, fail_try, node->key);
		if (node->fail != nullptr)
			break;
		if (fail_try == self->root) {
			node->fail = self->root;
			break;
		}
		fail_try = fail_try->fail;
	}
}

void acsm_compile(acsm * self) {
	p_ac_node curr;
	queue *wsqueue = new(queue);
	
	log_notice("Prepare acsm by finding faillink.");
	log_debug("Hashmap blanket resize to %ul", self->nodes->size);
	log_debug("Before hashmap rehash, used blanket %ul/%ul, deepest blanket %ul.", hashmap_full(self->nodes), self->nodes->capacity, hashmap_deepest(self->nodes));
	hashmap_resize(self->nodes, self->nodes->size);
	log_debug("Hashmap used blanket %ul/%ul, deepest blanket %ul.", hashmap_full(self->nodes), self->nodes->capacity, hashmap_deepest(self->nodes));

	log_debug("Finding faillink.");
	acsm_children_inqueue(self, wsqueue, self->root);

	while ((curr = queue_pop(wsqueue)) != nullptr) {
		acsm_children_inqueue(self, wsqueue, curr);
		acsm_find_fail(self, curr);
	}

	delete(queue, wsqueue);
	log_debug("Acsm prepared.");
}

void acsm_search_init(acsm * self, match_handle cb, void * cb_arg) {
	self->curr_node = self->root;
	self->cb = cb;
	self->cb_arg = cb_arg;
}

bool acsm_search_init_file(acsm * self, file_stream * fs, match_handle cb, void * cb_arg) {
	self->fs = fs;
	self->input_type = USE_FILE_STREAM;
	acsm_search_init(self, cb, cb_arg);
	return true;
}

bool acsm_search_init_string(acsm * self, fixed_wstring * text, match_handle cb, void *cb_arg) {
	self->text = text;
	self->input_type = USE_STRING;
	acsm_search_init(self, cb, cb_arg);
	return true;
}

static inline bool acsm_search_getend(acsm *self) {
	if (self->input_type == USE_STRING)
		return fixed_wstring_getend(self->text);
	else
		return file_stream_getend(self->fs);
}

static inline wchar acsm_curr_key(acsm *self) {	// < :
	if (self->input_type == USE_STRING)
		return fixed_wstring_get(self->text);
	else
		return file_stream_get(self->fs);
}

static inline void acsm_next(acsm *self) {
	if (self->input_type == USE_STRING)
		fixed_wstring_next(self->text);
	else if (self->input_type == USE_FILE_STREAM)
		file_stream_next(self->fs);
}

static inline void acsm_match(acsm *self, p_ac_node start) {
	do {
		if (start->match_id != NO_MATCH)
			callback(self->cb, start->match_id, self->cb_arg);
		start = start->fail;
	} while (start != self->root);
}

bool acsm_search_ac(acsm *self) {
	p_ac_node child;

	log_notice("Start searching.");

	while (!acsm_search_getend(self)) {
		child = acsm_find_child(self, self->curr_node, acsm_curr_key(self));
		
		if (!child) {
			if (self->curr_node == self->root) {
				acsm_next(self);
				continue;
			}
			self->curr_node = self->curr_node->fail;
		}
		else
			self->curr_node = child;

		if (self->curr_node->match_id != NO_MATCH)
			acsm_match(self, self->curr_node);

		acsm_next(self);
	}

	log_notice("Searching completed.");

	return true;
}

bool acsm_search_trie(acsm * self, fixed_wstring * token) {
	p_ac_node curr, child;

	curr = self->root;
	for (fixed_wstring_begin(token); !fixed_wstring_getend(token); fixed_wstring_next(token)) {
		child = acsm_find_child(self, curr, fixed_wstring_get(token));
		if (!child)
			return false;
		curr = child;
	}

	if (curr->match_id != NO_MATCH)
		callback(self->cb, curr->match_id, self->cb_arg);

	return true;
}
