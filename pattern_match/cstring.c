#include "cstring.h"

constructor(fixed_string, char *cstr) {
	if (cstr) {
		self->buffer = cstr;
		self->size = strlen(cstr);
	}
	constructor_end;
}

distructor(fixed_string) {
	mem_free(self->buffer);
}

constructor(fixed_wstring, char *cstr) {
	if (cstr) {
		self->f.buffer = cstr;
		self->f.size = strlen(cstr) >> 1;
	}
	constructor_end;
}

distructor(fixed_wstring) {
	mem_free(self->f.buffer);
}
