#include "rc.h"

#include <stdalign.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	size_t ref;
	size_t size;
	char alignas(max_align_t) ptr[];
} cel_pp_rc;

static inline size_t tag_size(size_t data_size)
{
	return offsetof(cel_pp_rc, ptr) + data_size;
}

void *rc_alloc(size_t size)
{
	cel_pp_rc *tag = malloc(tag_size(size));
	if (!tag) {
		exit(1);
	}

	tag->ref = 1;
	tag->size = size;

	return tag->ptr;
}

static inline cel_pp_rc *get_tag(void *ptr)
{
	return (cel_pp_rc *)((char *)ptr - offsetof(cel_pp_rc, ptr));
}

void *rc_ref(void *ptr)
{
	if (!ptr) {
		return NULL;
	}

	cel_pp_rc *tag = get_tag(ptr);
	tag->ref++;

	return ptr;
}

bool rc_unref(void *ptr)
{
	if (!ptr) {
		return false;
	}

	cel_pp_rc *tag = get_tag(ptr);
	tag->ref--;

	return !tag->ref;
}

void rc_free(void *ptr)
{
	if (ptr) {
		free(get_tag(ptr));
	}
}

bool rc_edit(void **ptr_p)
{
	void *ptr = *ptr_p;
	if (!ptr) {
		return false;
	}

	cel_pp_rc *src = get_tag(ptr);
	if (src->ref == 1) {
		return false;
	}
	src->ref--;

	size_t size = src->size;
	ptr = rc_alloc(size);
	memcpy(ptr, src->ptr, size);
	*ptr_p = ptr;

	return true;
}

bool rc_realloc(void **ptr_p, size_t size)
{
	void *ptr = *ptr_p;
	if (!ptr) {
		ptr = rc_alloc(size);
		*ptr_p = ptr;

		return false;
	}

	cel_pp_rc *src = get_tag(ptr);
	if (src->ref == 1) {
		src = realloc(src, tag_size(size));
		if (!src) {
			exit(1);
		}

		src->size = size;
		*ptr_p = src->ptr;

		return false;
	}
	src->ref--;

	ptr = rc_alloc(size);
	memcpy(ptr, src->ptr, src->size);
	*ptr_p = ptr;

	return true;
}
