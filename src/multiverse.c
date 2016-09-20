/*
 * Licensed under the GPL v2, or (at your option) v3
 */

#include <gcc-plugin.h>


#define HELPMESSAGE "There you go"

int plugin_is_GPL_compatible;

/*
 * The following data is required for 'gcc --version / --help' and is thus
 * useful to expose the interface of the plugin and further information to the
 * user.
 */
//static struct plugin_info mv_plugin_info =
//{
//    .version = "early bird",
//    .help = HELPMESSAGE,
//};

/*
 * Specify which version(s) of the GCC are compatible to this plugin.  There are
 * more members, such as time stamp or revision, to further narrow
 * compatibility.  Look into 'plugin-version.h' to check the fields of your
 * infrastructure.
 */
//static struct plugin_gcc_version mv_plugin_version =
//{
//    .basever = "4.6",
//};

//
//static bool mv_gimple_gate()
//{
//    return true;
//}
//
//
//static unsigned int mv_gimple_execute()
//{
//    printf("...mv_gimple_execute()\n");
//    return 0;
//}


/*
 * See tree-pass.h for a list and descriptions for the fields of this struct.
 */
//static struct gimple_opt_pass mv_gimple_opt_pass =
//{
//    .pass.type = GIMPLE_PASS,
//    .pass.name = "multiverse", /* For use in the dump file */
//
//    /* Predicate (boolean) function that gets executed before your pass.  If the
//     * return value is 'true' your pass gets executed, otherwise, the pass is
//     * skipped.
//     */
//    .pass.gate = mv_gimple_gate,        /* always returns true, see full code */
//    .pass.execute = mv_gimple_execute,  /* Your pass handler/callback */
//};
//


int plugin_init(struct plugin_name_args *info, struct plugin_gcc_version *ver)
{

//   /*
//    * Used to tell the plugin-framework about where we want to be called in the
//    * set of all passes.  This is located in tree-pass.h
//    */
//    struct register_pass_info pass;
//    printf("Plugin initialized...\n");
//
//   /*
//    * We could call: plugin_default_version_check() to validate our plugin, but
//    * we will skip that.  Instead, as mentioned it can be more useful if we
//    * validate the version information ourselves
//    */
////    if (strncmp(ver->basever, myplugin_ver.basever, strlen("4.6")))
////        return -1; /* Incorrect version of GCC */
//
//   /*
//    * Setup the info to register with GCC telling when we want to be called and
//    * to what GCC should call, when it's time to be called.
//    */
//    pass.pass = &my_gimple_opt_pass.pass;
//
//   /*
//    * Get called after GCC has produced the SSA representation of the program.
//    * After the first SSA pass.
//    */
//    pass.reference_pass_name = "ssa";
//    pass.ref_pass_instance_number = 1;
//    pass.pos_op = PASS_POS_INSERT_AFTER;
//
//    // Tell GCC we want to be called after the first SSA pass
//    register_callback("multiverse", PLUGIN_PASS_MANAGER_SETUP, NULL, &pass);
//
//   /*
//    * Tell GCC some information about us... just for use in --help and
//    * --version
//    */
//    register_callback("multiverse", PLUGIN_INFO, NULL, &mv_plugin_info);
//
//    // Successful initialization
    return 0;
}
