#pragma once

#include "afx.h"

bool util_read_entire_file(const char *path, const char *mode, char **content, uint32_t *len);

char *util_cstr_copy(const char *src);

#define GB_MASK ((wchar)0xA0)

static inline bool util_wchar_is_GB2312(char *ch) {
	return (*ch) & GB_MASK;
}

static inline wchar util_ascii_to_wchar(char *ch) {
	return (wchar)(*ch);
}

static inline wchar util_wchar_get(char *ch) {
	if (util_wchar_is_GB2312(ch))
		return *((wchar*)ch);
	else
		return util_ascii_to_wchar(ch);
}

static inline void util_wchar_next(char *ch, size_t *index) {
	if (util_wchar_is_GB2312(ch + (*index)))
		*index += 2;
	else 
		*index += 1;
}
