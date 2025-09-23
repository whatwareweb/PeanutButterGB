#include <stdint.h>
#include <stdlib.h>

#include "fileutils.h"


long get_file_size(FILE *f) {
	fseek(f, 0L, SEEK_END);
	long file_size = ftell(f);
	rewind(f);
	return file_size;
}

uint8_t * load_file(const char *filename) {
	FILE *f;

	f = fopen(filename, "rb");

	if (f == NULL) {
		return NULL;
	}

	long file_size = get_file_size(f);

	uint8_t *dest;

	if ((dest = malloc(file_size + 1)) == NULL) {
		fprintf(stderr, "error: could not load save file\n");
		fclose(f);
		return NULL;
	}

	if (fread(dest, sizeof(uint8_t), file_size, f) != file_size) {
		free(dest);
		fclose(f);
		return NULL;
	}


	fclose(f);

	dest[file_size] = '\0'; // null-terminated

	return dest;
}