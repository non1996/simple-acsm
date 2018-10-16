#pragma once

#include "afx.h"
#include "allocator.h"

class_decl(file_stream);
class_decl(fixed_wstring);
class_decl(ac_node);
class_decl(hashmap);
ALLOCATOR_DECL(ac_node);
typedef ac_node *p_ac_node;

typedef void(*match_handle)(uint32_t pattern_id, void *arg);

class(acsm){
	p_ac_node root;								//	root of trie tree

	size_t pattern_num;							//	pattern added to the trie tree

	union {										//	string to be search, use file_stream
		fixed_wstring *text;					//	or string as input.
		file_stream *fs;
	};

	p_ac_node curr_node;						//	searching iterator

	match_handle cb;							//	if a pattern match, call this function to handle
	void *cb_arg;								//	cb's argument	

	hashmap *nodes;								//	hashmap to contain trie tree nodes

	allocator(ac_node) *ac_alloc;				//	trie tree node allocator

	uint8_t input_type;							//	use string or file_stream as input
};

constructor(acsm);
distructor(acsm);
bool acsm_add_pattern(acsm *self, fixed_wstring *pattern, uint32_t ptn_id);
//bool acsm_del_pattern(acsm * self, fixed_string *pattern, uint32_t ptn_id);
void acsm_compile(acsm *self);
void acsm_search_init(acsm * self, match_handle cb, void * cb_arg);
bool acsm_search_init_file(acsm *self, file_stream *text, match_handle cb, void *cb_arg);
bool acsm_search_init_string(acsm *self, fixed_wstring *text, match_handle cb, void *cb_arg);
bool acsm_search_ac(acsm *self);
bool acsm_search_trie(acsm *self, fixed_wstring *token);