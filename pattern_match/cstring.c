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
	fixed_string_constructor((fixed_string*)self, cstr);
	constructor_end;
}

distructor(fixed_wstring) {
	mem_free(self->f.buffer);
}
