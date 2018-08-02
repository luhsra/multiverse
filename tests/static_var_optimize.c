/*
 * Prevent static multiverse variables from getting optimized out.
 */

static __attribute__((multiverse)) int a;

__attribute__((multiverse)) int func() {
    !a;
    return 0;
}

int main(int argc, char **argv)
{
    // We are not interested in the runtime behaviour.
    // Just make sure this compiles.
    func();
    return 0;
}
