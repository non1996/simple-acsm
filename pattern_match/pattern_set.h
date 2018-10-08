#pragma once

#include "afx.h"

class_decl(fixed_string);
class_decl(fixed_wstring);
class_decl(vector);
class_decl(pattern_set_iterator);
class_decl(pattern_set);

class(pattern_set) {
	char *pattern_file;
	
	fixed_string *text_patterns;
	
	vector *patterns;

	pattern_set_iterator *iter;
};

constructor(pattern_set, const char *pattern_file);
distructor(pattern_set);

bool pattern_set_init(pattern_set *self);
fixed_wstring *pattern_set_get(pattern_set *self, size_t id);
void pattern_set_inc(pattern_set *self, size_t id);
pattern_set_iterator *pattern_set_begin(pattern_set *self);
size_t pattern_set_size(pattern_set *self);
void pattern_set_sort(pattern_set *self);

void pattern_set_iterator_reset(pattern_set_iterator *self);
fixed_wstring *pattern_set_iterator_get_pattern(pattern_set_iterator *self);
uint32_t pattern_set_iterator_get_id(pattern_set_iterator *self);
size_t pattern_set_iterator_get_count(pattern_set_iterator *self);
void pattern_set_iterator_next(pattern_set_iterator *self);
bool pattern_set_iterator_getend(pattern_set_iterator *self);