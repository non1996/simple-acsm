#include "pattern_match.h"
#include "file_stream.h"
#include "pattern_set.h"
#include "ac.h"
#include "cstring.h"
#include "log.h"

#define MODE_AC		0
#define MODE_TRIE	1

constructor(pattern_match, int argc, char **argv) {
	self->ac = new(acsm);

	if (argc >= 2) {
		self->fs = new(file_stream, argv[1], 0);
	}
	else {
		self->fs = new(file_stream, "string.txt", 0);
	}

	if (argc >= 3) {
		self->patterns = new(pattern_set, argv[2]);
	}
	else {
		self->patterns = new(pattern_set, "pattern.txt");
	}

	self->ac_output_name = "ac_output.txt";
	self->mode = MODE_AC;

	constructor_end;
}

distructor(pattern_match) {
	delete(acsm, self->ac);
	delete(file_stream, self->fs);
	delete(pattern_set, self->patterns);
}

static void pattern_match_handle_cb(uint32_t pattern_id, void *arg) {
	pattern_match *self = (pattern_match*)arg;
	pattern_set_inc(self->patterns, pattern_id);
}

static inline bool pattern_match_output(pattern_match *self) {
	pattern_set_iterator *iter;
	
	if (!(self->ac_output = fopen(self->ac_output_name, "w"))) {
		return false;
	}

	pattern_set_sort(self->patterns);

	for (iter = pattern_set_begin(self->patterns);
		!pattern_set_iterator_getend(iter);
		pattern_set_iterator_next(iter)) {
		fixed_string *s = (fixed_string*)pattern_set_iterator_get_pattern(iter);
		size_t count = pattern_set_iterator_get_count(iter);
		fprintf(self->ac_output, "%s %d\n", fixed_string_cstr(s), count);
	}
	return true;
}

static inline bool pattern_match_work_ac(pattern_match *self) {
	pattern_set_iterator *iter;

	log_notice("Load pattern file.");
	if (!pattern_set_init(self->patterns))
		return false;

	log_notice("Add patterns to acsm.");
	for (iter = pattern_set_begin(self->patterns);
		!pattern_set_iterator_getend(iter);
		pattern_set_iterator_next(iter)) {

		acsm_add_pattern(self->ac, 
			pattern_set_iterator_get_pattern(iter), 
			pattern_set_iterator_get_id(iter));
	}

	log_notice("Prepare acsm.");
	acsm_prepare(self->ac);
	
	log_notice("Load string.");
	if (!file_stream_init(self->fs))
		return false;

	acsm_search_init_file(self->ac, self->fs, pattern_match_handle_cb, self);
	log_notice("Search.");
	acsm_search(self->ac);

	log_notice("Ouput result.");
	if (!pattern_match_output(self))
		return false;

	log_notice("Mission completed and Quit.");
	return true;
}

static inline bool pattern_match_work_trie(pattern_match *self) {
	assert(0);
	return true;
}

bool pattern_match_work(pattern_match * self) {
	if (self->mode == MODE_AC)
		return pattern_match_work_ac(self);
	else if (self->mode == MODE_TRIE)
		return pattern_match_work_trie(self);
	return false;
}
