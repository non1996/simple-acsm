#include "ac.h"
#include "cstring.h"
#include "queue.h"
#include "log.h"
#include "file_stream.h"
#include "allocator.h"

static inline uint64_t make_key(p_ac_node *parent, wchar key) {
	uint64_t p = (uint64_t)parent << 16;
	return p | (uint64_t)key;
}

class(hashmap) {
	p_ac_node *blanket;
	size_t capacity;
	size_t size;
};

constructor(hashmap, size_t cap) {
	self->blanket = mem_alloc(p_ac_node, cap);
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

static inline void hashmap_insert(hashmap *self, uint64_t key, p_ac_node node) {
	size_t index = hashmap_hash(self, key);
	if (self->blanket[index])
		node->h_next = self->blanket[index]->h_next;
	self->blanket[index] = node;
}

static inline p_ac_node hashmap_find(hashmap *self, uint64_t key) {
	size_t index = hashmap_hash(self, key);
	p_ac_node iter = self->blanket[index];
	while (iter) {
		if (make_key(iter->parent, iter->key) == key)
			return iter;
		iter = iter->h_next;
	}
	return nullptr;
}

#define USE_FILE_STREAM 0
#define USE_STRING		1

static inline p_ac_node acsm_find_child(acsm *self, p_ac_node p, wchar key);
static inline ac_node *acsm_alloc_node(acsm * self, wchar key);
static inline bool acsm_should_insert(acsm *self, fixed_wstring *pattern, uint32_t ptn_id,
	p_ac_node *parent, bool *repeat);
static inline void acsm_add_child(acsm *self, p_ac_node parent, p_ac_node child);
static inline ac_node *acsm_construct_subtree(acsm * self, fixed_wstring *pattern, uint32_t pattern_id);
static inline void acsm_children_inqueue_helper(acsm *self, queue *q, rbtree_node *node);
static inline void acsm_find_fail(acsm *self, p_ac_node node);
static inline void acsm_children_inqueue(acsm *self, queue *q, p_ac_node p);
static inline bool acsm_search_getend(acsm *self);
static inline wchar acsm_curr_key(acsm *self);
static inline void acsm_next(acsm *self);

class(ac_node) {
	p_ac_node faillink, parent, children, h_next, child_next;
	uint32_t match_id;
	union {
		wchar key;
		uint32_t padding;
	};
};

ALLOCATOR_IMPL(ac_node);

#define ac_to_rb(node) ((rbtree_node*)node)
#define rb_to_ac(node) ((ac_node*)node)
#define children(node) (&(node->children))

constructor(ac_node, wchar key) {
	self->key = key;
	constructor_end;
}

constructor(acsm) {
	self->ac_alloc = allocator_new(ac_node)();
	self->root = acsm_alloc_node(self, 0x0000u);
	self->root->faillink = self->root;
	
	constructor_end;
}

distructor(acsm) {
	allocator_delete(ac_node)(self->ac_alloc);
}

static inline p_ac_node acsm_find_child(acsm *self, p_ac_node p, wchar key) {
	return hashmap_find(self->nodes, make_key(p, key));
}

static inline bool acsm_should_insert(acsm *self, fixed_wstring *pattern, 
	uint32_t ptn_id, p_ac_node *parent, bool *repeat) {
	p_ac_node node_iter = self->root, child_iter;

	assert(fixed_wstring_size(pattern));

	for (fixed_wstring_begin(pattern); !fixed_wstring_getend(pattern); fixed_wstring_next(pattern)) {
		child_iter = acsm_find_child(self, node_iter, fixed_wstring_get(pattern));
		//	rest nodes should be inserted as *parent's child
		if (!child_iter) {
			*parent = node_iter;
			return true;
		}
		node_iter = child_iter;
	}

	if (child_iter->match_id == 0)
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
	parent->children = child->child_next;
	hashmap_insert(self->nodes, make_key(parent, child->key), child);
}

static inline ac_node *acsm_construct_subtree(acsm * self, fixed_wstring *pattern, uint32_t pattern_id) {
	p_ac_node new_node, list, tail;

	new_node = acsm_alloc_node(self, fixed_wstring_get(pattern));
	list = tail = new_node;
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
	p_ac_node insert, branch_first;

	if (acsm_should_insert(self, pattern, ptn_id, &insert, &repeat)) {
		branch_first = acsm_construct_subtree(self, pattern, ptn_id);
		acsm_add_child(self, insert, branch_first);
		self->pattern_num++;
		self->is_prep = false;
		return true;
	}

	if (!repeat) {
		self->pattern_num++;
		self->is_prep = false;
		return true;
	}
	return false;
}

static inline void acsm_children_inqueue_helper(acsm *self, queue *q, rbtree_node *node) {
	if (node == self->nil)
		return;
	queue_push(q, node);
	acsm_children_inqueue_helper(self, q, node->left);
	acsm_children_inqueue_helper(self, q, node->right);
}

static inline void acsm_children_inqueue(acsm *self, queue *q, p_ac_node p) {
	acsm_children_inqueue_helper(self, q, children(p)->root);
}

static inline void acsm_find_fail(acsm *self, p_ac_node node) {
	p_ac_node fail_try = node->parent->faillink;

	if (node->parent == self->root) {
		node->faillink = self->root;
		return;
	}

	while (true) {
		node->faillink = acsm_find_child(self, fail_try, ac_to_rb(node)->key);
		if (node->faillink != nullptr)
			break;
		if (fail_try == self->root) {
			node->faillink = self->root;
			break;
		}
		fail_try = fail_try->faillink;
	}
}

void acsm_prepare(acsm * self) {
	p_ac_node curr;
	queue *wsqueue = new(queue);
	
	acsm_children_inqueue(self, wsqueue, self->root);

	while ((curr = queue_pop(wsqueue)) != nullptr) {
		acsm_children_inqueue(self, wsqueue, curr);
		acsm_find_fail(self, curr);
	}

	self->is_prep = true;
	self->is_init = false;
	delete(queue, wsqueue);
}

void acsm_search_init(acsm * self, output_handle cb, void * cb_arg) {
	self->curr_node = self->root;
	self->cb = cb;
	self->cb_arg = cb_arg;
	self->is_init = true;
	self->is_end = false;
}

bool acsm_search_init_file(acsm * self, file_stream * fs, output_handle cb, void * cb_arg) {
	if (!self->is_prep) {
		return false;
	}

	self->fs = fs;
	self->search_type = USE_FILE_STREAM;
	acsm_search_init(self, cb, cb_arg);
	return true;
}

bool acsm_search_init_string(acsm * self, fixed_wstring * text, output_handle cb, void *cb_arg) {
	if (!self->is_prep) {
		return false;
	}

	self->text = text;
	self->search_type = USE_STRING;
	acsm_search_init(self, cb, cb_arg);
	return true;
}

static inline bool acsm_search_getend(acsm *self) {
	if (self->search_type == USE_STRING)
		return fixed_wstring_getend(self->text);
	else
		return file_stream_getend(self->fs);
}

static inline wchar acsm_curr_key(acsm *self) {	// < :
	if (self->search_type == USE_STRING)
		return fixed_wstring_get(self->text);
	else
		return file_stream_get(self->fs);
}

static inline void acsm_next(acsm *self) {
	if (self->search_type == USE_STRING)
		fixed_wstring_next(self->text);
	else if (self->search_type == USE_FILE_STREAM)
		file_stream_next(self->fs);
}

static inline void acsm_output(acsm *self, p_ac_node start) {
	do {
		if (start->match_id != 0)
			self->cb(start->match_id, self->cb_arg);
		start = start->faillink;
	} while (start != self->root);
}

bool acsm_search_ac(acsm *self) {
	p_ac_node child;

	if (!self->is_prep || !self->is_init || self->is_end) {
		//	log_error
		return false;
	}

	while (!acsm_search_getend(self)) {
		child = acsm_find_child(self, self->curr_node, acsm_curr_key(self));
		
		if (!child) {
			if (self->curr_node == self->root) {
				acsm_next(self);
				continue;
			}
			self->curr_node = self->curr_node->faillink;
		}
		else
			self->curr_node = child;

		if (self->curr_node->match_id != 0)
			acsm_output(self, self->curr_node);

		acsm_next(self);
	}

	self->is_end = true;

	return true;
}

bool acsm_search_trie(acsm * self, fixed_wstring * token) {
	p_ac_node curr, child;
	if (!self->is_prep || !self->is_init || self->is_end) {
		//	log_error
		return false;
	}

	curr = self->root;
	for (fixed_wstring_begin(token); !fixed_wstring_getend(token); fixed_wstring_next(token)) {
		child = acsm_find_child(self, curr, fixed_wstring_get(token));
		if (!child)
			return false;
		curr = child;
	}

	self->cb(curr->match_id, self->cb_arg);

	return true;
}
