#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/multiverse.h>

MODULE_LICENSE("GPL");

static __attribute__((multiverse)) bool config_A = false;
static __attribute__((multiverse)) bool config_B = false;

void __attribute__((multiverse)) foo(void) {
    label:
    printk(KERN_INFO "foo at: %p\n", &&label);
    if (config_A)
        printk(KERN_INFO ">>> config_A\n");
    if (config_B)
        printk(KERN_INFO ">>> config_B\n");
}

static int multiverse_test_init_module(void) {
    printk(KERN_INFO "Hello from multiverse_test.\n");

    multiverse_init();

    config_A = true;
    multiverse_commit_refs(&config_A);
    foo();
    config_B = true;
    multiverse_commit_refs(&config_B);
    foo();

    return 0;
}

static void multiverse_test_cleanup_module(void) {
    printk(KERN_INFO "Goodbye from multiverse_test.\n");
}


module_init(multiverse_test_init_module);
module_exit(multiverse_test_cleanup_module);
