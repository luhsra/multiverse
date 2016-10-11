#include <stdio.h>

typedef enum { false, true } bool;

__attribute__((multiverse)) bool configA = true;

//__attribute__((multiverse)) int * break_it;


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

    bool * pointer = &configA;
    if (*pointer)
        printf("POINTER\n");

    return res;
}

void lvalue_func()
{
    if (configA)
        foo();

    configA = 0x42;
}


int main(int argc, char **argv)
{
    printf("Hello multiverse!\n");

    foo();


    if (configA) {
        int a;
        a= 3;
    }

    lvalue_func();

    return 0;
}
