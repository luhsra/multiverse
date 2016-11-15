# multiverse
__Function multiverse__ is a compiler plugin for the GNU C compiler (GCC) to generate multiple versions of a function, each specialized with different variable settings.  An additional __multiverse run-time library__ allows to switch between the different function versions by means of *binary patching*.

Consider the following function `void foo()`, whose control flow is altered by a global variable `config_A`.  At some point during execution we may know that `config_A` is stable for a certain amount of time or simply forever.  Then we also know that `foo()` could run more efficiently since the branch (including compare and jump) could be avoided.  In this case, function multiverse generates two versions of `void foo()`, one with the variable setting `config_A=true` another with `config_A=false`.
```c
static bool config_A;

void foo()
{
    if (config_A) {
        do_a();
    } else {
        do_b();
    }
}
```

__Function multiverse entails several benefits:__
* Code executes and performs as efficiently as possible
* No register clobbering
* No [id]cache pollution
* Ability to identify and compartmentalize dead code
* Ability to decide how to handle dead code at run time

An expected negative impact is the size increase of the binary, so that function multiverse should be used with care.

## Using Function Multiverse
The paradigm of __function multiverse__ is that everything *must* be made explicit.  First, we need to let the compiler know *which functions* should be multiversed with *which variables*.  In terms of GCC, this means we have to attribute functions and variables with `__attribute__((multiverse))` when they are declared.  Second, we need to make it explicit when a certain multiverse variable is doomed stable which further let's the run-time system decide which versions of multiversed functions will be used.

The code below is an example of how multiverse can be used:
```c
static __attribute__((multiverse)) bool config_A;

void __attribute__((multiverse)) foo()
{
    if (config_A) {
        do_a();
    } else {
        do_b();
    }
    return 0;
}

int main()
{
    foo();

    config_A = true;
    gcc_multiverse_commit(&config_A);

    foo();

    return 0;
}
```

Compiling the upper code with the multiverse plugin will additionally compile the following two specialized versions of `void foo()` in the binary.  As soon as we know that `config_A` is stable (e.g., it's set to true) we can signal the multiverse run-time system via `gcc_multiverse_commit(&config_A)`.  The run-time system will then determine the specialized function version `foo_config_A_true()` and binary patch *all* call sites to use `foo_config_A_true()` instead of the generic `foo()`.  The generic function version of `foo()` will also be patched to jump to the new version, since `foo()` might still be callable, for instance via function pointers.
```c
void foo_config_A_true()
{
  do_a();
  return 0;
}

void foo_config_B_true()
{
  do_b();
  return 0;
}
```

## Dependencies
To test and use multiverse, you need the following packages:
* __Debian__:
 * g++-$VERSION
 * gcc-$VERSION
 * gcc-$VERSION-source
 * gcc-$VERSION-plugin-dev
* __openSUSE__:
 * gcc-devel
