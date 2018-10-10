#include "pattern_set.h"
#include "cstring.h"
#include "vector.h"
#include "log.h"
#include "util.h"

#define id_to_idx(id) (id - 1)
#define idx_to_id(idx) (idx + 1)

class(pattern_node) {
	fixed_wstring ptn;
	size_t count;
};

class(pattern_set_iterator) {
	pattern_set *ps;
	size_t idx;
};

static inline constructor(pattern_node, char *ptn) {
	fixed_wstring_constructor(&(self->ptn), ptn);
	constructor_end;
}

static inline distructor(pattern_node) {
	
}

static inline constructor(pattern_set_iterator, pattern_set *ps) {
	self->ps = ps;
	self->idx = 0;
	constructor_end;
}

static inline distructor(pattern_set_iterator) {

}

static inline void pattern_set_pretreat(pattern_set *self) {
	size_t end, start;
	char *patterns = fixed_string_cstr(self->text_patterns);

	if (fixed_string_size(self->text_patterns) == 0)
		return;

	self->patterns = new(vector);

	for (start = end = 0; patterns[end] != '\0'; ++end) {
		if (patterns[end] != LF)
			continue;
		
		patterns[end] = '\0';
			
		if (start == end) {
			log_debug("empty line");
			start = end + 1;
			continue;
		}
		vector_push_back(self->patterns, new(pattern_node, patterns + start));
		start = end + 1;
	}
	if (start != end)
		vector_push_back(self->patterns, new(pattern_node, patterns + start));

	log_debug("%s", ((pattern_node*)vector_back(self->patterns))->ptn.f.buffer);
}

constructor(pattern_set, const char *pattern_file) {
	self->pattern_file = util_cstr_copy(pattern_file);
	self->iter = new(pattern_set_iterator, self);
	constructor_end;
}
distructor(pattern_set) {
	mem_free(self->pattern_file);
	delete(fixed_string, self->text_patterns);
	delete(vector, self->patterns);
	delete(pattern_set_iterator, self->iter);
}

bool pattern_set_init(pattern_set * self) {
	char *content;
	size_t len;

	if (!util_read_entire_file(self->pattern_file, "rb", &content, &len))
		return false;

	content[len] = '\0';
	self->text_patterns = new(fixed_string, content);
	pattern_set_pretreat(self);
	log_notice("Loaded entire pattern file.");
	return true;
}

bool pattern_set_init_wchar(pattern_set * self) {
	return false;
}

fixed_wstring * pattern_set_get(pattern_set * self, size_t id) {
	assert(id <= vector_size(self->patterns) && id > 0);	
	return &(((pattern_node*)vector_at(self->patterns, id_to_idx(id)))->ptn);
}

void pattern_set_inc(pattern_set * self, size_t id) {
	assert(id <= vector_size(self->patterns) && id > 0);

	pattern_node *node = (pattern_node*)vector_at(self->patterns, id_to_idx(id));
	node->count++;
}

pattern_set_iterator * pattern_set_begin(pattern_set * self) {
	self->iter->idx = 0;
	return self->iter;
}

size_t pattern_set_size(pattern_set * self) {
	return vector_size(self->patterns);
}

static int pattern_set_cmp(const void *p1, const void *p2) {
	pattern_node *pn1 = *((pattern_node**)p1);
	pattern_node *pn2 = *((pattern_node**)p2);
	return pn2->count - pn1->count;
}

void pattern_set_sort(pattern_set * self) {
	vector_sort(self->patterns, pattern_set_cmp);
}

void pattern_set_iterator_reset(pattern_set_iterator * self) {
	self->idx = 0;
}

fixed_wstring * pattern_set_iterator_get_pattern(pattern_set_iterator * self) {
	return &(((pattern_node*)vector_at(self->ps->patterns, self->idx))->ptn);
}

uint32_t pattern_set_iterator_get_id(pattern_set_iterator * self) {
	return idx_to_id(self->idx);
}

size_t pattern_set_iterator_get_count(pattern_set_iterator * self) {
	return ((pattern_node*)vector_at(self->ps->patterns, self->idx))->count;
}

void pattern_set_iterator_next(pattern_set_iterator * self) {
	self->idx++;
}

bool pattern_set_iterator_getend(pattern_set_iterator * self) {
	return self->idx >= vector_size(self->ps->patterns);
}

