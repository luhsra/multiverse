#include <stdio.h>
#include "multiverse.h"
#include "testsuite.h"

typedef enum {true, false} bool;

__attribute__((multiverse("tracked"))) bool conf_a;
__attribute__((multiverse("tracked"))) bool conf_b;


void __attribute((multiverse, noinline)) foo()
{
    if (conf_a) {
        // do a-ish stuff
        printf("Hallo Welt - A!\n");
    }
    else if (conf_b) {
        // do b-ish stuff
        printf("Hallo Welt - B!\n");
    }
}


int main(int argc, char **argv)
{
    foo();
    return 0;
}
