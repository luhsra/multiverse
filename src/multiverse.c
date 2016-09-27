/*
 * Copyright 2016 by Valentin Rothberg <valentinrothberg@gmail.com>
 * Licensed under the GPL v2
 *
 *
 * This GCC plugin adds support for multiversing functions, as described by
 * [1,2].  Currently, the multiverse support is limited to boolean variables
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

struct plugin_info mv_plugin_info = { .version = "092016" };


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
static tree clone_fndecl (tree fndecl, std::string suffix)
{
    tree new_decl;
    cgraph_node * node;
    cgraph_node * clone;
    std::string fname;

    new_decl = copy_node (fndecl);

    fname = IDENTIFIER_POINTER (DECL_NAME (fndecl));
    fname += MV_SUFFIX;  // ".multiverse"
    fname += suffix.c_str();
    DECL_NAME (new_decl) = get_identifier (fname.c_str ());

    SET_DECL_ASSEMBLER_NAME (new_decl, get_identifier(fname.c_str()));

    // Not sure if we need to copy the attributes list: better safe than sorry
    DECL_ATTRIBUTES (new_decl) = copy_list (DECL_ATTRIBUTES (fndecl));

    // Change builtin function code
    if (DECL_BUILT_IN (new_decl)) {
        gcc_assert (DECL_BUILT_IN_CLASS (new_decl) == BUILT_IN_NORMAL);
        gcc_assert (DECL_FUNCTION_CODE (new_decl) < BEGIN_CHKP_BUILTINS);
        DECL_FUNCTION_CODE (new_decl)= (enum built_in_function)
                                       (DECL_FUNCTION_CODE (new_decl)
                                       + BEGIN_CHKP_BUILTINS + 1);
    }

    DECL_FUNCTION_CODE (new_decl);

    node = get_fn_cnode(fndecl);
    clone = node->create_version_clone(new_decl, vNULL, NULL);
    clone->externally_visible = node->externally_visible;
    clone->local = node->local;
    clone->address_taken = node->address_taken;
    clone->thunk = node->thunk;
    clone->alias = node->alias;
    clone->weakref = node->weakref;
    clone->cpp_implicit_alias = node->cpp_implicit_alias;
    clone->instrumented_version = node;
    clone->orig_decl = fndecl;
    clone->instrumentation_clone = true;
//    node->instrumented_version = clone;

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
 * Set func ontop of the function stack.  That's required to alter the GIMPLE
 * statements in other functions than cfun.
 */
static inline void set_func(function *func)
{
    pop_cfun();
    push_cfun(func);
}


/*
 * Add a new const variable with value to the first basic block in fndecl and
 * replace all occurences of var with this new variable.  Later optimizations
 * passes will remove dead code and take care of the 'specialization' of
 * multiversed functions.
 */
static void replace_and_constify(tree &fndecl, tree &var, bool value)
{
    function * func = DECL_STRUCT_FUNCTION(fndecl);
    set_func(func);

    std::string var_name = IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(var));
    if (value)
        var_name += "_true";
    else
        var_name += "_false";

    // add a new local variable and the corresponding assignment to the
    // function body
    basic_block bb;
    tree new_var;

    FOR_EACH_BB_FN(bb, func) {
        gimple_stmt_iterator gsi;
        gimple stmt;
        new_var = create_tmp_var(integer_type_node, var_name.c_str());
        new_var = make_ssa_name(new_var, gimple_build_nop());
        stmt = gimple_build_assign(fndecl, new_var);
        for (gsi=gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
            gsi_insert_before(&gsi, stmt, GSI_NEW_STMT);
            break;
        }
        break;
    }
}


/*
 * TODO: this function must change soon (adding multiverse vectors as args etc.)
 */
static void multiverse_function(tree &fndecl, tree &var)
{
    tree clone;

#ifdef DEBUG
    const char * fname = IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(fndecl));
    fprintf(stderr, "---- Generating function clones for '%s'\n", fname);
#endif

    clone = clone_fndecl(fndecl, "_true");
    gcc_assert(clone != fndecl);

    function *func = cfun;

    replace_and_constify(clone, var, true);
    set_func(func);

    return;
}


/*
 * Return true if fndecl is a multiversed function (i.e., cloned and
 * '.multiverse' substring in the identifier).  The substring should be enough,
 * but as so oftern: better safe than sorry.
 */
bool is_multiverse_function(tree fndecl)
{
    cgraph_node * node;
    std::string fname;

    node = get_fn_cnode(fndecl);

    if (!node->instrumentation_clone)
        return false;

    fname = IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(fndecl));
    if (fname.find(MV_SUFFIX, 0) == std::string::npos)
        return false;

    return true;
}


/*
 * Pass to find multiverse attributed variables in the current function.  In
 * case such variables are used in conditional statements, the functions is
 * cloned and specialized (constant propagation, etc.).
 */
static unsigned int find_mv_vars_execute()
{
    // TODO: varpool doesn't work - we need _at least_ a list of referenced
    // variables in the respective function
    varpool_node_ptr node;
    std::string fname;

    fname = IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(cfun->decl));
    if (fname.compare("main") == 0)
        return 0;

#ifdef DEBUG
	fprintf(stderr, "\n******** Searching multiverse variables in '%s'\n", fname.c_str());
#endif

	FOR_EACH_VARIABLE(node) {
		tree var = NODE_DECL(node);

#ifdef DEBUG
        fprintf(stderr, "---- Found multiverse var ");
		print_generic_stmt(stderr, var, 0);
#endif

		if(!is_multiverse_var(var))
            continue;

        // TODO: this must be extended to a set of variables and not only one
        if (!is_multiverse_function(cfun->decl)) {
            multiverse_function(cfun->decl, var);
        } else {
#ifdef DEBUG
            fprintf(stderr, "---- Skipping multiversed function '%s'\n", fname.c_str());
#endif
        }
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
    find_mv_vars_info.reference_pass_name = "ssa";
    find_mv_vars_info.ref_pass_instance_number = 1;
    find_mv_vars_info.pos_op = PASS_POS_INSERT_AFTER;
    register_callback(plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL, &find_mv_vars_info);

    return 0;
}
