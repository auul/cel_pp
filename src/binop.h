#ifndef CEL_PP_BINOP_H
#define CEL_PP_BINOP_H

typedef enum {
    CEL_PP_BINOP_ADD,
    CEL_PP_BINOP_SUB,
    CEL_PP_BINOP_MUL,
    CEL_PP_BINOP_DIV,
} cel_pp_binop_type;
typedef struct cel_pp_binop cel_pp_binop;

#include "data.h"

cel_pp_binop *binop_create(cel_pp_binop_type type, cel_pp_data left,
                           cel_pp_data right);
void binop_unref(cel_pp_binop *binop);
void binop_print(const cel_pp_binop *binop);

#endif
