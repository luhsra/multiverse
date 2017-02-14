/*
 * Multiverse supports enumerators in ways that a referencing function will be
 * specialized for each possible value.  The function func() below references
 * two multiverse variables.  Both variables are of type state, which is an enum
 * with 5 possible values [0,4].  Multiverse will thus generate 25 clones of
 * func().
 */

#include <stdio.h>
#include "multiverse.h"
#include "testsuite.h"

typedef enum { a=0, b, c, d, e } state;


__attribute__((multiverse)) state conf_a;
__attribute__((multiverse)) state conf_b;


int __attribute((multiverse)) func()
{
    printf("conf_a=%d, conf_b=%d\n", conf_a, conf_b);
    return conf_a + conf_b;
}


int main(int argc, char **argv)
{
    multiverse_init();

    conf_a = a; // 0
    multiverse_commit_refs(&conf_a);
    func();      // conf_a=0, conf_b=0

    conf_b = b; // 1
    func();      // conf_a=0, conf_b=1
    multiverse_commit_refs(&conf_b);
    func();      // conf_a=0, conf_b=1

    return 0;
}
