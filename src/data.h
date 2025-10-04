#ifndef CEL_PP_DATA_H
#define CEL_PP_DATA_H

#include "str.h"

#include <stdint.h>

typedef struct cel_pp_list cel_pp_list;

typedef enum {
	CEL_PP_DATA_TEXT,
	CEL_PP_DATA_NAT,
	CEL_PP_DATA_LIST,
} cel_pp_type;

typedef struct {
	cel_pp_type type;
	union {
		cel_pp_str *str;
		cel_pp_list *list;
		uint64_t nat;
		void *ptr;
	};
} cel_pp_data;

cel_pp_data data_text(cel_pp_str *str);
cel_pp_data data_nat(uint64_t nat);
cel_pp_data data_list(cel_pp_list *list);
void data_print(const cel_pp_data *data);
void data_ref(cel_pp_data *data);
void data_unref(cel_pp_data *data);

#endif
