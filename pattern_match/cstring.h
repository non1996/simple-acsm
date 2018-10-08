#ifndef CSTRING_H
#define CSTRING_H

#include "afx.h"

class(fixed_string) {
	char *buffer;
	size_t size;
};

class(fixed_wstring) {
	fixed_string f;
};

constructor(fixed_string, char *str);
distructor(fixed_string);

constructor(fixed_wstring, char *str);
distructor(fixed_wstring);

static inline char fixed_string_at(const fixed_string *self, size_t index) {
	return index >= self->size ? '\0' : self->buffer[index];
}

static inline wchar fixed_wstring_at(const fixed_wstring *self, size_t index) {
	return index >= self->f.size ? 0x0000u : *(wchar*)(self->f.buffer + (index << 1));
}

static inline char *fixed_string_cstr(const fixed_string *self) {
	return self->buffer;
}

static inline wchar *fixed_wstring_cstr(const fixed_wstring *self) {
	return (wchar*)(self->f.buffer);
}

static inline size_t fixed_string_size(const fixed_string *self) {
	return self->size;
}

static inline size_t fixed_wstring_size(const fixed_wstring *self) {
	return self->f.size;
}

#endif // !CSTRING_H


