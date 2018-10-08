#pragma once

#include "afx.h"

class(file_stream) {
	FILE *handle;
	char *filename;
	char *content;
	size_t size;
	size_t curr_index;
};

constructor(file_stream, const char *filename, size_t buf_size);
distructor(file_stream);

bool file_stream_init(file_stream *self);
void file_stream_next(file_stream *self);
wchar file_stream_get(file_stream *self);
bool file_stream_getend(file_stream *self);
void file_stream_reset(file_stream *self);
