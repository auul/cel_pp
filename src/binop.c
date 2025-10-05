#include "binop.h"
#include "rc.h"

#include <stdio.h>

struct cel_pp_binop {
    cel_pp_binop_type type;
    cel_pp_data left;
    cel_pp_data right;
};

cel_pp_binop *binop_create(cel_pp_binop_type type, cel_pp_data left,
                           cel_pp_data right)
{
    cel_pp_binop *binop = rc_alloc(sizeof(cel_pp_binop));
    binop->type = type;
    binop->left = left;
    binop->right = right;
    return binop;
}

void binop_unref(cel_pp_binop *binop)
{
    if (rc_unref(binop)) {
        data_unref(&binop->left);
        data_unref(&binop->right);
        rc_free(binop);
    }
}

void binop_print(const cel_pp_binop *binop)
{
    printf("(");
    data_print(&binop->left);

    switch (binop->type) {
    case CEL_PP_BINOP_ADD:
        printf("+");
        break;
    case CEL_PP_BINOP_SUB:
        printf("-");
        break;
    case CEL_PP_BINOP_MUL:
        printf("*");
        break;
    case CEL_PP_BINOP_DIV:
        printf("/");
        break;
    default:
        printf("(%u)", binop->type);
        break;
    }

    data_print(&binop->right);
    printf(")");
}
