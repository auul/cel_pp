#ifndef CEL_PP_DICT_H
#define CEL_PP_DICT_H

#include "str.h"

#include <stddef.h>

typedef struct cel_pp_dict cel_pp_dict;

cel_pp_str *dict_key(const cel_pp_dict *dict);
void *dict_value(const cel_pp_dict *dict);
void dict_unref(cel_pp_dict *dict);
const cel_pp_dict *
dict_lookup(const cel_pp_dict *dict, const char *key, size_t key_len);
cel_pp_dict *
dict_define(cel_pp_dict **dict_p, const char *key, size_t key_len, void *value);
cel_pp_str *dict_touch(cel_pp_dict **dict_p, const char *key, size_t key_len);

#endif
