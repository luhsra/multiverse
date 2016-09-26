#include <stdio.h>

typedef enum { false, true } bool;

static __attribute__((multiverse)) bool configA = true;


int foo(int a, short b, char c)
{
    if (configA)
        printf("configA\n");
    else
        printf("!~configA\n");
    return 42;
}


int main(int argc, char **argv)
{
    printf("Hallo Welt\n");

    foo(0, 1, 2);

    return 0;
}
