#include "fn.h"
#include "list.h"
#include "rc.h"
#include "str.h"

#include <stdio.h>

struct cel_pp_fn {
    cel_pp_str *name;
    cel_pp_list *args;
};

cel_pp_fn *fn_create(cel_pp_str *name, cel_pp_list *args)
{
    cel_pp_fn *fn = rc_alloc(sizeof(cel_pp_fn));
    fn->name = name;
    fn->args = args;
    return fn;
}

void fn_unref(cel_pp_fn *fn)
{
    if (rc_unref(fn)) {
        str_unref(fn->name);
        list_unref(fn->args);
        rc_free(fn);
    }
}

void fn_edit(cel_pp_fn **fn_p)
{
    if (rc_edit((void **)fn_p)) {
        cel_pp_fn *fn = *fn_p;
        rc_ref(fn->name);
        rc_ref(fn->args);
    }
}

void fn_print(const cel_pp_fn *fn)
{
    str_print(fn->name);
    printf("(");
    list_print(fn->args);
    printf(")");
}
