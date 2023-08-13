#include "../ir.h"
#include <string>
#include <unordered_set>

const std::unordered_set<std::string> library_functions = {"print", "read_line", "size", "copy", "ref"};

identifier_detail& identifier_detail_of(intermediate_representation& ir, const std::string& id_name);

std::string generate_print();

std::string generate_read_line();

std::string generate_size();
