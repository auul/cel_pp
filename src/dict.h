#ifndef CEL_PP_DICT_H
#define CEL_PP_DICT_H

#include "core.h"

typedef struct cel_pp_dict {
	cel_char *key;
	void *value;
	struct cel_pp_dict *left;
	struct cel_pp_dict *right;
} cel_pp_dict;

cel_pp_dict *
cel_pp_dict_insert(cel_pp_dict **dict_p, const cel_char *src, size_t len);

#endif
