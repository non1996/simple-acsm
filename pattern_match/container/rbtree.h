#pragma once

#include "afx.h"



static inline void rbtree_left_rotate(rbtree *self, rbtree_node *curr) {
	rbtree_node *curr_right = curr->right;
	curr->right = curr_right->left;
	if (curr_right != self->nil)
		curr_right->left->parent = curr;
	curr_right->parent = curr->parent;
	if (curr->parent == self->nil)
		self->root = curr_right;
	else if (curr == curr->parent->left)
		curr->parent->left = curr_right;
	else
		curr->parent->right = curr_right;

	curr_right->left = curr;
	curr->parent = curr_right;
}

static inline void rbtree_right_rotate(rbtree *self, rbtree_node *curr) {
	rbtree_node *curr_left = curr->left;
	curr->left = curr_left->right;
	if (curr_left->right != self->nil)
		curr_left->right->parent = curr;
	if (curr->parent == self->nil)
		self->root = curr_left;
	else if (curr == curr->parent->left)
		curr->parent->left = curr_left;
	else
		curr->parent->right = curr_left;

	curr_left->right = curr;
	curr->parent = curr_left;
}

static inline void rbtree_fix(rbtree *self, rbtree_node *start) {

}

static inline void rbtree_insert(rbtree *self, rbtree_node *node) {

}

