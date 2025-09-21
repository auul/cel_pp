#include "file.h"
#include "dict.h"

#include <stdio.h>
#include <string.h>

static cel_pp_dict *file_dict;

static inline cel_char get_trigraph_value(const cel_char *src)
{
	if (src[0] != '?' || src[1] != '?') {
		return 0;
	}

	switch (src[2]) {
	case '=':
		return '#';
	case '(':
		return '[';
	case ')':
		return ']';
	case '/':
		return '\\';
	case '\'':
		return '^';
	case '<':
		return '{';
	case '>':
		return '}';
	case '!':
		return '|';
	case '-':
		return '~';
	default:
		return 0;
	}
}

static inline size_t
replace_trigraph(cel_char *src, size_t len, size_t i, cel_char value)
{
	src[i] = value;
	memmove(src + i + 1, src + i + 3, len - i - 2);
	return len - 2;
}

static cel_char *remove_trigraphs(cel_char *src, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		cel_char value = get_trigraph_value(src + i);
		if (value) {
			len = replace_trigraph(src, len, i, value);
		}
	}
	return src;
}

cel_char *cel_pp_file_load(const cel_char *filename)
{
	FILE *f = fopen((const char *)filename, "rb");
	if (!f) {
		return NULL;
	}

	size_t filename_len = strlen((const char *)filename);
	cel_pp_dict *node = cel_pp_dict_insert(&file_dict, filename, filename_len);
	if (!node) {
		fclose(f);
		return NULL;
	} else if (node->value) {
		fclose(f);
		return node->value;
	} else if (fseek(f, 0, SEEK_END) != 0) {
		fclose(f);
		return NULL;
	}

	long flen = ftell(f);
	if (flen < 0 || fseek(f, 0, SEEK_SET) != 0) {
		fclose(f);
		return NULL;
	}
	size_t src_len = (size_t)flen;

	cel_char *src = malloc(src_len + 1);
	if (!src) {
		fclose(f);
		return NULL;
	}

	size_t bytes = fread(src, 1, src_len, f);
	fclose(f);

	if (bytes != src_len) {
		free(src);
		return NULL;
	}
	src[src_len] = 0;
	node->value = remove_trigraphs(src, src_len);

	return src;
}
