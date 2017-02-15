/*
 * Multi-object and pthreads.
 */

#include <stdio.h>
#include <pthread.h>
#include "multiverse.h"
#include "testsuite.h"

#include "module.h"


void * run()
{
    printf("----thread running\n");

    printf("----we just reverted, so everything's is generic\n");
    conf_a = 13; printf("conf_a=%d, func_a()=%d\n", conf_a, func_a());
    conf_a = 0; printf("conf_a=%d, func_a()=%d\n", conf_a, func_a());

    printf("----now let's commit (conf_a=0), and set conf_a to 1\n");
    multiverse_commit_refs(&conf_a);
    conf_a = 1; printf("conf_a=%d, func_a()=%d\n", conf_a, func_a());

    printf("----thread exiting\n");
}


int main(int argc, char **argv)
{
    printf("Hello world!\n");

    multiverse_init();
    conf_a = 0, conf_b = 0;

    assert(conf_a == 0 && func_a() == 0 && conf_b ==0 && func_b() ==0);

    printf("a=%d, b=%d\n", conf_a, conf_b);

    printf("----commit conf_a\n");
    multiverse_commit_refs(&conf_a);
    conf_a = 1; assert(func_a() == 0);

    printf("----commit conf_b\n");
    multiverse_commit_refs(&conf_b);
    conf_b = 2; assert(func_b() == 0);

    printf("----multiverse_revert()\n");
    multiverse_revert();
    conf_a = 42; assert(func_a() == 42);
    conf_b = 23; assert(func_b() == 23);

    pthread_t thread;
    int ret = pthread_create(&thread, NULL, run, NULL);
    assert(!ret);

    void *res = 0;
    pthread_join(thread, res);

    printf("----back in main\n");
    conf_a = 1; printf("conf_a=%d, func_a()=%d\n", conf_a, func_a());
    assert(conf_a == 1 && func_a() == 0);
    return 0;
}
