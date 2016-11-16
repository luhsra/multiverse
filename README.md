# Function Multiverse
__Function multiverse__ is a compiler plugin for the GNU C compiler (GCC) to generate multiple versions of a function, each specialized with different variable settings.  An additional __multiverse run-time library__ allows to switch between the different function versions by means of *binary patching*.

Consider the following function `foo()` whose control flow is altered by a global variable `config_A`.  At some point during execution we may know that `config_A` is stable for a certain amount of time or simply forever.  Then we also know that `foo()` could run more efficiently since the branch (including compare and jump) could be avoided.  In this case, function multiverse generates two versions of `foo()` one with the variable setting `config_A=true` another with `config_A=false`.

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

An expected negative impact is the size increase of the binary, so that function multiverse should be used with care.  However, optimizations such as clone detection help to reduce the size overhead.


## Using Function Multiverse
The paradigm of __function multiverse__ is that everything *must* be made explicit.  First, we need to let the compiler know *which functions* should be multiversed with *which variables*.  In terms of GCC, this means we have to attribute functions and variables with `__attribute__((multiverse))` when they are declared.  Second, we need to make it explicit when a certain multiverse variable is stable (e.g., via `gcc_multiverse_commit()`) which further let's the run-time system decide which versions of multiversed functions referencing this variable will be used.

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

Compiling the upper code with the multiverse plugin will additionally compile the following two specialized versions of the generic `foo()` in the binary.  Internally, the plugin clones the generic function with the corresponding variable settings.  Later compiler optimizations such as constant propagation and dead-code elimination further optimize the code.  As soon as we know that `config_A` is stable (e.g., it's set to true) we can signal the multiverse run-time system via `gcc_multiverse_commit(&config_A)`.  The run-time system will then determine the specialized function version `foo_config_A_true()` and binary patch *all* call sites to use `foo_config_A_true()` instead of the generic `foo()`.  The generic function version of `foo()` will also be patched to jump to the new version, since `foo()` might still be reachable, for instance via function pointers.

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

## What Can Be Multiversed?
* __Variables__
  * Currently, only boolean-like variables are supported (i.e., either integer or enumeral type).
* __Functions__
  * In general, all functions can be attributed with multiverse, but they cannot be inlined.


## How Does Function Multiverse Work?
Function multiverse is implemented as a GCC plugin, which adds several passes to the middle-end and back-end of the compiler.  The first step is the attribution of variables and functions.  As aforementioned, only boolean-like variables are supported and multiverse functions cannot be inlined.  The second step is to determine how many specialized versions of a multiverse-attributed function must be generated.  Thus, the plugin checks which multiverse variables are referenced in a multiverse function and builds all combinations of those variables.  Currently, all multiverse variables are considered to be independent, so the plugin generates an exponential amount of combinations.  Let's consider the following code snippet:

```c
static __attribute__((multiverse)) bool config_A;
static __attribute__((multiverse)) bool config_B;

void __attribute__((multiverse)) foo()
{
  if (config_A)
    do_a();
  if (config_B)
    do_b();
}
```

The upper function `foo()` references the multiverse variables `config_A` and `config_B`, so that there are _2^2_ combinations and thus _4 specialized versions_ of `foo()` with the variable settings (config_A=[0,1], config_B=[0,1]).  Notice, that a multiverse variable is excluded from the variable settings if the current function assigns some value to it.  For instance, if the upper function `foo()` had the statement `config_A=42;` the plugin would throw a compiler warning and exclude `config_A` from the variable setting.  In this case, only two specialized version of `foo()` would be generated, one with `config_B=0` another with `config_B=1`.


### The Run-Time Library
TODO: let's wait until the API stable


## Building and Using Multiverse
To build the plugin you need the following packages:
* __Debian__:
  * g++-$VERSION
  * gcc-$VERSION
  * gcc-$VERSION-source
  * gcc-$VERSION-plugin-dev
* __openSUSE__:
  * gcc
  * gcc-c++
  * gcc-devel

After having built the plugin with `$ make` you can use it via `$ gcc -fplugin=multiverse.so`.
