/*
 * Licensed under the GPL v2, or (at your option) v3
 */

#include <gcc-plugin.h>
#include <tree-pass.h>

#define HELPMESSAGE "There you go"

int plugin_is_GPL_compatible;

/*
 * The following data is required for 'gcc --version / --help' and is thus
 * useful to expose the interface of the plugin and further information to the
 * user.
 */
static struct plugin_info mv_plugin_info =
{
    .version = "early bird",
    .help = HELPMESSAGE,
};

/*
 * Specify which version(s) of the GCC are compatible to this plugin.  There are
 * more members, such as time stamp or revision, to further narrow
 * compatibility.  Look into 'plugin-version.h' to check the fields of your
 * infrastructure.
 */
static struct plugin_gcc_version mv_plugin_version =
{
    .basever = "6",
};


static bool mv_gimple_gate()
{
    return true;
}


static unsigned int mv_gimple_execute()
{
    printf("...mv_gimple_execute()\n");
    return 0;
}


int plugin_init(struct plugin_name_args *info, struct plugin_gcc_version *ver)
{
    printf("plugin_init");
    return 0;
}
