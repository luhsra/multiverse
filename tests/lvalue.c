/*
 * If a multiverse variable is the lvalue of an assignment, the function won't
 * be specialized for this specific variable.  The compiler throws a warning in
 * such case, since it should be discouraged to set multiverse variables
 * _within_ a multiverse function.
 */

#include "multiverse.h"
#include "testsuite.h"

typedef enum { false, true } bool;


bool __attribute__((multiverse)) conf_a;


void __attribute__((multiverse)) lvalue_func()
{
    int foo = 1;

    if (conf_a)
        foo = conf_a;

    conf_a = false;  // the compiler will throw a warning for this line
}


int main(int argc, char **argv)
{
    multiverse_init();

    multiverse_dump_info();

    lvalue_func();

    return 0;
}
