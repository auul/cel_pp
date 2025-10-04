#include "str.h"
#include "rc.h"

#include <string.h>

struct cel_pp_str {
	size_t len;
	char data[];
};

size_t str_len(const cel_pp_str *str)
{
	return str->len;
}

const char *str_data(const cel_pp_str *str)
{
	return str->data;
}

static inline size_t str_size(size_t len)
{
	return offsetof(cel_pp_str, data) + len + 1;
}

cel_pp_str *str_create_n(const char *src, size_t len)
{
	cel_pp_str *str = rc_alloc(str_size(len));
	str->len = len;
	memcpy(str->data, src, len);
	str->data[len] = 0;

	return str;
}

cel_pp_str *str_create(const char *src)
{
	return str_create_n(src, strlen(src));
}

void str_unref(cel_pp_str *str)
{
	if (rc_unref(str)) {
		rc_free(str);
	}
}
