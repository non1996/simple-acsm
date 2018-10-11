#include "ac.h"
#include "cstring.h"
#include "queue.h"
//#include "bitset.h"
#include "log.h"
#include "file_stream.h"
#include "rbtree.h"
#include "allocator.h"

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
	rbtree_node rb;
	p_ac_node faillink, parent;
	rbtree children;
	uint32_t match_id;
};

ALLOCATOR_IMPL(ac_node);

#define ac_to_rb(node) ((rbtree_node*)node)
#define rb_to_ac(node) ((ac_node*)node)
#define children(node) (&(node->children))

constructor(ac_node, rbtree_node *nil, wchar key) {
	rbtree_node_constructor(ac_to_rb(self), RB_RED);
	rbtree_constructor(children(self), nil);
	self->rb.left = nil;
	self->rb.right = nil;
	self->rb.key = key;
	constructor_end;
}

constructor(acsm) {
	self->ac_alloc = allocator_new(ac_node)();
	//self->patterns = new(bitset, 2260000);
	self->nil = new(rbtree_node, RB_BLACK);
	self->root = acsm_alloc_node(self, 0x0000u);
	self->root->faillink = self->root;
	
	constructor_end;
}

distructor(acsm) {
	allocator_delete(ac_node)(self->ac_alloc);
	delete(rbtree_node, self->nil);
}

static inline p_ac_node acsm_find_child(acsm *self, p_ac_node p, wchar key) {
	return rb_to_ac(rbtree_find(children(p), self->nil, key));
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
	allocator_construct(ac_node)(new_node, self->nil, key);
	return new_node;
}

static inline void acsm_add_child(acsm *self, p_ac_node parent, p_ac_node child) {
	rbtree_insert(children(parent), ac_to_rb(child), self->nil);
	child->parent = parent;
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

	//if (bitset_is_set(self->patterns, ptn_id)) {
	//	log_debug("Repeated pattern id [%d]", ptn_id);
	//	return false;
	//}

	if (acsm_should_insert(self, pattern, ptn_id, &insert, &repeat)) {
		branch_first = acsm_construct_subtree(self, pattern, ptn_id);
		acsm_add_child(self, insert, branch_first);
		//bitset_set(self->patterns, ptn_id);
		self->pattern_num++;
		self->is_prep = false;
		return true;
	}

	if (!repeat) {
		//bitset_set(self->patterns, ptn_id);
		self->pattern_num++;
		self->is_prep = false;
		return true;
	}
	//log_debug("Repeated pattern [%s]", pattern->f.buffer);
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
