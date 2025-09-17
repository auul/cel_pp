#include "dict.h"

#include <string.h>

static cel_pp_dict *
new_dict_node(cel_char *key, void *value, cel_pp_dict *left, cel_pp_dict *right)
{
	cel_pp_dict *node = malloc(sizeof(cel_pp_dict));
	if (!node) {
		return NULL;
	}

	node->key = key;
	node->value = value;
	node->left = left;
	node->right = right;

	return node;
}

static int compare_key(const cel_char *key, const cel_char *src, size_t len)
{
	size_t i;
	for (i = 0; i < len; i++) {
		if (key[i] > src[i]) {
			return 1;
		} else if (key[i] < src[i]) {
			return -1;
		}
	}

	if (key[i]) {
		return 1;
	}
	return 0;
}

static cel_pp_dict **
find_node(cel_pp_dict **dict_p, const cel_char *src, size_t len)
{
	cel_pp_dict *dict = *dict_p;
	while (dict) {
		int cmp = compare_key(dict->key, src, len);
		if (cmp < 0) {
			dict_p = &dict->left;
		} else if (cmp > 0) {
			dict_p = &dict->right;
		} else {
			return dict_p;
		}
		dict = *dict_p;
	}
	return dict_p;
}

cel_pp_dict *
cel_pp_dict_insert(cel_pp_dict **dict_p, const cel_char *src, size_t len)
{
	dict_p = find_node(dict_p, src, len);
	if (*dict_p) {
		return *dict_p;
	}

	cel_char *key = malloc(len + 1);
	if (!key) {
		return NULL;
	}
	memcpy(key, src, len);
	key[len] = 0;

	*dict_p = new_dict_node(key, NULL, NULL, NULL);
	if (!*dict_p) {
		free(key);
		return NULL;
	}
	return *dict_p;
}
