#include "module.h"

int __attribute((multiverse)) func_a()
{
    return conf_a;
}


int __attribute((multiverse)) func_b()
{
    return conf_b;
}
