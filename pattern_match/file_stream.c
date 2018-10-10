#include "file_stream.h"
#include "util.h"
#include "log.h"
#include "mem_chunk.h"

#define MODE_COMPLETE 0
#define MODE_ONE_BYTE 1

static inline bool file_stream_read_next(file_stream *self, uint8_t mode) {
	if (!self->is_valid || (self->curr_file_offset >= self->file_size))
		return false;

	size_t read, to_read;
	char *buffer = self->buffer->block;

	log_debug("Load next file chunk, curr_file_offset: %d, curr_chunk_offset: %d.", self->curr_file_offset, self->curr_buffer_index);

	to_read = (self->file_size - self->curr_file_offset >= self->buffer->cap) ?
		self->buffer->cap : (size_t)(self->file_size - self->curr_file_offset);

	mem_chunk_clear(self->buffer);

	if (mode == MODE_ONE_BYTE) {
		to_read -= 1;
		buffer += 1;
		self->buffer->block[0] = self->buffer->block[self->curr_buffer_index];
	}
	self->curr_file_offset += to_read;

	if (!util_read_file(self->handle, buffer, to_read, &read))
		return false;

	assert(to_read == read);

	self->curr_buffer_index = 0;
	mem_chunk_use(self->buffer, read + mode);
	return true;
}

static inline file_stream_skip_lf(file_stream *self) {
	while (!file_stream_getend(self) && self->buffer->block[self->curr_buffer_index] == LF) {
		self->curr_buffer_index++;
		if (self->curr_buffer_index >= self->buffer->used)
			file_stream_read_next(self, MODE_COMPLETE);
	}
}

constructor(file_stream, const char *filename, size_t buf_size) {
	if (self->is_valid)
		file_stream_clear(self);
	self->filename = util_cstr_copy(filename);

	file_stream_open(self, buf_size);

	constructor_end;
}

distructor(file_stream) {
	file_stream_clear(self);
}

bool file_stream_open(file_stream * self, size_t buf_size) {
	if (self->is_valid)
		return true;

	if (!util_file_size(self->filename, &(self->file_size)))
		return false;
	self->buffer = mem_chunk_new(buf_size);
	self->handle = fopen(self->filename, "rb");
	self->is_valid = true;
	file_stream_read_next(self, MODE_COMPLETE);

	return true;
}

bool file_stream_isopen(file_stream * self) {
	return self->is_valid;
}

void file_stream_clear(file_stream *self) {
	if (self->handle)
		fclose(self->handle);
	mem_free(self->filename);
	mem_chunk_delete(self->buffer);
	self->file_size = 0;
	self->curr_file_offset = 0;
	self->curr_buffer_index = 0;
	self->is_valid = false;
}

void file_stream_next(file_stream * self) {
	if (!self->is_valid)
		return;
	util_wchar_next(self->buffer->block, &(self->curr_buffer_index));
	if (self->curr_buffer_index >= self->buffer->used) {
		file_stream_read_next(self, MODE_COMPLETE);
	}
	else if (self->buffer->used - self->curr_buffer_index == 1) {
		file_stream_read_next(self, MODE_ONE_BYTE);
	}
	file_stream_skip_lf(self);
}

wchar file_stream_get(file_stream * self) {
	if (!self->is_valid || file_stream_getend(self))
		return 0x0000u;
	return util_wchar_get(self->buffer->block + self->curr_buffer_index);
}

bool file_stream_getend(file_stream * self) {
	if (!self->is_valid)
		return true;
	if (self->curr_buffer_index + self->curr_file_offset >= self->file_size)
		return true;
	return false;
}

void file_stream_reset(file_stream * self) {
	if (!self->is_valid)
		return;
	self->curr_buffer_index = 0;
	self->curr_file_offset = 0;
	fseek(self->handle, 0, SEEK_SET);
	file_stream_read_next(self, MODE_COMPLETE);
	file_stream_skip_lf(self);
}


