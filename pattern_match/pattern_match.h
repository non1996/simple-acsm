#pragma once

#include "afx.h"

class_decl(acsm);
class_decl(file_stream);
class_decl(pattern_set);

class(pattern_match) {
	acsm *ac;
	file_stream *fs;
	pattern_set *patterns;
	FILE *ac_output;
	char *ac_output_name;
	uint8_t mode;
};

constructor(pattern_match, int argc, char **argv);
distructor(pattern_match);

bool pattern_match_work(pattern_match *self);