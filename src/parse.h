#ifndef CEL_PP_PARSE_H
#define CEL_PP_PARSE_H

#include "core.h"

typedef enum {
	CEL_PP_PARSE_EOF,
	CEL_PP_PARSE_NEWLINE,
	CEL_PP_PARSE_WHITESPACE,
	CEL_PP_PARSE_HEADER_NAME,
	CEL_PP_PARSE_IDENTIFIER,
	CEL_PP_PARSE_PP_NUMBER,
	CEL_PP_PARSE_CHARACTER_CONSTANT,
	CEL_PP_PARSE_STRING_LITERAL,
	CEL_PP_PARSE_PUNCTUATOR,
	CEL_PP_PARSE_OTHER_CHAR,
} cel_pp_parse_type;

typedef struct {
	const cel_char *src;
	size_t len;
} cel_pp_parse_span;

#endif
