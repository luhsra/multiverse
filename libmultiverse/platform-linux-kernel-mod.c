#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");

static int multiverse_test_init_module(void) {
    printk(KERN_INFO "Loading libmultiverse.\n");
    return 0;
}

static void multiverse_test_cleanup_module(void) {
    printk(KERN_INFO "Unloading libmultiverse.\n");
}


module_init(multiverse_test_init_module);
module_exit(multiverse_test_cleanup_module);
