#include "file_stream.h"
#include "util.h"
#include "log.h"
#include "mem_chunk.h"
#include "cstring.h"

//					buffer_index				rest
//						|				  |				
//	block |	... ... ...	_ ... ... ... ... | ... ... ... ...
//						|		retain    |
static inline bool file_stream_load_next(file_stream *self, size_t retain) {
	static size_t invoke_count = 1;

	assert(retain < self->buffer->cap);

	if (!self->is_valid || (self->file_offset >= self->file_size))
		return false;

	size_t rest, read, to_read;
	char *buffer = self->buffer->block;

	rest = (size_t)(self->file_size - self->file_offset - self->buffer->used);		//rest bytes in file not be loaded
	to_read = (rest >= self->buffer->cap) ? self->buffer->cap : rest;				

	self->file_offset += self->buffer_index;

	mem_chunk_clear(self->buffer);

	if (retain != 0) {
		memmove(buffer, buffer + self->buffer_index, retain);
		buffer += retain;
		if (to_read + retain > self->buffer->cap)
			to_read = self->buffer->cap - retain;
	}

	self->buffer_index = 0;

	if (!util_read_file(self->handle, buffer, to_read, &read))
		return false;

	assert(to_read == read);

	mem_chunk_use(self->buffer, read + retain);

	log_debug("The %dth, Load next file chunk, curr_file_offset: %d, curr_chunk_offset: %d.", invoke_count++, self->file_offset, self->buffer_index);

	return true;
}

static inline void file_stream_skip_lf(file_stream *self) {
	while (!file_stream_getend(self) && self->buffer->block[self->buffer_index] == LF) {
		self->buffer_index++;
		if (self->buffer_index >= self->buffer->used)
			file_stream_load_next(self, 0);
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
	file_stream_load_next(self, 0);

	return true;
}

void file_stream_close(file_stream * self) {
	file_stream_clear(self);
}

bool file_stream_isopen(file_stream * self) {
	return self->is_valid;
}

void file_stream_clear(file_stream *self) {
	if (!self->is_valid)
		return;
	if (self->handle)
		fclose(self->handle);
	mem_free(self->filename);
	mem_chunk_delete(self->buffer);
	self->file_size = 0;
	self->file_offset = 0;
	self->buffer_index = 0;
	self->is_valid = false;
}

bool file_stream_getline(file_stream * self, fixed_wstring *str) {
	assert(str);
	
	size_t lf_index = self->buffer_index;
	
	if (file_stream_getend(self))
		return false;

	while (self->buffer->block[lf_index] != LF) {
		lf_index++;

		if (self->file_offset + lf_index >= self->file_size)
			break;

		if (lf_index >= self->buffer->used) {
			lf_index -= self->buffer_index;
			file_stream_load_next(self, self->buffer->used - self->buffer_index);
		}

	}

	str->f.buffer = self->buffer->block + self->buffer_index;
	str->f.size = lf_index - self->buffer_index;
	return true;
}

void file_stream_next(file_stream * self) {
	if (!self->is_valid)
		return;
	util_wchar_next(self->buffer->block, &(self->buffer_index));
	if (self->buffer_index >= self->buffer->used) {
		file_stream_load_next(self, 0);
	}
	else if (self->buffer->used - self->buffer_index == 1) {
		file_stream_load_next(self, 1);
	}
	file_stream_skip_lf(self);
}

wchar file_stream_get(file_stream * self) {
	if (file_stream_getend(self))
		return 0x0000u;
	return util_wchar_get(self->buffer->block + self->buffer_index);
}

bool file_stream_getend(file_stream * self) {
	if (!self->is_valid || 
		(self->buffer_index + self->file_offset >= self->file_size))
		return true;
	return false;
}

void file_stream_reset(file_stream * self) {
	if (!self->is_valid)
		return;
	self->buffer_index = 0;
	self->file_offset = 0;
	fseek(self->handle, 0, SEEK_SET);
	file_stream_load_next(self, 0);
	file_stream_skip_lf(self);
}
