#include <stdio.h>

typedef enum { false, true } bool;

static __attribute__((multiverse)) bool configA = true;


void foo()
{
    if (configA)
        printf("configA\n");
    else
        printf("!~configA\n");
}


int main(int argc, char **argv)
{
    printf("Hallo Welt\n");

    foo();

    return 0;
}
