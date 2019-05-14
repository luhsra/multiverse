#ifndef __MV_KERNEL_LISTS
#define __MV_KERNEL_LISTS

#include <linux/export.h>
#include "multiverse.h"

/* Module registration helper methods. */
void multiverse_append_var_list(struct mv_info_var *start, struct mv_info_var *stop, const char *extension_name, void *mod_addr);
void multiverse_append_fn_list(struct mv_info_fn *start, struct mv_info_fn *stop, const char *extension_name, void *mod_addr);
void multiverse_append_callsite_list(struct mv_info_callsite *start, struct mv_info_callsite *stop, const char *extension_name, void *mod_addr);

void multiverse_remove_var_list(struct mv_info_var_xt *entry);
void multiverse_remove_fn_list(struct mv_info_fn_xt *entry);
void multiverse_remove_callsite_list(struct mv_info_callsite_xt *entry);

/* Getting the multiverse-info-extended struct information of a module. */
struct mv_info_var_xt* multiverse_get_mod_var_xt_entry(void *mod_addr);
struct mv_info_fn_xt* multiverse_get_mod_fn_xt_entry(void *mod_addr);
struct mv_info_callsite_xt* multiverse_get_mod_callsite_xt_entry(void *mod_addr);

/* Search for a mv_info_fn by it's body. */
struct mv_info_fn *multiverse_get_mod_fn_entry(void *function_body);

/* Generic methods to simplify removal process. */
void multiverse_remove_generic_extension_entry(struct mv_info_gen_xt **list, struct mv_info_gen_xt *entry);
void multiverse_free_generic_extension_entry(struct mv_info_gen_xt *entry);

void multiverse_debug_lists(void);

EXPORT_SYMBOL(multiverse_append_var_list);
EXPORT_SYMBOL(multiverse_append_fn_list);
EXPORT_SYMBOL(multiverse_append_callsite_list);
EXPORT_SYMBOL(multiverse_debug_lists);

#endif