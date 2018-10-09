#ifndef CSTRING_H
#define CSTRING_H

#include "afx.h"
#include "util.h"

class(fixed_string) {
	char *buffer;
	size_t size;
	
};

class(fixed_wstring) {
	fixed_string f;
	size_t traver_index;
};

constructor(fixed_string, char *str);
distructor(fixed_string);

constructor(fixed_wstring, char *str);
distructor(fixed_wstring);

static inline char fixed_string_at(const fixed_string *self, size_t index) {
	return index >= self->size ? '\0' : self->buffer[index];
}

//static inline wchar fixed_wstring_at(const fixed_wstring *self, size_t index) {
//	return index >= self->f.size ? 0x0000u : *(wchar*)(self->f.buffer + (index << 1));
//}

static inline char *fixed_string_cstr(const fixed_string *self) {
	return self->buffer;
}

//static inline wchar *fixed_wstring_cstr(const fixed_wstring *self) {
//	return (wchar*)(self->f.buffer);
//}

static inline size_t fixed_string_size(const fixed_string *self) {
	return self->size;
}

static inline size_t fixed_wstring_size(const fixed_wstring *self) {
	return self->f.size;
}

static inline void fixed_wstring_begin(fixed_wstring *self) {
	self->traver_index = 0;
}

static inline bool fixed_wstring_getend(fixed_wstring *self) {
	return self->traver_index >= self->f.size;
}

static inline wchar fixed_wstring_get(fixed_wstring *self) {
	return util_wchar_get(self->f.buffer + self->traver_index);
}

static inline void fixed_wstring_next(fixed_wstring *self) {
	util_wchar_next(self->f.buffer, &(self->traver_index));
}

#endif // !CSTRING_H


