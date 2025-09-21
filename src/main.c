#include "parse.h"

#include <stdio.h>

cel_pp_parse_type parse_define_line(cel_pp_parse_span *span,
                                    cel_pp_parse_span *identifier,
                                    cel_pp_parse_span *identifier_list,
                                    cel_pp_parse_span *replacement_list);

int main(int argc, char **args)
{
	if (argc < 2) {
		return 0;
	}

	cel_pp_parse_span span, identifier, identifier_list, replacement_list;
	span.start = (cel_char *)args[1];
	span.end = span.start;

	cel_pp_parse_type retval = parse_define_line(
		&span, &identifier, &identifier_list, &replacement_list);
	switch (retval) {
	case CEL_PP_PARSE_DEFINE_MACRO_OBJ:
		printf("identifier: %.*s\n",
		       (int)((cel_uptr)identifier.end - (cel_uptr)identifier.start),
		       identifier.start);
		printf("replacement_list: %.*s\n",
		       (int)((cel_uptr)replacement_list.end
		             - (cel_uptr)replacement_list.start),
		       replacement_list.start);
		break;
	case CEL_PP_PARSE_DEFINE_MACRO_FN:
		printf("identifier: %.*s\n",
		       (int)((cel_uptr)identifier.end - (cel_uptr)identifier.start),
		       identifier.start);
		printf("identifier_list: %.*s\n",
		       (int)((cel_uptr)identifier_list.end
		             - (cel_uptr)identifier_list.start),
		       identifier_list.start);
		printf("replacement_list: %.*s\n",
		       (int)((cel_uptr)replacement_list.end
		             - (cel_uptr)replacement_list.start),
		       replacement_list.start);
		break;
	default:
		printf("Not a define\n");
		break;
	}

	return 0;
}
