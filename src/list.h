#ifndef CEL_PP_LIST_H
#define CEL_PP_LIST_H

#include "data.h"

typedef struct cel_pp_list cel_pp_list;

void list_print(const cel_pp_list *list);
void list_unref(cel_pp_list *list);
void list_append(cel_pp_list ***end_cdr_pp, cel_pp_data data);
void list_prepend(cel_pp_list **list_p, cel_pp_data data);

#endif
