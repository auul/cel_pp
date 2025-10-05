#ifndef CEL_PP_STR_H
#define CEL_PP_STR_H

#include <stddef.h>

typedef struct cel_pp_str cel_pp_str;

size_t str_len(const cel_pp_str *str);
const char *str_data(const cel_pp_str *str);
cel_pp_str *str_create_n(const char *src, size_t len);
cel_pp_str *str_create(const char *src);
void str_unref(cel_pp_str *str);
void str_print(const cel_pp_str *str);

#endif
