#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

void multiverse_fn(void);

static int multiverse_test_init_module(void) {
    printk(KERN_INFO "Hello from multiverse_test.\n");

    multiverse_fn();

    return 0;
}

static void multiverse_test_cleanup_module(void) {
    printk(KERN_INFO "Goodbye from multiverse_test.\n");
}


module_init(multiverse_test_init_module);
module_exit(multiverse_test_cleanup_module);
