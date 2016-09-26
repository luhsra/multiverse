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

// For debugging purposes
extern void print_generic_stmt(FILE *, tree, int);

/*
 * All plugins must export this symbol so that they can be loaded by GCC.
 */
int plugin_is_GPL_compatible = 0xF5F3;


struct plugin_info mv_plugin_info =
{
    .version = "092016",
    .help = NULL,
};


/*
 * Handler of the multiverse attribute.  Currently it's doing nothing but to
 * return NULL_TREE, but maybe we need to do something at a later point.
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
 * Return the call graph node of fndecl.  We need the corresponding cgrap node
 * for multiversing / cloning functions, which can only be done in the cgraph.
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
 * Clone fn_cnode and insert the clone into the call graph.
 *
 * Notes:
 *      - The function must be uninlineable
 */
static void generate_fn_clones(tree &orig)
{
//	struct cgraph_node  * fn_cnode = get_fn_cnode(decl);
//	struct cgraph_node  * clone = cgraph_create_node(decl);
//  tree decl = clone_function_name(orig, "my_fn_clone");

#ifdef DEBUG
    fprintf(stderr, "---- Generating function clones for ");
    print_generic_stmt(stderr, orig, 0);
#endif

    char fnname[32] = {0};
    tree decl, resdecl, initial, proto;

    snprintf(fnname, 31, "__FUNCTION_CLONE__");
    proto = build_varargs_function_type_list(integer_type_node, NULL_TREE);
    decl = build_fn_decl(fnname, proto);
    SET_DECL_ASSEMBLER_NAME(decl, get_identifier(fnname));

    /* Result */
    resdecl=build_decl(BUILTINS_LOCATION,RESULT_DECL,NULL_TREE,integer_type_node);
    DECL_ARTIFICIAL(resdecl) = 1;
    DECL_CONTEXT(resdecl) = decl;
    DECL_RESULT(decl) = resdecl;

    /* Initial */
    initial = make_node(BLOCK);
    TREE_USED(initial) = 1;
    DECL_INITIAL(decl) = initial;
    DECL_UNINLINABLE(decl) = 1;
    DECL_EXTERNAL(decl) = 0;
    DECL_PRESERVE_P(decl) = 1;

    /* Func decl */
    TREE_USED(decl) = 1;
    TREE_PUBLIC(decl) = 1;
    TREE_STATIC(decl) = 1;
    DECL_ARTIFICIAL(decl) = 1;

    /* Make the function */
    push_struct_function(decl);
    cfun->function_end_locus = BUILTINS_LOCATION;
    gimplify_function_tree(decl);

    /* Update */
    cgraph_node::add_new_function(decl, false);
    pop_cfun();

    return;
}


bool is_function_clone(tree decl)
{
    return false;
}


static int cloned = 0;

static unsigned int find_mv_vars_execute()
{
    // varpool doesn't work - we need _at least_ a list of referenced variables
    // in the respective function
    varpool_node_ptr node;

#ifdef DEBUG
	fprintf(stderr, "\n**** Searching multiverse variables in ");
    print_generic_stmt(stderr, cfun->decl, 0);
#endif

	FOR_EACH_VARIABLE(node) {
		tree var = NODE_DECL(node);
#ifdef DEBUG
        fprintf(stderr, "---- Found multiverse var ");
		print_generic_stmt(stderr, var, 0);
#endif

		if(!is_multiverse_var(var))
            continue;

//        printf("\tTHAT'S MULTIVERSE BABY!\n");
        // Get the node of cfun in the call graph to generate clones.
        if (cloned++ == 0) {
            generate_fn_clones(cfun->decl);
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

    find_mv_vars_info.pass = make_find_mv_vars_pass();
    find_mv_vars_info.reference_pass_name = "ssa";
    find_mv_vars_info.ref_pass_instance_number = 1;
    find_mv_vars_info.pos_op = PASS_POS_INSERT_AFTER;


    if (!plugin_default_version_check(version, &gcc_version)) {
        error(G_("incompatible gcc/plugin versions"));
        return 1;
    }

    // register plugin information
    register_callback(plugin_name, PLUGIN_INFO, NULL, &mv_plugin_info);

    // register the multiverse attribute
    register_callback(plugin_name, PLUGIN_ATTRIBUTES, register_mv_attribute, NULL);

    // register the multiverse GIMPLE pass
    register_callback(plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL, &find_mv_vars_info);

    return 0;
}
