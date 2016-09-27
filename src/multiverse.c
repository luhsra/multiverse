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

/* Build a clone of FNDECL with a modified name.  */

static tree clone_fndecl (tree fndecl)
{
  tree new_decl = copy_node (fndecl);
  tree new_name;
  std::string s;

  s = IDENTIFIER_POINTER (DECL_NAME (fndecl));
  s += ".multiverse";
  DECL_NAME (new_decl) = get_identifier (s.c_str ());


  s = IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (fndecl));
  s += ".multiverse";
  new_name = get_identifier (s.c_str ());
//  IDENTIFIER_TRANSPARENT_ALIAS (new_name) = 1;
//  TREE_CHAIN (new_name) = DECL_ASSEMBLER_NAME (fndecl);

  SET_DECL_ASSEMBLER_NAME (new_decl, new_name);

  /* We are going to modify attributes list and therefore should
     make own copy.  */
  DECL_ATTRIBUTES (new_decl) = copy_list (DECL_ATTRIBUTES (fndecl));

  /* Change builtin function code.  */
  if (DECL_BUILT_IN (new_decl))
    {
      gcc_assert (DECL_BUILT_IN_CLASS (new_decl) == BUILT_IN_NORMAL);
      gcc_assert (DECL_FUNCTION_CODE (new_decl) < BEGIN_CHKP_BUILTINS);
      DECL_FUNCTION_CODE (new_decl)
	= (enum built_in_function)(DECL_FUNCTION_CODE (new_decl)
				   + BEGIN_CHKP_BUILTINS + 1);
    }

  DECL_FUNCTION_CODE (new_decl);
  return new_decl;
}


#include <tree-chkp.h>
extern cgraph_node *chkp_maybe_create_clone (tree fndecl);

static void generate_fn_clones(tree &fndecl)
{
    tree new_decl;
    cgraph_node * node;
    cgraph_node * clone;

#ifdef DEBUG
    const char * fname = IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(fndecl));
    fprintf(stderr, "---- Generating function clones for '%s'\n", fname);
    fprintf(stderr, "---- Generating function clones for '%s'\n", fname);
#endif

    node = get_fn_cnode(fndecl);
    new_decl = clone_fndecl(fndecl);

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
    node->instrumented_version = clone;

    if (gimple_has_body_p(fndecl)) {
        tree_function_versioning(fndecl, new_decl, NULL, false,
                                 NULL, false, NULL, NULL);
        clone->lowered = true;
    }

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
	fprintf(stderr, "\n******** Searching multiverse variables in ");
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
