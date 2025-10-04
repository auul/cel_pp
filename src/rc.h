#ifndef CEL_PP_RC_H
#define CEL_PP_RC_H

#include <stdbool.h>
#include <stddef.h>

void *rc_alloc(size_t size);
void *rc_ref(void *ptr);
bool rc_unref(void *ptr);
void rc_free(void *ptr);
bool rc_edit(void **ptr_p);
bool rc_realloc(void **ptr_p, size_t size);

#endif
