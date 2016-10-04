/*
 * Copyright 2016 by Valentin Rothberg <valentinrothberg@gmail.com>
 * Licensed under the GPL v2
 *
 *
 * This GCC plugin adds support for multiversing functions, as described by
 * [1,2].  Currently, the multiverse support is limited to 'boolean' variables
 * that must be attributed with 'multiverse'.
 *
 *
 * Further reading for function/compiler multiverse:
 *
 * [1] Kernel Newbies wiki
 *     https://kernelnewbies.org/KernelProjects/compiler-multiverse
 *
 * [2] "Function Multiverses for Dynamic Variability"
 *     https://www4.cs.fau.de/Publications/2016/rothberg_16_dspl.pdf
 */

#include "gcc-common.h"
#include <string>

#define MV_SUFFIX ".multiverse"

int plugin_is_GPL_compatible = 0xF5F3;

struct plugin_info mv_plugin_info = { .version = "102016" };


/*
 * Handler for multiverse attributing.  Currently it's only used for debugging
 * purposes but maybe we need to do something at a later point.
 */
static tree handle_mv_attribute(tree *node, tree name, tree args, int flags, bool *no_add_attrs)
{
#ifdef DEBUG
    fprintf(stderr, "---- attributing MULTIVERSE variable ");
    print_generic_stmt(stderr, *node, 0);
#endif
    return NULL_TREE;
}


/*
 * The struct below is required to register the __attribute__((multiverse))
 * attribute.  Please refer to tree-core.h for detailed information about this
 * struct and its members.
 */
static struct attribute_spec mv_attribute =
{
    .name = "multiverse",
    .min_length = 0,
    .max_length = 0,
    .decl_required = false,
    .type_required = false,
    .function_type_required = false,
    .handler = handle_mv_attribute,
    .affects_type_identity = false,
};


/*
 * The callback function to register the multiverse attribute.
 */
static void register_mv_attribute(void *event_data, void *data)
{
    register_attribute(&mv_attribute);
}


/*
 * Return the call graph node of fndecl.
 */
struct cgraph_node *get_fn_cnode(const_tree fndecl)
{
    gcc_assert(TREE_CODE(fndecl) == FUNCTION_DECL);
#if BUILDING_GCC_VERSION <= 4005
    return cgraph_get_node(CONST_CAST_TREE(fndecl));
#else
    return cgraph_get_node(fndecl);
#endif
}


/*
 * Return true if var is multiverse attributed.
 */
static bool is_multiverse_var(tree &var)
{
    if (!is_global_var(var) || TREE_CODE(var) != VAR_DECL)
        return false;

    tree attr = lookup_attribute("multiverse", DECL_ATTRIBUTES(var));
    if(attr == NULL_TREE)
        return false;
    return true;
}


/*
 * Build a clone of FNDECL with a modified name.
 *
 * Code bases on chkp_maybe_create_clone().
 */
static tree clone_fndecl(tree fndecl, std::string suffix)
{
    tree new_decl;
    cgraph_node * node;
    cgraph_node * clone;
    std::string fname;

    new_decl = copy_node(fndecl);

    fname = IDENTIFIER_POINTER(DECL_NAME(fndecl));
    fname += MV_SUFFIX;  // ".multiverse"
    fname += suffix.c_str();
    DECL_NAME(new_decl) = get_identifier(fname.c_str());

    SET_DECL_ASSEMBLER_NAME(new_decl, get_identifier(fname.c_str()));

    // Not sure if we need to copy the attributes list: better safe than sorry
    DECL_ATTRIBUTES(new_decl) = copy_list(DECL_ATTRIBUTES(fndecl));

    // Change builtin function code
    if (DECL_BUILT_IN(new_decl)) {
        gcc_assert(DECL_BUILT_IN_CLASS(new_decl) == BUILT_IN_NORMAL);
        gcc_assert(DECL_FUNCTION_CODE(new_decl) < BEGIN_CHKP_BUILTINS);
        DECL_FUNCTION_CODE(new_decl) = (enum built_in_function)
                                       (DECL_FUNCTION_CODE(new_decl)
                                       + BEGIN_CHKP_BUILTINS + 1);
    }

    DECL_FUNCTION_CODE(new_decl);

    node = get_fn_cnode(fndecl);
    clone = node->create_version_clone(new_decl, vNULL, NULL);
    clone->externally_visible = true;   // just to avoid being removed
    clone->local = node->local;
    clone->address_taken = node->address_taken;
    clone->thunk = node->thunk;
    clone->alias = node->alias;
    clone->weakref = node->weakref;
    clone->cpp_implicit_alias = node->cpp_implicit_alias;
    clone->orig_decl = fndecl;

    if (gimple_has_body_p(fndecl)) {
        tree_function_versioning(fndecl, new_decl, NULL, NULL,
                NULL, false, NULL, NULL);
        clone->lowered = true;
    }

#ifdef DEBUG
    fprintf(stderr, "---- Generated function clone '%s'\n", fname.c_str());
#endif

    return new_decl;
}


/*
 * Push func the function stack.  That's required to alter the GIMPLE
 * statements in other functions than cfun.
 */
static inline void set_func(function *func)
{
    pop_cfun();
    push_cfun(func);
}


/*
 * Replace all occurrences of var in cfun with value.
 */
static void replace_and_constify(tree old_var, const int value)
{
    tree new_var = build_int_cst(TREE_TYPE(old_var), value);

    basic_block bb;
	FOR_EACH_BB_FN(bb, cfun) {
        // iterate over each GIMPLE statement
        gimple_stmt_iterator gsi;
        for (gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
            gimple stmt = gsi_stmt(gsi);

#ifdef DEBUG
            fprintf(stderr, "---- Checking operands in statement: ");
            print_gimple_stmt(stderr, stmt, 0, TDF_SLIM);
#endif

            if (!is_gimple_assign(stmt)) {
                #ifdef DEBUG
                fprintf(stderr, "...skipping non-assign statement\n");
                #endif
                continue;
            }

            // check if any operand is a multiverse variable
            for (int num = 1; num < gimple_num_ops(stmt); num++) {
                tree var = gimple_op(stmt, num);
                if (var != old_var)
                    continue;

                gimple_set_op(stmt, num, new_var);
                update_stmt(stmt);
                update_stmt_if_modifie(stmt);
                fprintf(stderr, "...replacing operand in this statement: ");
                print_gimple_stmt(stderr, stmt, 0, TDF_SLIM);
            }
        }
    }
}


/*
 * TODO: this function must change soon (adding multiverse vectors as args etc.)
 */
static void multiverse_function(tree &fndecl, tree &var)
{
#ifdef DEBUG
    const char * fname = IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(fndecl));
    fprintf(stderr, "---- Generating function clones for '%s'\n", fname);
#endif

    tree clone;
    function * func;
    function * old_func = cfun;

    // case TRUE
    clone = clone_fndecl(fndecl, "_true");
    gcc_assert(clone != fndecl);
    func = DECL_STRUCT_FUNCTION(clone);
    set_func(func);
    replace_and_constify(var, true);

    // case FALSE
    clone = clone_fndecl(fndecl, "_false");
    gcc_assert(clone != fndecl);
    func = DECL_STRUCT_FUNCTION(clone);
    set_func(func);
    replace_and_constify(var, false);

    set_func(old_func);
    return;
}


/*
 * Return true if fndecl is cloneable (i.e., not main and not multiversed).
 */
bool is_cloneable_function(tree fndecl)
{
    std::string fname = IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(fndecl));

    if (fname.compare("main") == 0)
        return false;

    if (fname.find(MV_SUFFIX, 0) != std::string::npos)
        return false;

    return true;
}


/*
 * Pass to find multiverse attributed variables in the current function.  In
 * case such variables are used in conditional statements, the functions is
 * cloned and specialized (constant propagation, etc.).
 */
#include <set>
static unsigned int find_mv_vars_execute()
{
    std::string fname = IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(cfun->decl));

    if (!is_cloneable_function(cfun->decl)) {
#ifdef DEBUG
        fprintf(stderr, "---- Skipping non-multiverseable function '%s'\n", fname.c_str());
#endif
        return 0;
    }

#ifdef DEBUG
	fprintf(stderr, "\n******** Searching multiverse variables in '%s'\n", fname.c_str());
#endif

    std::set<tree> mv_vars;
	basic_block bb;
    // iterate of each basic block in current function
	FOR_EACH_BB_FN(bb, cfun) {
        // iterate over each GIMPLE statement
        gimple_stmt_iterator gsi;
        for (gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
            gimple stmt = gsi_stmt(gsi);

#ifdef DEBUG
            fprintf(stderr, "---- Checking operands in statement: ");
            print_gimple_stmt(stderr, stmt, 0, TDF_SLIM);
#endif

            if (!is_gimple_assign(stmt)) {
#ifdef DEBUG
                fprintf(stderr, "...skipping non-assign statement\n");
#endif
                continue;
            }

            // if left hand side is a multiverse var skip the function
            tree lhs = gimple_assign_lhs(stmt);
            if (is_multiverse_var(lhs)) {
#ifdef DEBUG
                fprintf(stderr, "...skipping function: assign to multiverse variable\n");
#endif
                return 0;
            }

            // check if any operand is a multiverse variable
            for (int num = 1; num < gimple_num_ops(stmt); num++) {
                tree var = gimple_op(stmt, num);

                if (!is_multiverse_var(var))
                    continue;

#ifdef DEBUG
                fprintf(stderr, "...found multiverse operand: ");
                print_generic_stmt(stderr, var, 0);
#endif
                mv_vars.insert(var);
            }
        }
    }

#ifdef DEBUG
    fprintf(stderr, "...found '%d' multiverse variables\n", mv_vars.size());
#endif
    std::set<tree>::iterator trit;
    for (trit = mv_vars.begin(); trit != mv_vars.end(); trit++) {
        tree var = *trit;
#ifdef DEBUG
        fprintf(stderr, "...replace and constify: ");
        print_generic_stmt(stderr, var, 0);
#endif
//        replace_and_constify(var, true);
        multiverse_function(cfun->decl, var);
    }

    return 0;
}


#define PASS_NAME find_mv_vars
#define NO_GATE
#include "gcc-generate-gimple-pass.h"

/*
 * Initialization function of this plugin: the very heart & soul.
 */
int plugin_init(struct plugin_name_args *info, struct plugin_gcc_version *version)
{
#ifdef DEBUG
    fprintf(stderr, "---- Initializing the multiverse GCC plugin.\n");
#endif

    const char * plugin_name = info->base_name;
    struct register_pass_info find_mv_vars_info;

    if (!plugin_default_version_check(version, &gcc_version)) {
        error(G_("incompatible gcc/plugin versions"));
        return 1;
    }

    // register plugin information
    register_callback(plugin_name, PLUGIN_INFO, NULL, &mv_plugin_info);

    // register the multiverse attribute
    register_callback(plugin_name, PLUGIN_ATTRIBUTES, register_mv_attribute, NULL);

    // register the multiverse GIMPLE pass
    find_mv_vars_info.pass = make_find_mv_vars_pass();
    find_mv_vars_info.reference_pass_name = "early_optimizations";
    find_mv_vars_info.ref_pass_instance_number = 0;
    find_mv_vars_info.pos_op = PASS_POS_INSERT_BEFORE;
    register_callback(plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL, &find_mv_vars_info);

    return 0;
}
