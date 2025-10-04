#include "list.h"
#include "rc.h"

#include <stdio.h>

struct cel_pp_list {
	cel_pp_data car;
	cel_pp_list *cdr;
};

void list_print(const cel_pp_list *list)
{
	while (list) {
		data_print(&list->car);
		list = list->cdr;
		if (list) {
			printf(", ");
		}
	}
}

void list_unref(cel_pp_list *list)
{
	while (rc_unref(list)) {
		cel_pp_list *cdr = list->cdr;
		data_unref(&list->car);
		rc_free(list);
		list = cdr;
	}
}

static inline cel_pp_list *new_node(cel_pp_data car)
{
	cel_pp_list *node = rc_alloc(sizeof(cel_pp_list));
	node->car = car;
	node->cdr = NULL;
	return node;
}

static void edit_node(cel_pp_list **node_p)
{
	if (rc_edit((void **)node_p)) {
		cel_pp_list *node = *node_p;
		data_ref(&node->car);
	}
}

void list_append(cel_pp_list ***end_cdr_pp, cel_pp_data data)
{
	cel_pp_list **cdr_p = *end_cdr_pp;
	cel_pp_list *node = new_node(data);
	*cdr_p = node;
	*end_cdr_pp = &node->cdr;
}

void list_prepend(cel_pp_list **list_p, cel_pp_data data)
{
	cel_pp_list *node = new_node(data);
	node->cdr = *list_p;
	*list_p = node;
}
