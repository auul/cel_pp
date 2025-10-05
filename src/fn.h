#ifndef CEL_PP_FN_H
#define CEL_PP_FN_H

typedef struct cel_pp_fn cel_pp_fn;
typedef struct cel_pp_str cel_pp_str;
typedef struct cel_pp_list cel_pp_list;

cel_pp_fn *fn_create(cel_pp_str *name, cel_pp_list *args);
void fn_unref(cel_pp_fn *fn);
void fn_edit(cel_pp_fn **fn_p);
void fn_print(const cel_pp_fn *fn);

#endif
