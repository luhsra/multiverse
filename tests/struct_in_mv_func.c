/*
 * This test addresses an (now fixed) issue (in the elimination pass) that
 * caused a compiler segfault when a struct or an union is used in a multiversed
 * function.
 */

struct test_struct {
    int member;
};

__attribute__((multiverse)) long test_mv_var;
struct test_struct test_struct_var1;
struct test_struct test_struct_var2;

__attribute__((multiverse)) int test_mv_func(void)
{
    test_struct_var1 = test_struct_var2;

    if (test_mv_var)
        return 0;
    else
        return 1;
}

int main(int argc, char **argv)
{
    // We are not interested in the runtime behaviour.
    // Just make sure this compiles.
    test_mv_func();
    return 0;
}
