#pragma once

#include "afx.h"

class_decl(fixed_wstring);
class_decl(mem_chunk);

class(file_stream) {
	FILE *handle;
	char *filename;
	mem_chunk *buffer;
	uint64_t file_size;
	uint64_t file_offset;
	size_t buffer_index;
	bool is_valid;
};

constructor(file_stream, const char *filename, size_t buf_size);
distructor(file_stream);

bool file_stream_open(file_stream * self, size_t buf_size);
void file_stream_close(file_stream *self);
bool file_stream_isopen(file_stream *self);
void file_stream_clear(file_stream *self);
bool file_stream_getline(file_stream * self, fixed_wstring *str);

void file_stream_next(file_stream *self);
wchar file_stream_get(file_stream *self);
bool file_stream_getend(file_stream *self);
void file_stream_reset(file_stream *self);
