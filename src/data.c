#include "data.h"
#include "list.h"
#include "rc.h"

#include <stdio.h>

cel_pp_data data_text(cel_pp_str *str)
{
	cel_pp_data data = {.type = CEL_PP_DATA_TEXT, .str = str};
	return data;
}

cel_pp_data data_nat(uint64_t nat)
{
	cel_pp_data data = {.type = CEL_PP_DATA_NAT, .nat = nat};
	return data;
}

cel_pp_data data_list(cel_pp_list *list)
{
	cel_pp_data data = {.type = CEL_PP_DATA_LIST, .list = list};
	return data;
}

void data_print(const cel_pp_data *data)
{
	switch (data->type) {
	case CEL_PP_DATA_TEXT:
		printf("%s", str_data(data->str));
		break;
	case CEL_PP_DATA_NAT:
		printf("%lu", data->nat);
		break;
	case CEL_PP_DATA_LIST:
		printf("[");
		list_print(data->list);
		printf("]");
		break;
	default:
		printf("%p", data->ptr);
		break;
	}
}

void data_ref(cel_pp_data *data)
{
	switch (data->type) {
	case CEL_PP_DATA_TEXT:
	case CEL_PP_DATA_LIST:
		rc_ref(data->ptr);
		break;
	default:
		break;
	}
}

void data_unref(cel_pp_data *data)
{
	switch (data->type) {
	case CEL_PP_DATA_TEXT:
		str_unref(data->str);
		break;
	case CEL_PP_DATA_LIST:
		list_unref(data->list);
		break;
	default:
		break;
	}
}
