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
		if (strcmp("trie", argv[1]) == 0)
			self->work = pattern_match_work_trie;
		else
			self->work = pattern_match_work_ac;
	else
		self->work = pattern_match_work_ac;
		
	if (argc >= 3)
		self->text_name = util_cstr_copy(argv[2]);
	else 
		self->text_name = util_cstr_copy("string.txt");
	
	if (argc >= 4)
		self->pattern_name = util_cstr_copy(argv[3]);
	else 
		self->pattern_name = util_cstr_copy("pattern.txt");

	if (argc >= 5) 
		self->output_name = util_cstr_copy(argv[4]);
	else 
		self->output_name = util_cstr_copy("ac_output.txt");

	self->patterns = new(pattern_set, self->pattern_name);
	
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
	pattern_set_count(self->patterns, pattern_id);
}

static inline bool pattern_match_output(pattern_match *self) {
	if (!(self->ac_output = fopen(self->output_name, "w"))) {
		log_warm("Couldn't open output file.");
		return false;
	}
	
	pattern_set_sort(self->patterns);

	log_notice("Ouput result.");
	for (pattern_set_begin(self->patterns);
		!pattern_set_getend(self->patterns);
		pattern_set_next(self->patterns)) {

		fixed_string *s = (fixed_string*)pattern_set_curr_pattern(self->patterns);
		size_t count = pattern_set_curr_count(self->patterns);
		fprintf(self->ac_output, "%s %d\n", fixed_string_cstr(s), count);
	}
	fclose(self->ac_output);
	return true;
}

static bool pattern_match_work_ac(pattern_match *self) {
	if (!pattern_set_init(self->patterns))
		return false;

	log_notice("Add patterns to acsm.");
	for (pattern_set_begin(self->patterns);
		!pattern_set_getend(self->patterns);
		pattern_set_next(self->patterns)) {

		acsm_add_pattern(self->ac, 
			pattern_set_curr_pattern(self->patterns),
			pattern_set_curr_id(self->patterns));
	}

	acsm_compile(self->ac);
	
	self->fs = new(file_stream, self->text_name, 500 * 1024 * 1024);

	acsm_search_init_file(self->ac, self->fs, pattern_match_handle_cb, self);

	acsm_search_ac(self->ac);

	if (!pattern_match_output(self))
		return false;

	log_notice("Mission completed and Quit.");
	return true;
}

static bool pattern_match_work_trie(pattern_match *self) {
	if (!pattern_set_init(self->patterns))
		return false;

	log_notice("Add patterns to acsm.");
	for (pattern_set_begin(self->patterns);
		!pattern_set_getend(self->patterns);
		pattern_set_next(self->patterns)) {

		acsm_add_pattern(self->ac,
			pattern_set_curr_pattern(self->patterns),
			pattern_set_curr_id(self->patterns));
	}

	self->fs = new(file_stream, "fenci_result.txt", 500 * 1024 * 1024);

	acsm_search_init(self->ac, pattern_match_handle_cb, self);

	wstring_stream *line = new(wstring_stream, "");

	log_notice("Search.");
	while (file_stream_getline(self->fs, line)) {
		acsm_search_trie(self->ac, line);
	}
	
	if (!pattern_match_output(self))
		return false;

	log_notice("Mission completed and Quit.");
	return true;
}

bool pattern_match_work(pattern_match * self) {
	return self->work(self);
}
