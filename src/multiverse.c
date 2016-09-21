/*
 * Licensed under the GPL v2, or (at your option) v3
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


#include <gcc-plugin.h>
#include <tree.h>
#include <tree-pass.h>

extern void print_generic_stmt(FILE *, tree, int);

#define DEBUG

/*
 * All plugins must export this symbol so that they can be loaded with GCC.
 */
int plugin_is_GPL_compatible;


/*
 * The following data is required for 'gcc --version / --help' and is thus
 * useful to expose the interface of the plugin and further information to the
 * user.
 */
struct plugin_info mv_plugin_info =
{
    .version = "0.1",
    .help = "Here should be something written about how to use this plugin.",
};


/*
 * Specify which version(s) of the GCC are compatible to this plugin.  There are
 * more members, such as time stamp or revision, to further narrow
 * compatibility.  Look into 'plugin-version.h' to check the fields of your
 * infrastructure if compatibility problems occur.
 */
static struct plugin_gcc_version mv_plugin_version =
{
    .basever = "6",
};


/*
 * Handler of the multiverse attribute.  Currently it's doing nothing but to
 * return NULL_TREE, but maybe we need to do something at a later point.
 */

static tree handle_mv_attribute(tree *node, tree name, tree args, int flags, bool *no_add_attrs)
{
#ifdef DEBUG
    fprintf(stderr, "Found attribute\n");
    fprintf(stderr, "\tvariable name = ");
    print_generic_stmt(stderr, *node, 0);
    fprintf(stderr, "\tattribute = ");
    print_generic_stmt(stderr, name, 0);
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
 * Initialization function of this plugin: the very heart & soul.
 */
int plugin_init(struct plugin_name_args *info, struct plugin_gcc_version *ver)
{
#ifdef DEBUG
    printf("Initializing the multiverse GCC plugin.\n");
#endif

    char *plugin_name = info->base_name;

    // register plugin information
    register_callback(plugin_name, PLUGIN_INFO, NULL, &mv_plugin_info);

    // register the 'multiverse' attribute
    register_callback(plugin_name, PLUGIN_ATTRIBUTES, register_mv_attribute, NULL);

#ifdef DEBUG
    printf("Initializing finished.\n");
#endif
    return 0;
}
