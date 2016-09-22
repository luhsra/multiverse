/*
 * Licensed under the GPL v2
 *
 *
 * Further reading for function/compiler multiverse:
 *
 *  * "Function Multiverses for Dynamic Variability"
 *     https://www4.cs.fau.de/Publications/2016/rothberg_16_dspl.pdf
 *
 *  * Kernel Newbies wiki
 *    https://kernelnewbies.org/KernelProjects/compiler-multiverse
 */


#include "gcc-common.h"

/*
 * Produce declarations for all appropriate clones of FN.  If
 * UPDATE_METHOD_VEC_P is nonzero, the clones are added to the
 * CLASTYPE_METHOD_VEC as well.
 */
extern void clone_function_decl(tree, int);

extern void print_generic_stmt(FILE *, tree, int);

#define DEBUG

/*
 * All plugins must export this symbol so that they can be loaded with GCC.
 */
int plugin_is_GPL_compatible;


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
    fprintf(stderr, "---- Found MULTIVERSE variable : ");
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
 * Return true if var is multiverse attributed.
 */
static bool is_multiverse_var(tree &var)
{
	tree attr = lookup_attribute("multiverse", DECL_ATTRIBUTES(var));
	if(attr == NULL_TREE)
		return false;
	return true;
}


static unsigned int find_mv_vars_execute()
{
	basic_block bb;

	FOR_EACH_BB_FN(bb, cfun) {
        gimple_stmt_iterator gsi;
        for (gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
            gimple stmt;

            stmt = gsi_stmt(gsi);

#ifdef DEBUG
            print_gimple_stmt(stderr, stmt, 0, TDF_DETAILS);
#endif
        }
	}

    varpool_node_ptr node;

	printf("\n\nSearching multiverse variables\n");

	FOR_EACH_VARIABLE(node) {
		tree var = NODE_DECL(node);
		print_generic_stmt(stderr, var, 0);

		if(is_multiverse_var(var)) {
			printf("\tTHAT'S MULTIVERSE BABY!\n");
            clone_function_decl(NODE_DECL(cfun), 0);
		} else {
			printf("\tIt's NULL_TREE :(\n");
		}
	}

//    clone_function_decl(

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
    fprintf(stderr, "Initializing the multiverse GCC plugin.\n");
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
