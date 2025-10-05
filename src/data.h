#ifndef CEL_PP_DATA_H
#define CEL_PP_DATA_H

#include <stdint.h>

typedef struct cel_pp_list cel_pp_list;
typedef struct cel_pp_str cel_pp_str;
typedef struct cel_pp_fn cel_pp_fn;
typedef struct cel_pp_fn cel_pp_fn;
typedef struct cel_pp_binop cel_pp_binop;

typedef enum {
    CEL_PP_DATA_NAT,
    CEL_PP_DATA_TEXT,
    CEL_PP_DATA_ID,
    CEL_PP_DATA_LIST,
    CEL_PP_DATA_FN,
    CEL_PP_DATA_BINOP,
} cel_pp_type;

typedef struct {
    cel_pp_type type;
    union {
        cel_pp_str *str;
        cel_pp_list *list;
        cel_pp_fn *fn;
        cel_pp_binop *binop;
        uint64_t nat;
        void *ptr;
    };
} cel_pp_data;

cel_pp_data data_text(cel_pp_str *str);
cel_pp_data data_nat(uint64_t nat);
cel_pp_data data_list(cel_pp_list *list);
cel_pp_data data_fn(cel_pp_fn *fn);
void data_print(const cel_pp_data *data);
void data_ref(cel_pp_data *data);
void data_unref(cel_pp_data *data);

#endif
