#include <stdio.h>
#include <assert.h>
#include "multiverse.h"

__attribute__((multiverse(("values", 1, 3, 5)))) unsigned conf;

int __attribute__((multiverse)) func() {
    return conf;
}

int main(int argc, char **argv)
{
    multiverse_init();

    multiverse_dump_info(stderr);

    return 0;
}
