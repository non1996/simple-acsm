#pragma once

#include "afx.h"
#include "allocator.h"

class_decl(file_stream);
class_decl(fixed_string);
class_decl(fixed_wstring);
class_decl(ac_node);
class_decl(acsm);
class_decl(bitset);
class_decl(rbtree_node);
ALLOCATOR_DECL(ac_node);
typedef ac_node *p_ac_node;

typedef void(*output_handle)(uint32_t pattern_id, void *arg);

class(acsm){
	p_ac_node root;

	size_t pattern_num;
	bitset *patterns;

	union {
		fixed_wstring *text;
		file_stream *fs;
	};

	size_t curr_index;
	p_ac_node curr_node;

	output_handle cb;
	void *cb_arg;

	rbtree_node *nil;

	allocator(ac_node) *ac_alloc;

	bool is_init, is_end, is_prep;
	uint8_t search_type;
};

constructor(acsm);
distructor(acsm);
bool acsm_add_pattern(acsm *self, fixed_wstring *pattern, uint32_t ptn_id);
//bool acsm_del_pattern(acsm * self, fixed_string *pattern, uint32_t ptn_id);
void acsm_prepare(acsm *self);
bool acsm_search_init_file(acsm *self, file_stream *text, output_handle cb, void *cb_arg);
bool acsm_search_init_string(acsm *self, fixed_wstring *text, output_handle cb, void *cb_arg);
bool acsm_search(acsm *self);