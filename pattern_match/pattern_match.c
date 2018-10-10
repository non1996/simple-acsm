#include "pattern_match.h"
#include "file_stream.h"
#include "pattern_set.h"
#include "ac.h"
#include "cstring.h"
#include "log.h"

#define MODE_AC		0
#define MODE_TRIE	1

static bool pattern_match_work_ac(pattern_match *self);
static bool pattern_match_work_trie(pattern_match *self);

constructor(pattern_match, int argc, char **argv) {
	self->ac = new(acsm);

	if (argc >= 2)
		self->text_name = util_cstr_copy(argv[1]);
	else 
		self->text_name = "string.txt";
	
	if (argc >= 3)
		self->pattern_name = util_cstr_copy(argv[2]);
	else 
		self->pattern_name = "pattern.txt";

	self->patterns = new(pattern_set, self->pattern_name);

	if (argc >= 4) 
		self->output_name = util_cstr_copy(argv[4]);
	else 
		self->output_name = "ac_output.txt";

	self->work = pattern_match_work_ac;

	constructor_end;
}

distructor(pattern_match) {
	delete(acsm, self->ac);
	delete(file_stream, self->fs);
	delete(pattern_set, self->patterns);
	mem_free(self->text_name);
	mem_free(self->pattern_name);
	mem_free(self->output_name);
}

static void pattern_match_handle_cb(uint32_t pattern_id, void *arg) {
	pattern_match *self = (pattern_match*)arg;
	pattern_set_inc(self->patterns, pattern_id);
}

static inline bool pattern_match_output(pattern_match *self) {
	pattern_set_iterator *iter;
	
	if (!(self->ac_output = fopen(self->output_name, "w"))) {
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
	fclose(self->ac_output);
	return true;
}

static bool pattern_match_work_ac(pattern_match *self) {
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
	self->fs = new(file_stream, self->text_name, 500 * 1024 * 1024);

	acsm_search_init_file(self->ac, self->fs, pattern_match_handle_cb, self);
	log_notice("Search.");
	acsm_search(self->ac);

	log_notice("Ouput result.");
	if (!pattern_match_output(self))
		return false;

	log_notice("Mission completed and Quit.");
	return true;
}

static bool pattern_match_work_trie(pattern_match *self) {
	assert(0);
	return true;
}

bool pattern_match_work(pattern_match * self) {
	return self->work(self);
}
