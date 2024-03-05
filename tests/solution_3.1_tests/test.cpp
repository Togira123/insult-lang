#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 4) {
        return 1;
    }
    std::string expected_output = argv[1];
    std::string compiled_program = argv[2];
    std::string output_name = argv[3];
    std::ofstream outfile(output_name);
    auto command = "./" + compiled_program + ">" + output_name;
    if (system(command.c_str()) != 0) {
        return 2;
    }

    std::ifstream expected(expected_output);
    std::ifstream actual(output_name);

    std::string expected_line, actual_line;
    while (std::getline(expected, expected_line) && std::getline(actual, actual_line)) {
        if (expected_line != actual_line) {
            return 1;
        }
    }
    if (std::getline(expected, expected_line) || std::getline(actual, actual_line)) {
        return 1;
    }
    return 0;
}
