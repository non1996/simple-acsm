#include "ac.h"
#include "cstring.h"
#include "queue.h"
#include "bitset.h"
#include "log.h"
#include "file_stream.h"

#define USE_FILE_STREAM 0
#define USE_STRING		1

#define BR_COLOR_RED		0
#define BR_COLOR_BLACK		1

class(rbtree_node) {
	rbtree_node *parent, *left, *right;
	uint8_t color;
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

class(ac_node) {
	p_ac_node faillink;
	p_ac_node children, parent, next, prev;
	uint32_t match_id;
	wchar ch;
	char padding[2];
};

constructor(acsm) {
	self->root = mem_alloc_zero(ac_node, 1);
	self->root->faillink = self->root;
	self->patterns = new(bitset, 2260000);

	constructor_end;
}

distructor(acsm) {
	p_ac_node front, back, next;

	if (!self || !self->root)
		return;

	front = back = self->root;
	while (front != nullptr) {
		back->next = front->children;
		while (back->next != nullptr)
			back = back->next;

		next = front->next;
		mem_free(front);
		front = next;
	}

	delete(bitset, self->patterns);
}

static inline p_ac_node acsm_find_child(p_ac_node child, wchar ch) {
	while (child != nullptr) {
		if (child->ch == ch)
			return child;
		child = child->next;
	}
	return nullptr;
}

static inline bool acsm_find_insert(acsm *self, fixed_wstring *pattern, uint32_t ptn_id, 
									p_ac_node *parent, uint32_t *rest) {
	p_ac_node node_iter = self->root, child_iter;
	size_t pattern_length = fixed_wstring_size(pattern);
	uint32_t index;

	if (pattern_length == 0)
		return false;

	for (index = 0; index < pattern_length; ++index) {
		child_iter = acsm_find_child(node_iter->children, fixed_wstring_at(pattern, index));

		//	rest nodes should be inserted as *parent's child
		if (!child_iter) {
			*parent = node_iter;
			*rest = index;
			return true;
		}

		node_iter = child_iter;
	}

	child_iter->match_id = ptn_id;
	return false;
}

static inline ac_node *acsm_construct_subtree(fixed_wstring *pattern, size_t start, uint32_t pattern_id) {
	p_ac_node new_node, list = nullptr, tail;
	size_t pattern_length = fixed_wstring_size(pattern);
	uint32_t index;

	new_node = mem_alloc_zero(ac_node, 1);
	new_node->ch = fixed_wstring_at(pattern, start);
	list = tail = new_node;

	for (index = start + 1; index < pattern_length; ++index) {
		new_node = mem_alloc_zero(ac_node, 1);
		new_node->ch = fixed_wstring_at(pattern, index);
		tail->children = new_node;
		new_node->parent = tail;
		tail = tail->children;
	}

	tail->match_id = pattern_id;
	return list;
}

bool acsm_add_pattern(acsm * self, fixed_wstring *pattern, uint32_t ptn_id) {
	int start_index;
	p_ac_node insert, branch_list;

	if (bitset_is_set(self->patterns, ptn_id)) {
		return false;
	}

	if (acsm_find_insert(self, pattern, ptn_id, &insert, &start_index)) {

		branch_list = acsm_construct_subtree(pattern, start_index, ptn_id);
		
		branch_list->parent = insert;
		branch_list->next = insert->children;
		insert->children = branch_list;
	}

	bitset_set(self->patterns, ptn_id);
	self->pattern_num++;
	self->is_prep = false;

	return true;
}

static inline void acsm_children_inqueue(queue *q, p_ac_node child) {
	for (; child != nullptr; child = child->next)
		queue_push(q, child);
}

static inline void acsm_find_fail(acsm *self, p_ac_node node) {
	p_ac_node fail_try = node->parent->faillink;

	if (node->parent == self->root) {
		node->faillink = self->root;
		return;
	}

	while (true) {
		node->faillink = acsm_find_child(fail_try->children, node->ch);
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
	
	acsm_children_inqueue(wsqueue, self->root->children);

	while ((curr = queue_pop(wsqueue)) != nullptr) {
		acsm_children_inqueue(wsqueue, curr->children);
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
		child_iter = acsm_find_child(self->curr_node->children, acsm_curr_key(self));
		
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

#ifdef TEST
void acsm_print_tree(acsm * self) {
	queue *q = new(queue);
	p_ac_node temp;
	queue_push(q, self->root);
	while ((temp = queue_pop(q)) != nullptr) {
		acsm_children_inqueue(q, temp->children);

		printf("Node [%p]: match_id[%d], faillink[%p], children[%p], sibling[%p], parent[%p], charactor[%c]\n",
				temp, temp->match_id, temp->faillink, temp->children, temp->next, temp->parent, temp->ch);
	}
	delete(queue, q);
}
#endif

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
//		//	找到需要开始删除的分支节点。该节点是最新的，且满足下列两个条件之一的节点：
//		//		1	有一个模式在某个中间节点被满足（match_id != 0）。这种情况下，
//		//			分支节点位于它的下方，所以这个中间节点的子节点应被移除。
//		//		2	有一个节点有其他兄弟节点。在这种情况下，节点自身是分支节点，它和它的
//		//			子节点需要被移除
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
//	//	如果该字符串最后一个字符对应的节点有子节点，树不被改变
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
