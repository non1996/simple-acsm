#include "ac.h"
#include "cstring.h"
#include "queue.h"
#include "bitset.h"
#include "log.h"
#include "file_stream.h"

#define USE_FILE_STREAM 0
#define USE_STRING		1

#define RB_RED			0
#define RB_BLACK		1

class(rbtree_node) {
	rbtree_node *parent, *left, *right;
	wchar key;
	uint8_t color;
	uint8_t padding;
};

class(rbtree) {
	rbtree_node *root;
};

constructor(rbtree_node) {
	constructor_end;
}

distructor(rbtree_node) {

}

constructor(rbtree) {

}

distructor(rbtree) {

}

#define rb_parent(node) node->parent
#define rb_left(node) node->left
#define rb_right(node) node->right
#define rb_grand(node) node->parent->parent
#define rb_right_uncle(node) rb_grand(node)->right
#define rb_left_uncle(node) rb_grand(node)->left

static inline void rbtree_left_rotate(rbtree *self, rbtree_node *curr, rbtree_node *nil) {
	rbtree_node *curr_right = curr->right;
	curr->right = curr_right->left;
	if (curr_right != nil)
		curr_right->left->parent = curr;
	curr_right->parent = curr->parent;
	if (curr->parent == nil)
		self->root = curr_right;
	else if (curr == curr->parent->left)
		curr->parent->left = curr_right;
	else
		curr->parent->right = curr_right;

	curr_right->left = curr;
	curr->parent = curr_right;
}

static inline void rbtree_right_rotate(rbtree *self, rbtree_node *curr, rbtree_node *nil) {
	rbtree_node *curr_left = curr->left;
	curr->left = curr_left->right;
	if (curr_left->right != nil)
		curr_left->right->parent = curr;
	if (curr->parent == nil)
		self->root = curr_left;
	else if (curr == curr->parent->left)
		curr->parent->left = curr_left;
	else
		curr->parent->right = curr_left;

	curr_left->right = curr;
	curr->parent = curr_left;
}

static inline void rbtree_fix(rbtree *self, rbtree_node *start, rbtree_node *nil) {
	while (rb_parent(start)->color == RB_RED) {
		if (rb_parent(start) == rb_grand(start)->left) {
			if (rb_right_uncle(start)->color == RB_RED) {
				rb_parent(start)->color = RB_BLACK;
				rb_right_uncle(start)->color = RB_BLACK;
				rb_grand(start)->color = RB_RED;
				start = rb_grand(start);
			}
			else if (start == rb_parent(start)->right) {
				start = rb_parent(start);
				rbtree_left_rotate(self, start, nil);
			}
			rb_parent(start)->color = RB_BLACK;
			rb_grand(start)->color = RB_RED;
			rbtree_right_rotate(self, rb_grand(start), nil);
		}
		else {
			if (rb_left_uncle(start)->color == RB_RED) {
				rb_parent(start)->color = RB_BLACK;
				rb_left_uncle(start)->color = RB_BLACK;
				rb_grand(start)->color = RB_RED;
				start = rb_grand(start);
			}
			else if (start == rb_parent(start)->left) {
				start = rb_parent(start);
				rbtree_right_rotate(self, start, nil);
			}
			rb_parent(start)->color = RB_BLACK;
			rb_grand(start)->color = RB_RED;
			rbtree_left_rotate(self, rb_grand(start), nil);
		}
	}
	self->root->color = RB_BLACK;
}

static inline void rbtree_insert(rbtree *self, rbtree_node *node, rbtree_node *nil) {
	rbtree_node *iter = self->root;
	rbtree_node *parent = nil;
	while (iter != nil) {
		parent = iter;
		if (node->key < iter->key)
			iter = iter->left;
		else
			iter = iter->right;
	}

	node->parent = parent;
	if (parent == nil)
		self->root = node;
	else if (node->key < parent->key)
		parent->left = node;
	else
		parent->right = node;
	node->left = node->right = nil;
	node->color = RB_RED;
	rbtree_fix(self, node, nil);
}

static inline rbtree_node *rbtree_find(rbtree *self, rbtree_node *nil, wchar key) {
	rbtree_node *iter = self->root;
	while (iter != nil) {
		if (iter->key == key)
			return iter;
		iter = key < iter->key ? iter->left : iter->right;
	}
	return nullptr;
}

class(ac_node) {
	rbtree_node rb;
	p_ac_node faillink, parent;
	rbtree children;
	uint32_t match_id;
};

#define ac_to_rb(node) ((rbtree_node*)node)
#define rb_to_ac(node) ((ac_node*)node)
#define children(node) (&(node->children))

constructor(acsm) {
	self->root = mem_alloc_zero(ac_node, 1);
	self->root->faillink = self->root;
	self->patterns = new(bitset, 2260000);
	self->nil = new(rbtree_node);

	constructor_end;
}

distructor(acsm) {
	delete(bitset, self->patterns);
}

static inline p_ac_node acsm_find_child(acsm *self, p_ac_node p, wchar key) {
	if (!children(p)->root)
		return nullptr;

	return rbtree_find(children(p), self->nil, key);
}

static inline bool acsm_should_insert(acsm *self, fixed_wstring *pattern, uint32_t ptn_id, 
									p_ac_node *parent, uint32_t *rest, bool *repeat) {
	p_ac_node node_iter = self->root, child_iter;
	size_t pattern_length = fixed_wstring_size(pattern);
	uint32_t index;

	for (index = 0; index < pattern_length; ++index) {
		child_iter = acsm_find_child(self, node_iter, fixed_wstring_at(pattern, index));

		//	rest nodes should be inserted as *parent's child
		if (!child_iter) {
			*parent = node_iter;
			*rest = index;
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

static inline ac_node *acsm_new_node(wchar key) {
	p_ac_node new_node;

	new_node = mem_alloc_zero(ac_node, 1);
	ac_to_rb(new_node)->key = key;
	return new_node;
}

static inline ac_node *acsm_construct_subtree(acsm * self, fixed_wstring *pattern, size_t start, uint32_t pattern_id) {
	p_ac_node new_node, list = nullptr, tail;
	size_t pattern_length = fixed_wstring_size(pattern);
	uint32_t index;

	new_node = acsm_new_node(fixed_wstring_at(pattern, start));
	list = tail = new_node;

	for (index = start + 1; index < pattern_length; ++index) {
		new_node = acsm_new_node(fixed_wstring_at(pattern, index));
		rbtree_insert(children(tail), ac_to_rb(new_node), self->nil);
		new_node->parent = tail;
		tail = new_node;
	}

	tail->match_id = pattern_id;
	return list;
}

bool acsm_add_pattern(acsm * self, fixed_wstring *pattern, uint32_t ptn_id) {
	bool repeat = false;
	int start_index;
	p_ac_node insert, branch_list;

	if (bitset_is_set(self->patterns, ptn_id)) {
		return false;
	}

	if (acsm_should_insert(self, pattern, ptn_id, &insert, &start_index, &repeat)) {
		branch_list = acsm_construct_subtree(self, pattern, start_index, ptn_id);
		branch_list->parent = insert;
		rbtree_insert(children(insert), ac_to_rb(branch_list), self->nil);
		bitset_set(self->patterns, ptn_id);
		self->pattern_num++;
		self->is_prep = false;
		return true;
	}

	if (!repeat) {
		bitset_set(self->patterns, ptn_id);
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
	if (!children(p)->root)
		return;
	acsm_children_inqueue(self, q, children(p)->root);
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

static inline void acsm_search_init(acsm * self, output_handle cb, void * cb_arg) {
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
	self->curr_index = 0;
	self->search_type = USE_STRING;
	acsm_search_init(self, cb, cb_arg);
	return true;
}

static inline bool acsm_search_getend(acsm *self) {
	if (self->search_type == USE_STRING)
		return self->curr_index >= fixed_wstring_size(self->text);
	else
		return file_stream_getend(self->fs);
}

static inline wchar acsm_curr_key(acsm *self) {	// < :
	if (self->search_type == USE_STRING)
		return fixed_wstring_at(self->text, self->curr_index);
	else
		return file_stream_get(self->fs);
}

static inline void acsm_next(acsm *self) {
	if (self->search_type == USE_STRING)
		++self->curr_index;
	else if (self->search_type == USE_FILE_STREAM)
		file_stream_next(self->fs);
}

bool acsm_search(acsm *self) {
	p_ac_node child_iter;

	if (!self->is_prep || !self->is_init || self->is_end) {
		//	log_error
		return false;
	}

	while (!acsm_search_getend(self)) {
		child_iter = acsm_find_child(self, self->curr_node, acsm_curr_key(self));
		
		if (!child_iter) {
			if (self->curr_node == self->root)
				acsm_next(self);
			
			self->curr_node = self->curr_node->faillink;

			if (self->curr_node->match_id !=0)
				self->cb(self->curr_node->match_id, self->cb_arg);

			continue;
		}
			
		self->curr_node = child_iter;

		if (self->curr_node->match_id != 0)
			self->cb(self->curr_node->match_id, self->cb_arg);

		acsm_next(self);
	}

	self->is_end = true;

	return true;
}

//bool acsm_del_pattern(acsm * self, fixed_string *pattern, uint32_t ptn_id) {
//	uint32_t i;
//	p_ac_node node_iter, branch, branch_prev, child, child_prev;
//	bool del_branch;
//
//	if (!bitset_is_set(self->patterns, ptn_id))
//		return false;
//
//	del_branch = false;
//	branch = node_iter = self->root;
//	branch_prev = nullptr;
//
//	for (i = 0; i < fixed_string_size(pattern); ++i) {
//		child = node_iter->children;
//		child_prev = nullptr;
//
//		while (child != nullptr && child->ch != fixed_string_at(pattern, i)) {
//			child_prev = child;
//			child = child->sibling;
//		}
//
//		if (child == nullptr) {
//			// log_error
//			return false;
//		}
//
//		//	�ҵ���Ҫ��ʼɾ���ķ�֧�ڵ㡣�ýڵ������µģ�������������������֮һ�Ľڵ㣺
//		//		1	��һ��ģʽ��ĳ���м�ڵ㱻���㣨match_id != 0������������£�
//		//			��֧�ڵ�λ�������·�����������м�ڵ���ӽڵ�Ӧ���Ƴ���
//		//		2	��һ���ڵ��������ֵܽڵ㡣����������£��ڵ������Ƿ�֧�ڵ㣬��������
//		//			�ӽڵ���Ҫ���Ƴ�
//		if (i < fixed_string_size(pattern) - 1 && child->match_id != 0) {
//			del_branch = false;
//			branch = child;
//		}
//		else if (child_prev != nullptr || child->sibling != nullptr) {	//has sibling
//			del_branch = true;
//			branch = child;
//			branch_prev = (child_prev == nullptr ? node_iter : child_prev);
//		}
//		node_iter = child;
//	}
//	
//	//	������ַ������һ���ַ���Ӧ�Ľڵ����ӽڵ㣬�������ı�
//	if (node_iter->children != nullptr)
//		node_iter->match_id = 0;
//	else { //tnode->children == nullptr
//		if (del_branch) {
//			(branch_prev->children == branch) ? 
//			(branch_prev->children = branch->sibling) : 
//			(branch_prev->sibling = branch->sibling);
//		}
//		else {
//			child = branch->children;
//			branch->children = nullptr;
//			branch = child;
//		}
//
//		while (branch != nullptr) {
//			child = branch->children;
//			mem_free(branch);
//			branch = child;
//		}
//	}
//
//	self->pattern_num--;
//	bitset_clear(self->patterns, ptn_id);
//	self->is_prep = false;
//
//	return true;
//}
