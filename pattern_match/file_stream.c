#include "file_stream.h"
#include "util.h"
#include "log.h"

static inline file_stream_skip_lf(file_stream *self) {
	while (self->curr_index < self->size &&
		self->content[self->curr_index] == LF)
		self->curr_index++;
}

constructor(file_stream, const char *filename, size_t buf_size) {
	(void)buf_size;

	self->filename = util_cstr_copy(filename);

	constructor_end;
}

distructor(file_stream) {
	mem_free(self->filename);
	mem_free(self->content);
	mem_free(self);
}

bool file_stream_init(file_stream * self) {
	if (!util_read_entire_file(self->filename, "rb", &(self->content), &self->size))
		return false;

	log_notice("Loaded entire string file.");
	file_stream_reset(self);
	return true;
}

void file_stream_next(file_stream * self) {
	self->curr_index += 2;
	file_stream_skip_lf(self);
}

wchar file_stream_get(file_stream * self) {
	if (file_stream_getend(self))
		return '\0';
	return *(wchar*)(self->content + self->curr_index);
}

bool file_stream_getend(file_stream * self) {
	return self->curr_index >= self->size;
}

void file_stream_reset(file_stream * self) {
	self->curr_index = 0;
	file_stream_skip_lf(self);
}


