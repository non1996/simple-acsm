#include "util.h"
#include "log.h"
#include <sys/stat.h>
#include <sys/timeb.h>
#include <sys/types.h>

bool util_read_entire_file(const char *path, const char *mode, char **content, uint32_t *len) {
	FILE *file;
	char *cont;
	size_t to_read, read;

	if (!(file = fopen(path, mode))) {
		log_warm("Could not open file %s.", path);
		return false;
	}

	fseek(file, 0, SEEK_END);
	to_read = ftell(file);
	rewind(file);

	cont = mem_alloc_zero(char, to_read + 1);
	read = fread(cont, 1, to_read, file);

	if (read == 0 || to_read != read) {
		log_warm("Read error.");
		mem_free(cont);
		fclose(file);
		return false;
	}

	cont[to_read] = '\0';
	*content = cont;
	*len = to_read;
	fclose(file);
	return true;
}

bool util_read_file(FILE *handle, char *buffer, size_t to_read, size_t *read) {
	if (!handle)
		return false;

	*read = fread(buffer, sizeof(char), to_read, handle);

	return true;
}

bool util_file_size(const char * path, uint64_t * size) {
	FILE *file;

	if (!(file = fopen(path, "rb"))) {
		log_warm("Could not open file %s.", path);
		return false;
	}

	_fseeki64(file, 0, SEEK_END);
	*size = _ftelli64(file);
	fclose(file);
	return true;
}

char * util_cstr_copy(const char * src) {
	if (!src)
		return nullptr;

	char *temp = mem_alloc(char, strlen(src) + 1);
	strcpy(temp, src);
	return temp;
}

uint64_t get_timestamp() {
	struct timeb t;
	ftime(&t);
	return 1000 * t.time + t.millitm;
}
