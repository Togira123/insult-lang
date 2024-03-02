# Exercises

## What is this folder?
It contains exercises for you to solve. The exercises are designed to help you better understand the INSULT compiler (and compilers in general). They are grouped by difficulty in different folders:
- 1: The easiest exercises. They require you to only change the scanner.
- 2: More advanced exercises. Require you to mess with the parser.
- 3: Very advanced exercises. To solve these you must change things across multiple files.

## Solutions?
Yes. Solutions for each of the exercises are found in their respective branches. They are named `exercise_solution_<number>`, where `<number>` is the number of the exercise. You can look for a commit that is called something like "added solution for exercise" to see the exact changes that were made or just search for the respective tag (called `solution_<number>`).

## Tests?
Yes. Some tests for your potential solution are found on the solution branches in `tests/solution_{number}_tests/`. The tests are added in the `CMakeLists.txt` file. To run them, you can `cd` into `build` and then run `ctest ../CMakeLists.txt` (requires [Cmake](https://cmake.org/cmake/help/book/mastering-cmake/chapter/Getting%20Started.html) to be installed).
