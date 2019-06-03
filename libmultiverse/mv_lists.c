/* Debug headers */
#include <linux/printk.h>
#include <linux/kern_levels.h>
/* kmalloc/kfree */
#include <linux/slab.h>
#include <linux/gfp.h>
/* multiverse */
#include "mv_lists.h"

#include <linux/string.h>

/* The following global lists remain in the kernel space for it's complete live time. */
/* They represent each individual module multiverse data. */
/* The first entry of every list will be filled by the multiverse data of the kernel itself. */
struct mv_info_fn_xt *mv_info_fn_xt_list;
struct mv_info_callsite_xt *mv_info_callsite_xt_list;
struct mv_info_var_xt *mv_info_var_xt_list;

void multiverse_append_var_list(struct mv_info_var *start, struct mv_info_var *stop, const char *extension_name, void *mod_addr) {
    struct mv_info_var_xt *entry;
    struct mv_info_var_xt *global_var_list_iter;
    struct mv_info_var *iter;
    size_t mv_info_var_len = 0;
    size_t mv_info_var_index = 0;

    if(!start || !stop) return;
    if(stop < start) return;

    entry = kmalloc(sizeof(struct mv_info_var_xt), GFP_KERNEL);
    if(!entry) {
        printk(KERN_WARNING "Failed to allocate memory for mv var list by calling kmalloc.\n");
        return;
    }

    /* Prepare new entry */
    /* Calculate the list size */
    for(iter = start; iter < stop; iter++, mv_info_var_len++);


    entry->next = NULL;
    entry->mv_info_var = kmalloc(sizeof(struct mv_info_var *) * mv_info_var_len, GFP_KERNEL);
    entry->mv_info_var_len = mv_info_var_len;
    entry->xt_name = extension_name;
    entry->mod_addr = mod_addr;

    if(!entry->mv_info_var){
        printk(KERN_WARNING "Failed to allocate memory for mv_info_var by calling kmalloc.\n");

        /* Free up */
        kfree(entry);
    }

    /* Preparation done. Save the entries. */
    for(iter = start; iter < stop; iter++) {
        entry->mv_info_var[mv_info_var_index] = iter;
        mv_info_var_index++;
    }

    /* Append it at the end of the global list. */
    /* If it is empty, choose it as first candidate. */
    if(!mv_info_var_xt_list) {
        mv_info_var_xt_list = entry;
    }
    else {
        for(global_var_list_iter = mv_info_var_xt_list; global_var_list_iter->next != NULL; global_var_list_iter = global_var_list_iter->next);
        global_var_list_iter->next = entry;
    }
}

void multiverse_append_fn_list(struct mv_info_fn *start, struct mv_info_fn *stop, const char *extension_name, void *mod_addr) {
    struct mv_info_fn_xt *entry;
    struct mv_info_fn_xt *global_fn_list_iter;
    struct mv_info_fn *iter;
    size_t mv_info_fn_len = 0;
    size_t mv_info_fn_index = 0;

    if(!start || !stop) return;
    if(stop < start) return;

    entry = kmalloc(sizeof(struct mv_info_fn_xt), GFP_KERNEL);
    if(!entry) {
        printk(KERN_WARNING "Failed to allocate memory for mv fn list by calling kmalloc.\n");
        return;
    }

    /* Prepare new entry */
    /* Calculate the list size */
    for(iter = start; iter < stop; iter++, mv_info_fn_len++);

    entry->next = NULL;
    entry->mv_info_fn = kmalloc(sizeof(struct mv_info_fn *) * mv_info_fn_len, GFP_KERNEL);
    entry->mv_info_fn_len = mv_info_fn_len;
    entry->xt_name = extension_name;
    entry->mod_addr = mod_addr;

    if(!entry->mv_info_fn){
        printk(KERN_WARNING "Failed to allocate memory for mv_info_fn by calling kmalloc.\n");

        /* Free up */
        kfree(entry);
    }

    /* Preparation done. Save the entries. */
    for(iter = start; iter < stop; iter++) {
        entry->mv_info_fn[mv_info_fn_index] = iter;
        mv_info_fn_index++;
    }

    /* Append it at the end of the global list. */
    /* If it is empty, choose it as first candidate. */
    if(!mv_info_fn_xt_list) {
        mv_info_fn_xt_list = entry;
    }
    else {
        for(global_fn_list_iter = mv_info_fn_xt_list; global_fn_list_iter->next != NULL; global_fn_list_iter = global_fn_list_iter->next);
        global_fn_list_iter->next = entry;
    }
}
void multiverse_append_callsite_list(struct mv_info_callsite *start, struct mv_info_callsite *stop, const char *extension_name, void *mod_addr) {
    struct mv_info_callsite_xt *entry;
    struct mv_info_callsite_xt *global_callsite_list_iter;
    struct mv_info_callsite *iter;
    size_t mv_info_callsite_len = 0;
    size_t mv_info_callsite_index = 0;

    if(!start || !stop) return;
    if(stop < start) return;

    entry = kmalloc(sizeof(struct mv_info_callsite_xt), GFP_KERNEL);
    if(!entry) {
        printk(KERN_WARNING "Failed to allocate memory for mv callsite list by calling kmalloc.\n");
        return;
    }

    /* Prepare new entry */
    /* Calculate the list size */
    for(iter = start; iter < stop; iter++, mv_info_callsite_len++);

    entry->next = NULL;
    entry->mv_info_callsite = kmalloc(sizeof(struct mv_info_callsite *) * mv_info_callsite_len, GFP_KERNEL);
    entry->mv_info_callsite_len = mv_info_callsite_len;
    entry->xt_name = extension_name;
    entry->mod_addr = mod_addr;

    if(!entry->mv_info_callsite){
        printk(KERN_WARNING "Failed to allocate memory for mv_info_callsite by calling kmalloc.\n");

        /* Free up */
        kfree(entry);
    }

    /* Preparation done. Save the entries. */
    for(iter = start; iter < stop; iter++) {
        entry->mv_info_callsite[mv_info_callsite_index] = iter;
        mv_info_callsite_index++;
    }

    /* Append it at the end of the global list. */
        /* If it is empty, choose it as first candidate. */
    if(!mv_info_callsite_xt_list) {
        mv_info_callsite_xt_list = entry;
    }
    else {
        for(global_callsite_list_iter = mv_info_callsite_xt_list; global_callsite_list_iter->next != NULL; global_callsite_list_iter = global_callsite_list_iter->next);
        global_callsite_list_iter->next = entry;
    }
}

void multiverse_remove_var_list(struct mv_info_var_xt *entry) {
    // Use the generic struct version for removal.
    struct mv_info_gen_xt **gen_var_list_address = (struct mv_info_gen_xt**)(&mv_info_var_xt_list);
    struct mv_info_gen_xt *gen_entry = (struct mv_info_gen_xt*)entry;

    multiverse_remove_generic_extension_entry(gen_var_list_address, gen_entry);
}
void multiverse_remove_fn_list(struct mv_info_fn_xt *entry) {
    // Use the generic struct version for removal.
    struct mv_info_gen_xt **gen_fn_list_address = (struct mv_info_gen_xt**)(&mv_info_fn_xt_list);
    struct mv_info_gen_xt *gen_entry = (struct mv_info_gen_xt*)entry;

    multiverse_remove_generic_extension_entry(gen_fn_list_address, gen_entry);
}
void multiverse_remove_callsite_list(struct mv_info_callsite_xt *entry) {
    // Use the generic struct version for removal.
    struct mv_info_gen_xt **gen_callsite_list_address = (struct mv_info_gen_xt**)(&mv_info_callsite_xt_list);
    struct mv_info_gen_xt *gen_entry = (struct mv_info_gen_xt*)entry;

    multiverse_remove_generic_extension_entry(gen_callsite_list_address, gen_entry);
}

struct mv_info_var_xt* multiverse_get_mod_var_xt_entry(void *mod_addr) {
    struct mv_info_var_xt *iter;

    for(iter = mv_info_var_xt_list; iter != NULL; iter = iter->next){
        if(iter->mod_addr == mod_addr) return iter;
    }

    return NULL;
}

struct mv_info_fn_xt* multiverse_get_mod_fn_xt_entry(void *mod_addr) {
    struct mv_info_fn_xt *iter;

    for(iter = mv_info_fn_xt_list; iter != NULL; iter = iter->next){
        if(iter->mod_addr == mod_addr) return iter;
    }

    return NULL;
}

struct mv_info_callsite_xt* multiverse_get_mod_callsite_xt_entry(void *mod_addr) {
    struct mv_info_callsite_xt *iter;

    for(iter = mv_info_callsite_xt_list; iter != NULL; iter = iter->next){
        if(iter->mod_addr == mod_addr) return iter;
    }

    return NULL;
}

struct mv_info_var_xt *multiverse_get_mod_var_xt_entries(void) {
    return mv_info_var_xt_list;
}

struct mv_info_fn *multiverse_get_mod_fn_entry(void *function_body) {
    struct mv_info_fn_xt *iter;
    
    if(!function_body) return NULL;

    for(iter = mv_info_fn_xt_list; iter != NULL; iter = iter->next){
        int i;
        for(i = 0; i < iter->mv_info_fn_len; i++){
            struct mv_info_fn *fn_entry = iter->mv_info_fn[i];

            if(fn_entry->function_body == function_body){
                return fn_entry;
            }
        }
    }

    return NULL;
}

struct mv_info_mvfn *multiverse_get_mod_mvfn_entry(void *function_body) {
    struct mv_info_fn_xt *iter;
    
    if(!function_body) return NULL;

    for(iter = mv_info_fn_xt_list; iter != NULL; iter = iter->next){
        int i;
        for(i = 0; i < iter->mv_info_fn_len; i++){
            struct mv_info_fn *fn_entry = iter->mv_info_fn[i];

            if(fn_entry->mv_functions && fn_entry->n_mv_functions > 0){
                unsigned int mvfn_index = 0;

                for(; mvfn_index < fn_entry->n_mv_functions; mvfn_index++){
                    struct mv_info_mvfn mvfn_entry = fn_entry->mv_functions[mvfn_index];

                    if(mvfn_entry.function_body == function_body){
                        return &(fn_entry->mv_functions[mvfn_index]);
                    }
                }
            }
        }
    }

    return NULL;
}

void multiverse_remove_generic_extension_entry(struct mv_info_gen_xt **list, struct mv_info_gen_xt *entry) {
    struct mv_info_gen_xt *list_iter;
    struct mv_info_gen_xt *free_entry;

    /* Invalid parameter. */
    if(!list || !*list || !entry) return;

    /* Check if we need to remove the first entry of the list. */
    if(entry == *list){
        /* Save copy for freeing. */
        free_entry = *list;

        /* Remove the list entry. Next can be NULL, but this is of no importance. */
        *list = (*list)->next;
    }
    /* Search the entry in the list. */
    else {
        for(list_iter = *list; list_iter->next != NULL; list_iter++) {
            if(list_iter->next == entry){
                /* Save copy for freeing. */
                free_entry = list_iter->next;

                /* Remove the list entry. Next next can be NULL, but this is of no importance. */
                list_iter->next = list_iter->next->next;
            }
        }
    }

    /* Clean up. */
    multiverse_free_generic_extension_entry(free_entry);

}

void multiverse_free_generic_extension_entry(struct mv_info_gen_xt *entry) {
    if(!entry) return;

    if(entry->mv_info_entity_list) kfree(entry->mv_info_entity_list);
    kfree(entry);
}

void multiverse_debug_lists(void) {
    printk(KERN_INFO "[=== Multiverse debug lists start ===]\n");
    if(mv_info_var_xt_list) {
        struct mv_info_var_xt *iter;
        printk(KERN_INFO "Multiverse var instance list: \n");
        for(iter = mv_info_var_xt_list; iter != NULL; iter = iter->next) {
            unsigned int i;
            printk(KERN_INFO "Instance: %s\n", iter->xt_name);
            printk(KERN_INFO "---------\n");

            for(i = 0; i < iter->mv_info_var_len; i++){
                struct mv_info_var *var = iter->mv_info_var[i];

                printk(KERN_INFO "Variable '%s'\n", var->name);
            }
            printk(KERN_INFO "---------\n");
        }
    }
    if(mv_info_fn_xt_list) {
        struct mv_info_fn_xt *iter;
        printk(KERN_INFO "Multiverse fn instance list: \n");
        for(iter = mv_info_fn_xt_list; iter != NULL; iter = iter->next) {
            unsigned int i;
            printk(KERN_INFO "Instance: %s\n", iter->xt_name);
            printk(KERN_INFO "---------\n");

            for(i = 0; i < iter->mv_info_fn_len; i++){
                struct mv_info_fn *fn = iter->mv_info_fn[i];

                printk(KERN_INFO "Fn '%s' at 0x%llx\n", fn->name, (unsigned long long)fn->function_body);
            }
            printk(KERN_INFO "---------\n");
        }
    }
    if(mv_info_callsite_xt_list) {
        struct mv_info_callsite_xt *iter;
        printk(KERN_INFO "Multiverse callsite instance list: \n");
        for(iter = mv_info_callsite_xt_list; iter != NULL; iter = iter->next) {
            unsigned int i;
            printk(KERN_INFO "Instance: %s\n", iter->xt_name);
            printk(KERN_INFO "---------\n");

            for(i = 0; i < iter->mv_info_callsite_len; i++) {
                struct mv_info_callsite *callsite = iter->mv_info_callsite[i];

                printk(KERN_INFO "Function 0x%llx with call label 0x%llx\n", (unsigned long long)callsite->function_body, (unsigned long long)callsite->call_label);
            }
            printk(KERN_INFO "---------\n");
        }
    }
    printk(KERN_INFO "[=== Multiverse debug lists stop ===]\n");
}
