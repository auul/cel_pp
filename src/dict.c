#include "dict.h"
#include "rc.h"

#include <stdint.h>
#include <string.h>

struct cel_pp_dict {
	cel_pp_str *key;
	void *value;
	cel_pp_dict *left;
	cel_pp_dict *right;
};

cel_pp_str *dict_key(const cel_pp_dict *dict)
{
	return rc_ref((cel_pp_str *)dict->key);
}

void *dict_value(const cel_pp_dict *dict)
{
	return rc_ref((void *)dict->value);
}

static inline cel_pp_dict *
new_node(const char *key, size_t key_len, void *value)
{
	cel_pp_dict *node = rc_alloc(sizeof(cel_pp_dict));
	node->key = str_create_n(key, key_len);
	node->value = value;
	node->left = NULL;
	node->right = NULL;

	return node;
}

static inline void unref_value(void *value)
{
	if (rc_unref(value)) {
		rc_free(value);
	}
}

void dict_unref(cel_pp_dict *dict)
{
	while (rc_unref(dict)) {
		cel_pp_dict *right = dict->right;
		str_unref(dict->key);
		unref_value(dict->value);
		dict_unref(dict->left);
		rc_free(dict);
		dict = right;
	}
}

static inline void edit_node(cel_pp_dict **node_p)
{
	cel_pp_dict *node = *node_p;
	if (rc_edit((void **)&node)) {
		rc_ref(node->key);
		rc_ref(node->value);
		rc_ref(node->left);
		rc_ref(node->right);
	}
	*node_p = node;
}

static inline int
compare_key(const cel_pp_str *target, const char *key, size_t key_len)
{
	const char *target_data = str_data(target);
	size_t target_len = str_len(target);
	size_t n = target_len < key_len ? target_len : key_len;
	int cmp = memcmp(target_data, key, n);
	if (cmp) {
		return cmp;
	} else if (target_len < key_len) {
		return -1;
	} else if (target_len > key_len) {
		return 1;
	}
	return 0;
}

static inline cel_pp_dict *split_dict(
	cel_pp_dict **left, cel_pp_dict **right, cel_pp_dict *dict, const char *key,
	size_t key_len)
{
	while (dict) {
		edit_node(&dict);

		int cmp = compare_key(dict->key, key, key_len);
		if (cmp < 0) {
			cel_pp_dict *node = dict;
			dict = node->right;
			*left = node;
			left = &node->right;
		} else if (cmp > 0) {
			cel_pp_dict *node = dict;
			dict = node->left;
			*right = node;
			right = &node->left;
		} else {
			*left = dict->left;
			*right = dict->right;
			dict->left = NULL;
			dict->right = NULL;
			return dict;
		}
	}

	*left = NULL;
	*right = NULL;

	return NULL;
}

static inline uint32_t node_priority(const cel_pp_dict *node)
{
	uint64_t x = (uint64_t)(uintptr_t)node->key;
	x += 0x9e3779b97f4a7c15ull;
	x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
	x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
	x ^= x >> 31;
	return (uint32_t)(x ^ (x >> 32));
}

static inline cel_pp_dict *merge_dict(cel_pp_dict *left, cel_pp_dict *right)
{
	cel_pp_dict *dict = NULL;
	cel_pp_dict **dest = &dict;

	while (left && right) {
		if (node_priority(left) > node_priority(right)) {
			edit_node(&left);
			*dest = left;
			dest = &left->right;
			left = left->right;
		} else {
			edit_node(&right);
			*dest = right;
			dest = &right->left;
			right = right->left;
		}
	}

	if (left) {
		*dest = left;
	} else {
		*dest = right;
	}

	return dict;
}

const cel_pp_dict *
dict_lookup(const cel_pp_dict *dict, const char *key, size_t key_len)
{
	while (dict) {
		int cmp = compare_key(dict->key, key, key_len);
		if (cmp > 0) {
			dict = dict->left;
		} else if (cmp < 0) {
			dict = dict->right;
		} else {
			return dict;
		}
	}
	return NULL;
}

cel_pp_dict *
dict_define(cel_pp_dict **dict_p, const char *key, size_t key_len, void *value)
{
	cel_pp_dict *dict = *dict_p;
	cel_pp_dict *left = NULL;
	cel_pp_dict *right = NULL;
	cel_pp_dict *node = split_dict(&left, &right, dict, key, key_len);
	if (node) {
		unref_value(node->value);
		node->value = value;
	} else {
		node = new_node(key, key_len, value);
	}

	dict = merge_dict(merge_dict(left, node), right);
	*dict_p = dict;

	return node;
}

cel_pp_str *dict_touch(cel_pp_dict **dict_p, const char *key, size_t key_len)
{
	cel_pp_dict *dict = *dict_p;
	cel_pp_dict *node = (cel_pp_dict *)dict_lookup(dict, key, key_len);
	if (!node) {
		node = dict_define(dict_p, key, key_len, NULL);
		dict = *dict_p;
	}
	return rc_ref(node->key);
}
