#include <stdio.h>

typedef enum { false, true } bool;

__attribute__((multiverse)) int configA = true;


int foo()
{
    int res;

    if (configA) {
        printf("configA = true\n");
        res = 1;
    } else {
        printf("configA = false\n");
        res = 0;
    }

    return res;
}


int main(int argc, char **argv)
{
    printf("Hallo Welt\n");

    foo();

    if (configA) {
        int a;
        a= 3;
    }

    foo();

    return 0;
}
