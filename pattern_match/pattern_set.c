#include "pattern_set.h"
#include "cstring.h"
#include "vector.h"
#include "log.h"
#include "util.h"

#define id_to_idx(id) (id - 1)
#define idx_to_id(idx) (idx + 1)

class(pattern_node) {
	wstring_stream ptn;
	size_t count;
};

static inline constructor(pattern_node, char *ptn) {
	wstring_stream_constructor(&(self->ptn), ptn);
	constructor_end;
}

static inline distructor(pattern_node) {
	
}

static inline void pattern_set_pretreat(pattern_set *self) {
	size_t end, start;
	char *patterns = fixed_string_cstr(self->text_patterns);

	if (fixed_string_size(self->text_patterns) == 0)
		return;

	self->patterns = new(vector);

	log_info("Split pattern file to patterns.");
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

	log_info("Total %lu patterns.", vector_size(self->patterns));
}

constructor(pattern_set, const char *pattern_file) {
	self->pattern_file = util_cstr_copy(pattern_file);
	constructor_end;
}
distructor(pattern_set) {
	mem_free(self->pattern_file);
	delete(fixed_string, self->text_patterns);
	delete(vector, self->patterns);
}

bool pattern_set_init(pattern_set * self) {
	char *content;
	size_t len;

	log_notice("Pattern_set init, start loading pattern file %s.", self->pattern_file);
	if (!util_read_entire_file(self->pattern_file, "rb", &content, &len))
		return false;
	self->text_patterns = new(fixed_string, content);
	log_info("Loaded entire pattern file.");
	pattern_set_pretreat(self);

	log_notice("Pattern_set inited.");
	return true;
}

bool pattern_set_init_wchar(pattern_set * self) {
	return false;
}

wstring_stream * pattern_set_get(pattern_set * self, size_t id) {
	assert(id <= vector_size(self->patterns) && id > 0);	
	return &(((pattern_node*)vector_at(self->patterns, id_to_idx(id)))->ptn);
}

void pattern_set_count(pattern_set * self, size_t id) {
	assert(id <= vector_size(self->patterns) && id > 0);

	pattern_node *node = (pattern_node*)vector_at(self->patterns, id_to_idx(id));
	node->count++;
}

void pattern_set_begin(pattern_set * self) {
	self->iter = 0;
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

wstring_stream * pattern_set_curr_pattern(pattern_set * self) {
	return &(((pattern_node*)vector_at(self->patterns, self->iter))->ptn);
}

uint32_t pattern_set_curr_id(pattern_set *self) {
	return idx_to_id(self->iter);
}

size_t pattern_set_curr_count(pattern_set *self) {
	return ((pattern_node*)vector_at(self->patterns, self->iter))->count;
}

void pattern_set_next(pattern_set *self) {
	self->iter++;
}

bool pattern_set_getend(pattern_set *self) {
	return self->iter >= vector_size(self->patterns);
}

