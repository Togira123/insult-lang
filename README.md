# INSULT - Incredibly Nasty Script Unleashing Literal Tantrums

## Compiling
### Prerequisites
To compile the compiler you need the following packages:
* g++

    Check if it is installed with

    ```bash
    g++ version
    ```

    and if not installed, install with

    ```bash
    xcode-select --install
    ```

    This should install g++ to `/usr/bin/g++` which is where cmake expects it to be. Make sure this is the case.

    This will install tons of useful packages, including `g++`.

* make

    Check if it is installed with

    ```bash
    make --version
    ```

    and otherwise install with

    ```bash
    xcode-select --install
    ```

    This will install tons of useful packages, including `make`.

* cmake

    Check if it is installed with

    ```bash
    cmake --version
    ```

    and otherwise install with

    ```bash
    brew install cmake
    ```
    (In case `brew` is not installed on your computer you can install it following the instructions [here](https://brew.sh))

### Compile
To compile first create a directory named `build` in the root and then run

```bash
cd build && cmake .. && make
```

This will create an executable named `inslt`.

## Running

After compiling make sure you are in the `build` directory. Use `./inslt` to run the executable. It takes one argument which is the name of the file to compile. For example you can run
```bash
./inslt ../tests/fibonacci.inslt
```
to compile the file `fibonacci.inslt` located in the `tests` directory. The resulting file will have the default name `a.out`. That file can now be run with `./a.out`.

### Compiler flags

There are a few compiler flags which can be used to enable/disable certain features the compiler can offer. To use them just append them to the input file name, separated by spaces. Below is a list of all the currently available flags.

* __-optimize__

    Using this flag will tell the compiler to perform some optimization steps in order to make the resulting executable run faster and use up less memory. Omitting this flag will **not** prevent the compiler from performing some minor changes to the input.

* __-out__ <*name*>

    This flag can be used to set the name of the output file. This defaults to `a.out`. Set the name without the angle brackets.

* __-forbid_library_names__

    This flag forbids names of the standard library being used to define identifiers.

* __-emit_cpp__

    This flag emits the generated C++ code
