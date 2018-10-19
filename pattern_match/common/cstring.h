#ifndef CSTRING_H
#define CSTRING_H

#include "afx.h"
#include "util.h"

class(fixed_string) {
	char *buffer;
	size_t size;
	
};

class(wstring_stream) {
	fixed_string f;
	size_t traver_index;
};

constructor(fixed_string, char *str);
distructor(fixed_string);

constructor(wstring_stream, char *str);
distructor(wstring_stream);

static inline char fixed_string_at(const fixed_string *self, size_t index) {
	return index >= self->size ? '\0' : self->buffer[index];
}

static inline char *fixed_string_cstr(const fixed_string *self) {
	return self->buffer;
}

static inline size_t fixed_string_size(const fixed_string *self) {
	return self->size;
}

static inline size_t wstring_stream_size(const wstring_stream *self) {
	return self->f.size;
}

static inline void wstring_stream_begin(wstring_stream *self) {
	self->traver_index = 0;
}

static inline bool wstring_stream_getend(wstring_stream *self) {
	return self->traver_index >= self->f.size;
}

static inline wchar wstring_stream_get(wstring_stream *self) {
	if (wstring_stream_getend(self))
		return 0x0000u;
	return util_wchar_get(self->f.buffer + self->traver_index);
}

static inline void wstring_stream_next(wstring_stream *self) {
	util_wchar_next(self->f.buffer, &(self->traver_index));
}

#endif // !CSTRING_H


