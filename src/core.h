#ifndef CEL_PP_CORE_H
#define CEL_PP_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define CEL_UNUSED(name) ((void)(name))

typedef uint8_t cel_u8;
typedef unsigned char cel_char;
typedef uintptr_t cel_uptr;

enum {
	CEL_PP_E_ALLOC = -1,

	CEL_PP_SUCCESS = 0
};

#endif
