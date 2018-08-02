/*
 * Make sure that multiverse functions can be static.
 */

#include <stdio.h>

__attribute__((multiverse)) int a;

static __attribute__((multiverse)) int func(void) {
    if (a)
        return 0;
    else
        return 1;
}

int main(int argc, char **argv)
{
    // We are not interested in the runtime behaviour.
    // Just make sure this compiles.
    printf("func returns: %d\n", func());
    return 0;
}
