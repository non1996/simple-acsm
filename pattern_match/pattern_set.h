#pragma once

#include "afx.h"

class_decl(fixed_string);
class_decl(fixed_wstring);
class_decl(vector);
class_decl(pattern_set);

class(pattern_set) {
	char *pattern_file;
	fixed_string *text_patterns;
	vector *patterns;
	size_t iter;
};

constructor(pattern_set, const char *pattern_file);
distructor(pattern_set);

bool pattern_set_init(pattern_set *self);
fixed_wstring *pattern_set_get(pattern_set *self, size_t id);
void pattern_set_count(pattern_set *self, size_t id);
size_t pattern_set_size(pattern_set *self);
void pattern_set_sort(pattern_set *self);

void pattern_set_begin(pattern_set *self);
fixed_wstring *pattern_set_curr_pattern(pattern_set *self);
uint32_t pattern_set_curr_id(pattern_set *self);
size_t pattern_set_curr_count(pattern_set *self);
void pattern_set_next(pattern_set *self);
bool pattern_set_getend(pattern_set *self);
