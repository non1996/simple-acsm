#pragma once

#include "afx.h"

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

constructor(rbtree_node, uint8_t color) {
	self->color = color;
	constructor_end;
}

distructor(rbtree_node) {

}

constructor(rbtree, rbtree_node *nil) {
	self->root = nil;
	constructor_end;
}

distructor(rbtree) {

}

#define rb_parent(node) node->parent
#define rb_left(node) node->left
#define rb_right(node) node->right
#define rb_grand(node) node->parent->parent
#define rb_right_uncle(node) rb_grand(node)->right
#define rb_left_uncle(node) rb_grand(node)->left

static inline void rbtree_node_output(rbtree_node *self) {
	printf("s:%p[%d], p:%p[%d], l:%p[%d], r:%p[%d], c:%s\n", 
		self, self->key, self->parent, self->parent->key,
		self->left, self->left->key, self->right, self->right->key,
		(self->color ? "BLACK" : "RED"));
}

static inline void rbtree_output(rbtree_node *root, rbtree_node *nil) {
	if (root == nil)
		return;
	rbtree_output(root->left, nil);
	rbtree_node_output(root);
	rbtree_output(root->right, nil);
}

static inline void rbtree_left_rotate(rbtree *self, rbtree_node *curr, rbtree_node *nil) {
	rbtree_node *curr_right = curr->right;
	assert(curr_right != nil);

	curr->right = curr_right->left;
	if (curr_right->left != nil)//
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
	assert(curr_left != nil);

	curr->left = curr_left->right;
	if (curr_left->right != nil)
		curr_left->right->parent = curr;
	curr_left->parent = curr->parent;
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
				continue;
			}
			if (start == rb_parent(start)->right) {
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
				continue;
			}
			if (start == rb_parent(start)->left) {
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
	//node->left = node->right = nil;
	//rbtree_output(self->root, nil);
	//printf("->\n");
	rbtree_fix(self, node, nil);
	//rbtree_output(self->root, nil);
	//printf("\n\n");
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
