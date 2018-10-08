#pragma once

#include "afx.h"

bool util_read_entire_file(const char *path, const char *mode, char **content, uint32_t *len);

char *util_cstr_copy(const char *src);