#include <stdio.h>

typedef enum { false, true } bool;

__attribute__((multiverse)) bool configA;
__attribute__((multiverse)) bool configB;

//__attribute__((multiverse)) int * break_it;


int foo()
{
    if (configA) {
        printf("configA = true\n");
    } else {
        printf("configA = false\n");
    }

    return 42;
}


//void lvalue_func()
//{
//    if (configA)
//        foo();
//
//    configA = false;
//}
//
//
//void two_references()
//{
//    if (configA) {
//        printf("configA = true\n");
//    }
//
//    if (configB) {
//        printf("configB = true\n");
//    }
//
//    printf("two multiverse variables\n");
//
//}


int main(int argc, char **argv)
{
    printf("Hello multiverse!\n");

    foo();


    if (configA) {
        int a;
        a= 3;
    }

//    lvalue_func();

    return 0;
}
