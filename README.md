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

This will create an executable named `a.out`.

## Running

After compiling make sure you are in the `build` directory. Use `./a.out` to run the executable. It takes up to two arguments, the first one being the input file and the second one being an optional output name for the compiled program (defaults to `a.out` which is will overwrite the compiler executable with the same name!). For example you can run
```bash
./a.out ../tests/fibonacci.inslt my_output_file
```
to compile the file `fibonacci.inslt` located in the `tests` directory and name the resulting file `my_output_file`. That file can now be run with `./my_output_file`.
