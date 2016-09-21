#include <stdio.h>

typedef enum { false, true } bool;

static __attribute__((multiverse)) bool configA = true;

int main(int argc, char **argv)
{
    printf("Hallo Welt\n");
    if (configA)
        printf("configA\n");
    else
        printf("!~configA\n");
    return 0;
}
