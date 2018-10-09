#include "util.h"
#include "log.h"

bool util_read_entire_file(const char *path, const char *mode, char **content, uint32_t *len) {
	FILE *file;
	char *cont;
	size_t read;

	if (!(file = fopen(path, mode))) {
		log_warm("Could not open file %s.", path);
		return false;
	}

	fseek(file, 0, SEEK_END);
	read = ftell(file);
	rewind(file);

	read >>= 2;

	cont = mem_alloc_zero(char, read + 1);
	read = fread(cont, sizeof(char), read, file);

	if (read == 0) {
		log_warm("Read error, did not read anything.");
		mem_free(cont);
		return false;
	}

	*content = cont;
	*len = read;
	return true;
}

char * util_cstr_copy(const char * src) {
	if (!src)
		return nullptr;

	char *temp = mem_alloc(char, strlen(src) + 1);
	strcpy(temp, src);
	return temp;
}
