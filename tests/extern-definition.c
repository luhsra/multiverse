/*
 * A very simple test case with
 *  - one boolean variable 'config'
 *  - one function 'foo' that returns 'config'
 *
 * The test also checks some multiverse run-time data.
 */

#include <stdio.h>
#include "multiverse.h"
#include "testsuite.h"

typedef enum {true, false} bool;

#define M __attribute__((multiverse))

extern M bool unused;
extern M void unused_func();



extern M bool config_1;
bool M config_1;

extern M bool config_2;
bool config_2;

extern bool config_3;
bool M config_3;



extern M int func_1();
int M func_1() {
    return config_1 + config_2 + config_3;
}

extern int func_2();
int M func_2() {
    return config_1 + config_2 + config_3;
}

extern M int func_3();
int func_3() {
    return config_1 + config_2 + config_3;
}



int main(int argc, char **argv)
{
    multiverse_init();


    //multiverse_dump_info();

    // All three functions should be equal
    assert(body_count(&func_1) == 4);
    assert(desc_count(&func_1) == 8);

    assert(body_count(&func_2) == 4);
    assert(desc_count(&func_2) == 8);

    assert(body_count(&func_3) == 4);
    assert(desc_count(&func_3) == 8);


    return 0;
}
